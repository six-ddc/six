#ifndef _POLL_H_
#define _POLL_H_

#include <iostream>
#include <vector>
#include <sys/time.h>

#include "util.h"

namespace TD {

class EventLoop;
class Channel;

class Poll {
public:
    virtual ~Poll() {}

    static Poll* newPoll();

    virtual bool init() = 0;
    virtual bool resize(std::size_t setsize) = 0;
    virtual bool addEvent(Channel* ch, int mask) = 0;
    virtual bool delEvent(Channel* ch, int mask) = 0;
    virtual int  poll(struct timeval* tvp, std::vector<Channel*>& fired) = 0;
    virtual const char* name() = 0;
    
protected:
    Poll() {}
};

}

#endif // _POLL_H_
