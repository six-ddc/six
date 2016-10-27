#include "channel.h"
#include "event_loop.h"

using TD::Channel;

std::ostream& TD::operator<<(std::ostream& os, const Channel* ch) {
    os<<"{fd="<<ch->fd<<" mask="<<ch->mask<<"}";
    return os;
}

Channel::~Channel() {
    std::cout<<"~Channel "<<this<<std::endl;
}

void Channel::addEvent(int mask) {
    loop->addEvent(this, mask);
}

void Channel::delEvent(int mask) {
    loop->delEvent(this, mask);
}

void Channel::fireEventCallback() {
    if(firedMask & EVENT_CLOSABLE && closecb) {
        closecb(fd);
    }
    if(firedMask & EVENT_READABLE && readcb) {
        readcb(fd);
    }
    if(firedMask & EVENT_WRITABLE && writecb) {
        writecb(fd);
    }
}