#include "event_loop.h"
#include "poll.h"

using Six::EventLoop;

EventLoop::EventLoop() : stopped(false), poll(Poll::newPoll()) {
    wakeupFds = std::make_pair(0, 0);
}

EventLoop::~EventLoop() {
}

bool EventLoop::init() {
    int fds[2];

    if(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
        int eno = errno;
        std::cerr<<"socketpair error. "<<eno<<":"<<strerror(eno)<<std::endl;
        return false;
    }

    setNonblocking(fds[0]);
    setNonblocking(fds[1]);

    auto ch = std::make_shared<WakeupChannel>(shared_from_this(), fds[1]);
    ch->setReadCallback(std::bind(&EventLoop::workerReadableProc, this, std::placeholders::_1));
    ch->enableReading();
    addChannel(ch);

    wakeupFds = std::make_pair(fds[0], fds[1]);

    return poll->init();
}

bool EventLoop::addChannel(std::shared_ptr<Channel> ch) {
    channelList[ch->getFd()] = ch;
    return true;
}

bool EventLoop::delChannel(std::shared_ptr<Channel> ch) {
    ch->release();
    channelList.erase(ch->getFd());
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
            beforeProc(shared_from_this());
        }
        processEvents();
    }
}

const char* EventLoop::getPollName() {
    return poll->name();
}

void EventLoop::workerReadableProc(std::shared_ptr<Channel> c) {
    std::shared_ptr<WakeupChannel> ch = std::dynamic_pointer_cast<WakeupChannel>(c);
    int fd = ch->getFd();
    char buf[1] = {0};
    std::size_t len = ::read(fd, buf, 1);
    std::cout<<"recv..."<<buf[0]<<std::endl;
}
