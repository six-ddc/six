#include "event_loop.h"
#include "poll.h"
#include "channel.h"

using TD::EventLoop;

EventLoop::EventLoop() : stopped(false), poll(Poll::newPoll()) {
}

EventLoop::~EventLoop() {
}

bool EventLoop::init() {
    return poll->init();
}

bool EventLoop::addChannel(Channel* ch) {
    channelList[ch->getFd()] = ch;
    return true;
}

bool EventLoop::delChannel(Channel* ch) {
    ch->disableAll();
    channelList.erase(ch->getFd());
    delete ch;
    return true;
}

void EventLoop::addEvent(Channel* ch, int mask) {
    poll->addEvent(ch, mask);
}

void EventLoop::delEvent(Channel* ch, int mask) {
    poll->delEvent(ch, mask);
}

int EventLoop::processEvents() {
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    std::vector<Channel*> fired;
    int numevents = poll->poll(&tv, fired);
    for(Channel* ch : fired) {
        ch->fireEventCallback();
    }
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
