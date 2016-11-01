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
    if((firedMask & EVENT_CLOSABLE) && closecb) {
        closecb(getShared());
    }
    if((firedMask & EVENT_READABLE) && readcb) {
        readcb(getShared());
    }
    if((firedMask & EVENT_WRITABLE) && writecb) {
        writecb(getShared());
    }
    if((firedMask & EVENT_ERROR) && errorcb) {
        errorcb(getShared());
    }
}

void Channel::release() {
    disableAll();
    close();
}

void Channel::send(const std::string& msg) {
    ::write(fd, msg.data(), msg.size());
}

void Channel::send(const char* msg) {
    ::write(fd, msg, std::strlen(msg));
}
