#include <stdio.h>
#include <string.h>
#include "SC_PlugIn.hpp"
#include "OSCMessenger.hpp"

#include <oscpp/client.hpp>
#include <asio.hpp>

static InterfaceTable *ft;

#define OUTPUT_BUFFER_SIZE 4096

namespace OSCMessenger {

OSCMessenger::OSCMessenger() : mSocket(mIoContext, asio::ip::udp::v4()) {
    mCalcFunc = make_calc_function<OSCMessenger, &OSCMessenger::next_k>();
    
    allocBuffers();
    extractArgs();
    setupEndpoint();

    // start consuming thread
    mWorkerThread = std::thread(&OSCMessenger::sendPackets, this);

    // calc one output sample
    next_k(1);
}

void OSCMessenger::extractArgs() {
    // keep this in sync with *new1 in sclang land
    const int dynamicArgsOffset = 7;

    mPortNumber = (int) in0(0);
    // trigger skipped
    const int oscAddressSize = in0(2);
    const int doneAddressSize = in0(3);
    const int hostSize = in0(5);
    const bool appendNodeId = in0(6) > 0.5;
    mDoneValue = *in(4);
    mSendDoneMessage = doneAddressSize > 0.5;

    mOscAddress = extractString(2, dynamicArgsOffset);
    mDoneAddress = extractString(3, dynamicArgsOffset + oscAddressSize);
    mHostAddress = extractString(5, dynamicArgsOffset + oscAddressSize + doneAddressSize);

    if (appendNodeId) {
        std::string nodeID = std::to_string(mParent->mNode.mID);
        char const *nodeIDchar = nodeID.c_str(); 
        strcat(mOscAddress, nodeIDchar);
        strcat(mDoneAddress, nodeIDchar);
    }

    mValueOffset = dynamicArgsOffset + oscAddressSize + doneAddressSize + hostSize;
    mNumValues = mNumInputs - mValueOffset;
}

char* OSCMessenger::extractString(int sizeIndex, int startIndex) {
    // you'll need to define unit in order to use ClearUnitIfMemFailed
    Unit* unit = (Unit*) this;

    const int size = in0(sizeIndex);
    // +1 b/c of null termination
    const int stringAllocSize = (size + 1) * sizeof(char);    
    char* buffer = (char*) RTAlloc(mWorld, stringAllocSize);

    // on linux this will complain b/c the template can return void
    // ClearUnitIfMemFailed(buffer);

    for (int i = 0; i < size; i++) {
        buffer[i] = (char) in0(startIndex + i);
    }

    // terminate string
    buffer[size] = 0;

    return buffer;
}

void OSCMessenger::setupEndpoint() {
    asio::ip::udp::resolver resolver(mIoContext);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::resolver::query(mHostAddress, std::to_string(mPortNumber)));
    mEndpoint = *endpoints.begin();
}

void OSCMessenger::allocBuffers() {
    Unit* unit = (Unit*) this;
    mBuffer = (char *)RTAlloc(mWorld, OUTPUT_BUFFER_SIZE);
    ClearUnitIfMemFailed(mBuffer);
}

void OSCMessenger::next_k(int nSamples) {
    bool trigger = in0(1) > 0.0;

    if(trigger) {
        OSCPP::Client::Packet packet(mBuffer, OUTPUT_BUFFER_SIZE);
        packet.openMessage(mOscAddress, mNumValues);
        for(int i=0; i<mNumValues; i++) {
            packet.float32(in0(i+mValueOffset));
        }
        packet.closeMessage();

        // push packet onto the queue
        {
            std::lock_guard<std::mutex> lock(mQueueMutex);
            mPacketQueue.push(std::vector<char>(mBuffer, mBuffer + packet.size()));
        }
        mQueueCondition.notify_one();
        // mSocket.send_to(asio::buffer(mBuffer, packet.size()), mEndpoint);

    }

    out0(0) = 0.0;
}

void OSCMessenger::sendPackets() {
    while (true) {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        mQueueCondition.wait(lock, [this] { return !mPacketQueue.empty() || mStopWorker; });

        if (mStopWorker && mPacketQueue.empty()) {
            break;
        }

        std::vector<char> packet = std::move(mPacketQueue.front());
        mPacketQueue.pop();
        lock.unlock();

        mSocket.send_to(asio::buffer(packet), mEndpoint);
    }
}


OSCMessenger::~OSCMessenger() {
    // send done message
    if(mSendDoneMessage) {
        OSCPP::Client::Packet packet(mBuffer, OUTPUT_BUFFER_SIZE);
        packet.openMessage(mDoneAddress, 1)
            .float32(mDoneValue)
        .closeMessage();
        mSocket.send_to(asio::buffer(mBuffer, packet.size()), mEndpoint);
    }

    // stop the worker thread
    {
        std::lock_guard<std::mutex> lock(mQueueMutex);
        mStopWorker = true;
    }
    mQueueCondition.notify_one();
    mWorkerThread.join();

    RTFree(mWorld, mBuffer);
    RTFree(mWorld, mOscAddress);
    RTFree(mWorld, mDoneAddress); 
    RTFree(mWorld, mHostAddress); 
}

} // namespace OSCMessenger

PluginLoad(OSCMessengerUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<OSCMessenger::OSCMessenger>(ft, "OSCMessenger", false);
}
