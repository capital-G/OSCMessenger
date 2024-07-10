
#pragma once

#include "SC_PlugIn.hpp"
#include <oscpp/client.hpp>
#include <asio.hpp>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace OSCMessenger {

class OSCMessenger : public SCUnit {
public:
    OSCMessenger();

    // Destructor
    ~OSCMessenger();

private:
    // Calc function
    void next_k(int nSamples);
    void extractArgs();
    void setupEndpoint();
    void allocBuffers();
    void sendPackets();

    // Member variables
    char* mOscAddress;
    char* mDoneAddress;
    bool mSendDoneMessage;
    float mDoneValue;
    char* mBuffer;
    int mValueOffset;
    int mNumValues;
    int mPortNumber;

    OSCPP::Client::Packet mPacket;
    asio::io_context mIoContext;
    asio::ip::udp::socket mSocket;
    asio::ip::udp::endpoint mEndpoint;

    // thread
    std::thread mWorkerThread;
    std::queue<std::vector<char>> mPacketQueue;
    std::mutex mQueueMutex;
    std::condition_variable mQueueCondition;
    bool mStopWorker = false;
};

} // namespace OSCMessenger
