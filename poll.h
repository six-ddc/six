#ifndef _POLL_H_
#define _POLL_H_

#include <iostream>
#include <functional>
#include <sys/time.h>

#include "util.h"

namespace TD {

class EventLoop;

class Poll {
public:
    virtual ~Poll() {}

    static Poll* newPoll();

    virtual bool init() = 0;
    virtual bool resize(std::size_t setsize) = 0;
    virtual bool addEvent(int fd, int mask) = 0;
    virtual bool delEvent(int fd, int mask) = 0;
    virtual int  poll(struct timeval* tvp, std::function<void(int,int)> cb) = 0;
    virtual const char* name() = 0;
    
protected:
    Poll() {}
};

}

#endif // _POLL_H_
