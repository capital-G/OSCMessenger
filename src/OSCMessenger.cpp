#include "SC_PlugIn.hpp"
#include "OSCMessenger.hpp"

#include <oscpp/client.hpp>
#include <asio.hpp>

static InterfaceTable *ft;

#define OUTPUT_BUFFER_SIZE 4096

namespace OSCMessenger {

OSCMessenger::OSCMessenger() : mSocket(mIoContext, asio::ip::udp::v4()), mOscAddress("/myOscSender") {
    // you'll need to define unit in order to use ClearUnitIfMemFailed
    Unit* unit = (Unit*) this;
    mCalcFunc = make_calc_function<OSCMessenger, &OSCMessenger::next_k>();

    asio::ip::udp::resolver resolver(mIoContext);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::resolver::query("127.0.0.1", "5553"));
    mEndpoint = *endpoints.begin();

    // realtime-alloc buffer in memory
    mBuffer = (char *)RTAlloc(mWorld, OUTPUT_BUFFER_SIZE);
    ClearUnitIfMemFailed(mBuffer);

    // calc one output sample
    next_k(1);
}

void OSCMessenger::next_k(int nSamples) {
    OSCPP::Client::Packet packet(mBuffer, OUTPUT_BUFFER_SIZE);

    packet.openMessage("/my_message", 2)
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
