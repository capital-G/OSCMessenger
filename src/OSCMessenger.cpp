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

    // calc one output sample
    next_k(1);
}

void OSCMessenger::extractArgs() {
    // string extraction "stolen" from SendReply

    // you'll need to define unit in order to use ClearUnitIfMemFailed
    Unit* unit = (Unit*) this;

    /*
    args = [
        rate, <-- ignore
        portNumber,
        trigger,
        oscAddressAscii.size,
        doneAddressAscii.size,
        doneValue,
    ].addAll(oscAddressAscii).addAll(doneAddress).addAll(values);
    */
    mPortNumber = (int) in0(0);
    int oscAddressSize = in0(2);
    int doneAddressSize = in0(3);
    mDoneValue = *in(4);
    mSendDoneMessage = doneAddressSize > 0.5;
    const int oscAddressOffset = 5;
    const int doneAddressOffset = oscAddressOffset + oscAddressSize;
    mValueOffset = oscAddressOffset + oscAddressSize + doneAddressSize;
    mNumValues = mNumInputs - mValueOffset;

    // +1 b/c of null termination
    const int oscAddressAllocSize = (oscAddressSize + 1) * sizeof(char);    
    char *oscAddressChunk = (char *)RTAlloc(mWorld, oscAddressAllocSize);
    ClearUnitIfMemFailed(oscAddressChunk);
    mOscAddress = oscAddressChunk;
    for (int i = 0; i < (int)oscAddressSize; i++) {
        mOscAddress[i] = (char)in0(oscAddressOffset + i);
    }
    // terminate string
    mOscAddress[oscAddressSize] = 0;

    // same procedure
    const int doneAddressAllocSize = (doneAddressSize + 1) * sizeof(char);
    char *doneAddressChunk = (char *)RTAlloc(mWorld, doneAddressAllocSize);
    ClearUnitIfMemFailed(doneAddressChunk);
    mDoneAddress = doneAddressChunk;
    for (int i = 0; i < (int)doneAddressSize; i++) {
        mDoneAddress[i] = (char)in0(doneAddressOffset + i);
    }
    mOscAddress[oscAddressSize] = 0;
}

void OSCMessenger::setupEndpoint() {
    asio::ip::udp::resolver resolver(mIoContext);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::resolver::query("127.0.0.1", std::to_string(mPortNumber)));
    mEndpoint = *endpoints.begin();
}

void OSCMessenger::allocBuffers() {
    Unit* unit = (Unit*) this;
    mBuffer = (char *)RTAlloc(mWorld, OUTPUT_BUFFER_SIZE);
    ClearUnitIfMemFailed(mBuffer);
}

void OSCMessenger::next_k(int nSamples) {
    OSCPP::Client::Packet packet(mBuffer, OUTPUT_BUFFER_SIZE);
    bool trigger = in0(1) > 0.0;

    if(trigger) {
        packet.openMessage(mOscAddress, mNumValues);
        for(int i=0; i<mNumValues; i++) {
            packet.float32(in0(i+mValueOffset));
        }
        packet.closeMessage();
        mSocket.send_to(asio::buffer(mBuffer, packet.size()), mEndpoint);
    }

    out0(0) = 0.0;
}

OSCMessenger::~OSCMessenger() {
    if(mSendDoneMessage) {
        OSCPP::Client::Packet packet(mBuffer, OUTPUT_BUFFER_SIZE);
        packet.openMessage(mDoneAddress, 1)
            .float32(mDoneValue)
        .closeMessage();
        mSocket.send_to(asio::buffer(mBuffer, packet.size()), mEndpoint);
    }
}

} // namespace OSCMessenger

PluginLoad(OSCMessengerUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<OSCMessenger::OSCMessenger>(ft, "OSCMessenger", false);
}
