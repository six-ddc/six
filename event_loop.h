#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_

#include <vector>
#include <map>
#include <memory>

#include "alloc.h"
#include "util.h"

namespace Six {

class EventLoop;
class Poll;
class Channel;

class EventLoop {
    friend class Channel;
public:
    EventLoop();
    virtual ~EventLoop();

    void setEventBeforeProc(std::function<void(EventLoop*)> proc);

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

protected:
    bool stopped;
    std::unique_ptr<Poll> poll;
    std::map<int, std::shared_ptr<Channel>> channelList;            // fd, channel
    std::function<void(EventLoop*)> beforeProc;
};

// inline

inline void EventLoop::setEventBeforeProc(std::function<void(EventLoop*)> proc) {
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
