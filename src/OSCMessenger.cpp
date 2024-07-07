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
    mValueOffset = extractOscAddress();
    setupEndpoint(extractPortNumber());
    setupValues(mValueOffset);

    // calc one output sample
    next_k(1);
}

int OSCMessenger::extractOscAddress() {
    // you'll need to define unit in order to use ClearUnitIfMemFailed
    Unit* unit = (Unit*) this;
    
    // stolen from SendReply
    // offset is due to port, addressSize, oscAddressArgs...
    const int kVarOffset = 2;
    int m_oscAddressSize = in0(kVarOffset-1);

    // +1 b/c of null termination
    const int oscAddressAllocSize = (m_oscAddressSize + 1) * sizeof(char);
    
    char *chunk = (char *)RTAlloc(mWorld, m_oscAddressSize);
    ClearUnitIfMemFailed(chunk);
    mOscAddress = chunk;

    for (int i = 0; i < (int)m_oscAddressSize; i++) {
        mOscAddress[i] = (char)in0(kVarOffset + i);
    }
    // terminate string
    mOscAddress[m_oscAddressSize] = 0;

    return kVarOffset + m_oscAddressSize;
}

int OSCMessenger::extractPortNumber() {
    return (int) in0(0);
}

void OSCMessenger::setupEndpoint(int portNumber) {
    asio::ip::udp::resolver resolver(mIoContext);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::resolver::query("127.0.0.1", std::to_string(portNumber)));
    mEndpoint = *endpoints.begin();
}

void OSCMessenger::allocBuffers() {
    Unit* unit = (Unit*) this;
    mBuffer = (char *)RTAlloc(mWorld, OUTPUT_BUFFER_SIZE);
    ClearUnitIfMemFailed(mBuffer);
}

void OSCMessenger::setupValues(int valueOffset) {
    mNumValues = mNumInputs - valueOffset;
}

void OSCMessenger::next_k(int nSamples) {
    OSCPP::Client::Packet packet(mBuffer, OUTPUT_BUFFER_SIZE);

    packet.openMessage(mOscAddress, mNumValues);

    for(int i=0; i<mNumValues; i++) {
        packet.float32(in0(i+mValueOffset));
    }
    packet.closeMessage();
    mSocket.send_to(asio::buffer(mBuffer, packet.size()), mEndpoint);

    out0(0) = 0.0;
}

OSCMessenger::~OSCMessenger() {
    delete[] mBuffer;
}

} // namespace OSCMessenger

PluginLoad(OSCMessengerUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<OSCMessenger::OSCMessenger>(ft, "OSCMessenger", false);
}
