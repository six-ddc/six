#include "event_loop.h"
#include "Poll.h"

using TD::EventLoop;

EventLoop::EventLoop() : stopped(false), poll(Poll::newPoll()) {
}

EventLoop::~EventLoop() {
}

bool EventLoop::init() {
    return poll->init();
}

bool EventLoop::addFileEvent(int fd, int mask, rw_callback rcall, rw_callback wcall) {
    if(!poll->addEvent(fd, mask)) {
        return false;
    }
    auto iter = events.find(fd);
    if(iter != events.end()) {
        int& omask = std::get<1>(iter->second);
        omask |= mask;
        std::get<2>(iter->second) = rcall;
        std::get<3>(iter->second) = wcall;
    } else {
        events[fd] = std::make_tuple(fd, mask, rcall, wcall);
    }
    return true;
}

bool EventLoop::delFileEvent(int fd, int mask) {
    auto iter = events.find(fd);
    if(iter != events.end()) {
        int& omask = std::get<1>(iter->second);
        omask &= (~mask);
        if(omask == EVENT_NONE) {
            events.erase(fd);
        }
    }

    poll->delEvent(fd, mask);
    return true;
}

int EventLoop::getFileEventMask(int fd) {
    return std::get<1>(events[fd]);
}

void EventLoop::eventProc(int fd, int mask) {
    rw_callback rcall, wcall;
    std::tie(std::ignore, std::ignore, rcall, wcall) = events[fd];
    if(mask & EVENT_READABLE && rcall) {
        rcall(fd);
    }
    if(mask & EVENT_WRITABLE && wcall) {
        wcall(fd);
    }
}

int EventLoop::processEvents() {
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int numevents = poll->poll(&tv, std::bind(&EventLoop::eventProc, this, std::placeholders::_1, std::placeholders::_2));
    return numevents;
}

void EventLoop::loop() {
    stopped = false;
    while(!stopped) {
        if(beforeProc) {
            beforeProc(this);
        }
        processEvents();
    }
}

const char* EventLoop::getPollName() {
    return poll->name();
}
