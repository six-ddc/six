#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_

#include <vector>
#include <map>
#include <memory>

#include "alloc.h"
#include "util.h"

namespace TD {

class EventLoop;
class Poll;

class EventLoop {

    typedef std::pair<int, int>         fired_event;    // fd, mask
public:
    EventLoop();
    virtual ~EventLoop();

    void setEventBeforeProc(std::function<void(EventLoop*)> proc);

    bool init();
    void stop();
    bool addFileEvent(int fd, int mask, rw_callback rcall, rw_callback wcall);
    bool delFileEvent(int fd, int mask);
    int  getFileEventMask(int fd);
    int  processEvents();
    void loop();
    const char* getPollName();

private:
    void eventProc(int fd, int mask);

protected:
    bool        stopped;
    std::unique_ptr<Poll> poll;

    std::function<void(EventLoop*)> beforeProc;

    std::map<int, rw_event> events;
};

// inline

inline void EventLoop::setEventBeforeProc(std::function<void(EventLoop*)> proc) {
    beforeProc = proc;
}

inline void EventLoop::stop() {
    stopped = true;
}

}

#endif // _EVENT_LOOP_H_
