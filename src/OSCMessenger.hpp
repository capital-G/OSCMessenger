
#pragma once

#include "SC_PlugIn.hpp"
#include <oscpp/client.hpp>
#include <asio.hpp>

namespace OSCMessenger {

class OSCMessenger : public SCUnit {
public:
    OSCMessenger();

    // Destructor
    ~OSCMessenger();

private:
    // Calc function
    void next_k(int nSamples);
    void extractOscAddress();
    void extractPortNumber();

    // Member variables
    char* mOscAddress;
    int mPortNumber;
    char* mBuffer;
    OSCPP::Client::Packet mPacket;
    asio::io_context mIoContext;
    asio::ip::udp::socket mSocket;
    asio::ip::udp::endpoint mEndpoint;
};

} // namespace OSCMessenger
