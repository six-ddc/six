#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <iostream>
#include "util.h"

namespace TD {

class EventLoop;

class Channel : public std::enable_shared_from_this<Channel> {

    friend std::ostream& operator<<(std::ostream& os, const Channel* ch);
public:
    typedef std::function<void(std::shared_ptr<Channel> ch)> event_cb;

    Channel(std::weak_ptr<EventLoop> lp, int fd_) : loop(lp), fd(fd_), mask(EVENT_NONE), closed(false) {}
    virtual ~Channel();

    void setReadCallback(const event_cb& cb);
    void setWriteCallback(const event_cb& cb);
    void setCloseCallback(const event_cb& cb);
    void setErrorCallback(const event_cb& cb);

    void send(const std::string& msg);
    void send(const char* msg);

    void close();

    void fireEventCallback();
    
    int getMask();
    int getFd();

    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();

    void setFiredMask(int d);

    void release();

private:
    void delEvent(int mask);
    void addEvent(int mask);

    std::shared_ptr<Channel> getShared();

protected:
    event_cb readcb;
    event_cb writecb;
    event_cb closecb;
    event_cb errorcb;
    std::weak_ptr<EventLoop> loop;
    int fd;
    int mask;
    int firedMask;      // 当前循环触发的事件
    int closed;
};

std::ostream& operator<<(std::ostream& os, const Channel* ch);

inline std::shared_ptr<Channel> Channel::getShared() {
    return shared_from_this();
}

inline void Channel::setReadCallback(const event_cb& cb) {
    readcb = cb;
}

inline void Channel::setWriteCallback(const event_cb& cb) {
    writecb = cb;
}

inline void Channel::setCloseCallback(const event_cb& cb) {
    closecb = cb;
}

inline void Channel::setErrorCallback(const event_cb& cb) {
    errorcb = cb;
}

inline int Channel::getMask() {
    return mask;
}

inline int Channel::getFd() {
    return fd;
}

inline void Channel::enableReading() {
    if(!(mask & EVENT_READABLE)) {
        mask |= EVENT_READABLE;
        addEvent(EVENT_READABLE);
    }
}

inline void Channel::disableReading() {
    if(mask & EVENT_READABLE) {
        mask &= ~EVENT_READABLE;
        delEvent(EVENT_READABLE);
    }
}

inline void Channel::enableWriting() {
    if(!(mask & EVENT_WRITABLE)) {
        mask |= EVENT_WRITABLE;
        delEvent(EVENT_WRITABLE);
    }
}

inline void Channel::disableWriting() {
    if(mask & EVENT_WRITABLE) {
        mask &= ~EVENT_WRITABLE;
        delEvent(EVENT_WRITABLE);
    }
}

inline void Channel::close() {
    if(!closed) {
        ::close(fd);
        closed = true;
        // fd 不能置0
    }
}

inline void Channel::disableAll() {
    disableReading();
    disableWriting();
}

inline void Channel::setFiredMask(int d) {
    firedMask = d;
}

}

#endif // _CHANNEL_H_
