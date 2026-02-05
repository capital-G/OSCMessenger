
#pragma once

#include "SC_PlugIn.hpp"
#include <oscpp/client.hpp>
#include <asio.hpp>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>



namespace OSCMessenger {

/*! @class SharedInfo
 *  @brief RT owned data that gets shared with NRT thread.
 *
 *  @discussion Since this gets shared between threads and the RT thread
 *  can remove a unit any time, we shift some data into SharedInfo
 *  to wrap it with a refCounter.
 *  This allows us to delay any deletion until no one needs
 *  access to the data anymore.
 */
class SharedInfo {
public:
    int mRefCount = 0;
    char* mOscAddress = nullptr;
    char* mDoneAddress = nullptr;
    char* mHostAddress = nullptr;
    int mPortNumber = 0;
    int mNodeId = 0;
    bool mAppendNodeId = false;

    World* mWorld = nullptr;

    std::atomic<bool> mEndpointReady = false;
    asio::ip::udp::endpoint mEndpoint;

    /*! @brief frees all resources - only call this in RT environments! */
    void free();
};

/*! @brief Passed from RT to NRT thread.
 *  All resources are RT managed!
 *  mValues should be freed if message is not needed anymore. */
struct Message {
    float* mValues = nullptr;
    int mNumValues = 0;
    SharedInfo* mSharedInfo = nullptr;
};

struct MessageTuple {
    std::vector<char> buffer;
    asio::ip::udp::endpoint endpoint;
};

class OSCMessenger : public SCUnit {
public:
    OSCMessenger();
    ~OSCMessenger();

private:
    void next_k(int nSamples);
    /*! @brief turns signal into RT string. If RT fails,
     *  this will return a nullptr */
    char* extractString(int sizeIndex, int startIndex) const;

    /*! @brief will send out an async message. Will also
     *  take care of freeing the message and also take care of
     *  the lifetime of the sending resources.
     *  Only call this from RT thread. */
    void sendMessage(Message* message, bool isDoneMessage);

    SharedInfo* mSharedInfo;
    bool mSendDoneMessage;
    float mDoneValue;
    int mValueOffset;
    int mNumValues;
};

} // namespace OSCMessenger
