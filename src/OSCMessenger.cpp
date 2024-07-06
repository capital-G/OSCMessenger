#include "SC_PlugIn.hpp"
#include "OSCMessenger.hpp"

#include <oscpp/client.hpp>
#include <asio.hpp>

static InterfaceTable *ft;

#define OUTPUT_BUFFER_SIZE 4096

namespace OSCMessenger {

OSCMessenger::OSCMessenger() : mSocket(mIoContext, asio::ip::udp::v4()) {
    // you'll need to define unit in order to use ClearUnitIfMemFailed
    Unit* unit = (Unit*) this;
    mCalcFunc = make_calc_function<OSCMessenger, &OSCMessenger::next_k>();
    
    extractPortNumber();
    extractOscAddress();

    asio::ip::udp::resolver resolver(mIoContext);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::resolver::query("127.0.0.1", std::to_string(mPortNumber)));
    mEndpoint = *endpoints.begin();

    // realtime-alloc buffer in memory
    mBuffer = (char *)RTAlloc(mWorld, OUTPUT_BUFFER_SIZE);
    ClearUnitIfMemFailed(mBuffer);

    // calc one output sample
    next_k(1);
}

void OSCMessenger::extractOscAddress() {
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
}

void OSCMessenger::extractPortNumber() {
    mPortNumber = (int) in0(0);
}

void OSCMessenger::next_k(int nSamples) {
    OSCPP::Client::Packet packet(mBuffer, OUTPUT_BUFFER_SIZE);

    packet.openMessage(mOscAddress, 2)
        .float32(4.0)
        .float32(5.0)
    .closeMessage();

    mSocket.send_to(asio::buffer(mBuffer, packet.size()), mEndpoint);

    out0(0) = 4.5;
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
