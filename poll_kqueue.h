#ifndef _POLL_KQUEUE_H_
#define _POLL_KQUEUE_H_

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <vector>

#include "event_loop.h"
#include "channel.h"

namespace TD {

#define DEFUALT_SETSIZE 128

#define kqNew 0
#define kqAdded 1
#define kqDeleted 2

std::ostream& operator<<(std::ostream& os, const struct kevent* ke) {
    os<<"{ident="<<ke->ident<<" filter="<<ke->filter<<" flags="<<ke->flags<<" fflags="<<ke->fflags<<" udata="<<ke->udata<<"}";
    return os;
}

class KqueuePoll : public Poll {
public:
    KqueuePoll() {}
    virtual ~KqueuePoll() {
        ::close(kqfd);
    }
    virtual bool init() override {
        kevents.resize(DEFUALT_SETSIZE);
        kqfd = ::kqueue();
        return kqfd != -1;
    }
    virtual bool resize(std::size_t setsize) override {
        kevents.resize(setsize);
        return true;
    }
    virtual bool addEvent(Channel* ch, int mask) override {
        std::cout<<"addEvent "<<this<<std::endl;
        struct kevent ke;
        int fd = ch->getFd();
        if(mask & EVENT_READABLE) {
            EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, ch);
            if(kevent(kqfd, &ke, 1, NULL, 0, NULL) == -1) return false;
        }
        if(mask & EVENT_WRITABLE) {
            EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, ch);
            if(kevent(kqfd, &ke, 1, NULL, 0, NULL) == -1) return false;
        }
        return true;
    }
    virtual bool delEvent(Channel* ch, int mask) override {
        std::cout<<"delEvent "<<this<<std::endl;
        struct kevent ke;
        int fd = ch->getFd();
        if(mask & EVENT_READABLE) {
            EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, ch);
            if(kevent(kqfd, &ke, 1, NULL, 0, NULL) == -1) return false;
        }
        if(mask & EVENT_WRITABLE) {
            EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, ch);
            if(kevent(kqfd, &ke, 1, NULL, 0, NULL) == -1) return false;
        }
        return true;
    }
    virtual int  poll(struct timeval* tvp, std::vector<Channel*>& fired) override {
        int eventnum = 0;
        int retval = 0;
        if(tvp) {
            struct timespec timeout;
            timeout.tv_sec = tvp->tv_sec;
            timeout.tv_nsec = tvp->tv_usec * 1000;
            retval = kevent(kqfd, NULL, 0, &kevents[0], kevents.size(), &timeout);
        } else {
            retval = kevent(kqfd, NULL, 0, &kevents[0], kevents.size(), NULL);
        }
        if(retval > 0) {
            fired.reserve(retval);
            for(int j = 0; j < retval; j++) {
                int mask = EVENT_NONE;
                struct kevent *e = &kevents[j];
                std::cout<<"POLL EVENT:"<<e<<std::endl;
                if(e->filter == EVFILT_READ) {
                    if(e->flags & EV_EOF)                   // 读到EOF则不回调可读函数
                        mask |= EVENT_CLOSABLE;
                    else
                        mask |= EVENT_READABLE;
                }
                if(e->filter == EVFILT_WRITE) mask |= EVENT_WRITABLE;
                if(mask != EVENT_NONE) {
                    Channel* ch = reinterpret_cast<Channel*>(e->udata);
                    ch->setFiredMask(mask);
                    fired.push_back(ch);
                    ++eventnum;
                }
            }
            if(retval == kevents.size()) {
                resize(kevents.size() * 2);
            }
        } else if(retval < 0) {
            std::cout<<"poll error="<<errno<<"--"<<strerror(errno)<<std::endl;
        }
        return eventnum;
    }
    virtual const char* name() override {
        return "kqueue";
    }
    
private:
    int kqfd;
    std::vector<struct kevent> kevents;
};

}

#endif // _POLL_KQUEUE_H_
