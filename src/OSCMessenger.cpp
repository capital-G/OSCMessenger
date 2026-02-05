#include <stdio.h>
#include <string.h>
#include "SC_PlugIn.hpp"
#include "OSCMessenger.hpp"

#include <oscpp/client.hpp>
#include <asio.hpp>

static InterfaceTable *ft;

static std::thread gOscThread;
static asio::io_context gIoContext;
static asio::ip::udp::socket gSocket = asio::ip::udp::socket(gIoContext, asio::ip::udp::v4());
static std::mutex gQueueMutex;
std::queue<OSCMessenger::MessageTuple> gPacketQueue;
std::condition_variable gQueueCondition;
static bool gStopWorker = false;

#define OSC_BUFFER_SIZE 4096

namespace OSCMessenger {

void SharedInfo::free() {
    RTFree(mWorld, mOscAddress);
    RTFree(mWorld, mDoneAddress);
    RTFree(mWorld, mHostAddress);
    // @todo is this safe?!
    RTFree(mWorld, this);
}

void sendPackets() {
    while (true) {
        std::unique_lock<std::mutex> lock(gQueueMutex);
        gQueueCondition.wait(lock, [] { return !gPacketQueue.empty() || gStopWorker; });
        if (gStopWorker && gPacketQueue.empty()) {
            break;
        }
        auto packet = std::move(gPacketQueue.front());
        gPacketQueue.pop();
        lock.unlock();
        gSocket.send_to(asio::buffer(packet.buffer), packet.endpoint);
    }
}

OSCMessenger::OSCMessenger() {
    const auto unit = static_cast<OSCMessenger*>(this);
    mCalcFunc = make_calc_function<OSCMessenger, &OSCMessenger::next_k>();

    mSharedInfo = static_cast<SharedInfo*>(RTAlloc(mWorld, sizeof(SharedInfo)));
    ClearUnitIfMemFailed(mSharedInfo);
    mSharedInfo->mWorld = mWorld;
    mSharedInfo->mRefCount = 1;

    // keep this in sync with *new1 in sclang land
    static const int dynamicArgsOffset = 7;

    // trigger skipped
    const int oscAddressSize = static_cast<int>(in0(2));
    const int doneAddressSize = static_cast<int>(in0(3));
    const int hostSize = static_cast<int>(in0(5));
    const bool appendNodeId = in0(6) > 0.5;
    mDoneValue = *in(4);
    mSendDoneMessage = doneAddressSize > 0.5;

    mSharedInfo->mOscAddress = extractString(2, dynamicArgsOffset);
    ClearUnitIfMemFailed(mSharedInfo->mOscAddress);
    mSharedInfo->mDoneAddress = extractString(3, dynamicArgsOffset + oscAddressSize);
    ClearUnitIfMemFailed(mSharedInfo->mDoneAddress);
    mSharedInfo->mHostAddress = extractString(5, dynamicArgsOffset + oscAddressSize + doneAddressSize);
    ClearUnitIfMemFailed(mSharedInfo->mHostAddress);

    mSharedInfo->mPortNumber = static_cast<int>(in0(0));
    mSharedInfo->mNodeId = mParent->mNode.mID;
    mSharedInfo->mAppendNodeId = in0(6) > 0.5;;

    mValueOffset = dynamicArgsOffset + oscAddressSize + doneAddressSize + hostSize;
    mNumValues = mNumInputs - mValueOffset;

    // increment ref counter for async call
    mSharedInfo->mRefCount += 1;
    // setup endpoint async in nrt thread
    ft->fDoAsynchronousCommand(mWorld, nullptr, nullptr, this->mSharedInfo,
        [](World* world, void* cmdData)->bool {
            auto info = static_cast<SharedInfo*>(cmdData);
            asio::ip::udp::resolver resolver(gIoContext);
            asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::resolver::query(info->mHostAddress, std::to_string(info->mPortNumber)));
            info->mEndpoint = *endpoints.begin();
            // @todo how do i write an atomic? do i need relaxed or something like this?
            info->mEndpointReady = true;
            return true;
        }, [](World*, void* cmdData)->bool {
            auto info = static_cast<SharedInfo*>(cmdData);
            info->mRefCount -= 1;
            if (info->mRefCount <= 0) {
                info->free();
            }
            return true;
        }, nullptr,
        [](World* world, void* cmdData)->void {}, 0, nullptr);
}

char* OSCMessenger::extractString(int sizeIndex, int startIndex) const {
    const int size = static_cast<int>(in0(sizeIndex));
    // +1 b/c of null termination
    auto buffer = static_cast<char*>(RTAlloc(mWorld, (size + 1) * sizeof(char)));

    if (buffer == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < size; i++) {
        buffer[i] = static_cast<char>(in0(startIndex + i));
    }
    // terminate string
    buffer[size] = 0;
    return buffer;
}

// can't pass isDone flag as parameter ot asyncfn - so we use a template
template <bool isDoneMessage>
bool pushMessageToQueue(World* world, void* cmdData) {
    auto message = static_cast<Message*>(cmdData);
    std::vector<char> buffer(OSC_BUFFER_SIZE);
    auto packet = OSCPP::Client::Packet(buffer.data(), buffer.size());
    auto address = isDoneMessage ? message->mSharedInfo->mDoneAddress : message->mSharedInfo->mOscAddress;
    packet.openMessage(address, message->mNumValues);
    for (int i = 0; i < message->mNumValues; i++) {
        packet.float32(message->mValues[i]);
    }
    packet.closeMessage();
    // shrink to actual size
    buffer.resize(packet.size());

    auto tuple = MessageTuple();
    tuple.endpoint = message->mSharedInfo->mEndpoint;
    tuple.buffer = std::move(buffer);
    {
        std::lock_guard<std::mutex> lock(gQueueMutex);
        gPacketQueue.push(tuple);
    }
    gQueueCondition.notify_one();

    return true;
}

void OSCMessenger::sendMessage(Message* message, bool isDoneMessage) {
    auto queueFunc = isDoneMessage ? pushMessageToQueue<true> : pushMessageToQueue<false>;
    mSharedInfo->mRefCount += 1;
    ft->fDoAsynchronousCommand(mWorld, nullptr, nullptr, message,
    queueFunc,
    [](World* world, void* cmdData)->bool {
        return true;
    }, nullptr, [](World* world, void* cmdData)->void {
        auto message = static_cast<Message*>(cmdData);
        message->mSharedInfo->mRefCount -= 1;
        if (message->mSharedInfo->mRefCount <= 0) {
            message->mSharedInfo->free();
        }
        RTFree(world, message->mValues);
        RTFree(world, message);
    }, 0, nullptr);
}

void OSCMessenger::next_k(int nSamples) {
    bool trigger = in0(1) > 0.0;

    if(trigger && mSharedInfo->mEndpointReady) {
        auto message = static_cast<Message*>(RTAlloc(mWorld, sizeof(Message)));
        if (message==nullptr) {
            Print("ERROR: Not enough memory to allocate OSC message\n");
            goto end;
        }
        auto numValues = mSharedInfo->mAppendNodeId ? mNumValues +1 : mNumValues;
        auto values = static_cast<float*>(RTAlloc(mWorld, sizeof(float) * numValues));
        if (values == nullptr) {
            RTFree(mWorld, message);
            goto end;
        }
        message->mSharedInfo = mSharedInfo;
        message->mNumValues = numValues;
        message->mValues = values;
        if (mSharedInfo->mAppendNodeId) {
            message->mValues[0] = static_cast<float>(mParent->mNode.mID);
        }
        for (int i = 0; i < mNumValues; i++) {
            message->mValues[(mSharedInfo->mAppendNodeId ? 1 : 0)+ i] = in0(i + mValueOffset);
        }
        sendMessage(message, false);
    }
    end:
    // dummy operation to avoid c++23 only feature
    trigger = false;
    // do not write to/set out0 - acts as pass through
}

OSCMessenger::~OSCMessenger() {
    if(mSendDoneMessage) {
        auto message = static_cast<Message*>(RTAlloc(mWorld, sizeof(Message)));
        if (message==nullptr) {
            Print("ERROR: Not enough memory to allocate OSC message\n");
            goto end;
        }
        auto numValues = mSharedInfo->mAppendNodeId ? 2 : 1;
        auto values = static_cast<float*>(RTAlloc(mWorld, sizeof(float) * numValues));
        if (values == nullptr) {
            Print("ERROR: Not enough memory to allocate OSC message\n");
            RTFree(mWorld, message);
            goto end;
        }
        if (mSharedInfo->mAppendNodeId) {
            values[0] = static_cast<float>(mParent->mNode.mID);
            values[1] = mDoneValue;
        } else {
            values[0] = mDoneValue;
        }
        message->mNumValues = numValues;
        message->mValues = values;
        message->mSharedInfo = mSharedInfo;
        sendMessage(message, true);
    }
    end:
    mSharedInfo->mRefCount -= 1;
    if (mSharedInfo->mRefCount <= 0) {
        mSharedInfo->free();
    }
}

} // namespace OSCMessenger

PluginLoad(OSCMessenger) {
    // Plugin magic
    ft = inTable;
    registerUnit<OSCMessenger::OSCMessenger>(ft, "OSCMessenger", false);

    gOscThread = std::thread(OSCMessenger::sendPackets);
}

PluginUnload(OSCMessenger) {
    {
        std::lock_guard<std::mutex> lock(gQueueMutex);
        gStopWorker = true;
    }
    gQueueCondition.notify_one();
    gIoContext.stop();
    gOscThread.join();
}
