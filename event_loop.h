#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_

#include <vector>
#include <map>
#include <queue>
#include <memory>

#include "channel.h"
#include "alloc.h"
#include "util.h"

namespace Six {

class EventLoop;
class Poll;

class WakeupChannel : public Channel {
public:
    WakeupChannel(std::weak_ptr<EventLoop> lp, int fd_) : Channel(lp, fd_) {}

    void push(std::string&& data) {
        std::lock_guard<std::mutex> locker(queueMutex);
        queueData.push(data);
    }

    bool push(std::string& data) {
        std::lock_guard<std::mutex> locker(queueMutex);
        if(queueData.empty())
            return false;
        data = std::move(queueData.front());
        queueData.pop();
        return true;
    }

protected:
    std::queue<std::string> queueData;
    mutable std::mutex queueMutex;
};

class EventLoop : public std::enable_shared_from_this<EventLoop> {
    friend class Channel;
public:
    EventLoop();
    virtual ~EventLoop();

    void setEventBeforeProc(std::function<void(std::weak_ptr<EventLoop>)> proc);

    bool init();
    void stop();
    bool addChannel(std::shared_ptr<Channel> ch);
    bool delChannel(std::shared_ptr<Channel> ch);

    int  processEvents();
    void loop();
    const char* getPollName();
    std::size_t getChannelSize();

private:
    void addEvent(Channel* ch, int mask);
    void delEvent(Channel* ch, int mask);

    void workerReadableProc(std::shared_ptr<Channel> c);

protected:
    bool stopped;
    std::unique_ptr<Poll> poll;
    std::map<int, std::shared_ptr<Channel>> channelList;            // fd, channel
    std::function<void(std::weak_ptr<EventLoop>)> beforeProc;
    std::pair<int, int> wakeupFds;
};

// inline

inline void EventLoop::setEventBeforeProc(std::function<void(std::weak_ptr<EventLoop>)> proc) {
    beforeProc = proc;
}

inline void EventLoop::stop() {
    stopped = true;
}

inline std::size_t EventLoop::getChannelSize() {
    return channelList.size();
}

}

#endif // _EVENT_LOOP_H_
