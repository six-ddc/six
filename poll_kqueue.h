#ifndef _POLL_KQUEUE_H_
#define _POLL_KQUEUE_H_

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <vector>

#include "event_loop.h"

namespace TD {

#define DEFUALT_SETSIZE 128

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
    virtual bool addEvent(int fd, int mask) override {
        struct kevent ke;
        if(mask & EVENT_READABLE) {
            EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
            if(kevent(kqfd, &ke, 1, NULL, 0, NULL) == -1) return false;
        }
        if(mask & EVENT_WRITABLE) {
            EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
            if(kevent(kqfd, &ke, 1, NULL, 0, NULL) == -1) return false;
        }
        return true;
    }
    virtual bool delEvent(int fd, int mask) override {
        struct kevent ke;
        if(mask & EVENT_READABLE) {
            EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
            kevent(kqfd, &ke, 1, NULL, 0, NULL);
        }
        if(mask & EVENT_WRITABLE) {
            EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
            kevent(kqfd, &ke, 1, NULL, 0, NULL);
        }
        return true;
    }
    virtual int  poll(struct timeval* tvp, std::function<void(int,int)> cb) override {
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
            int j = 0;
            for(j = 0; j < retval; j++) {
                int mask = EVENT_NONE;
                struct kevent *e = &kevents[j];
                if(e->filter == EVFILT_READ) mask |= EVENT_READABLE;
                if(e->filter == EVFILT_WRITE) mask |= EVENT_WRITABLE;
                if(mask != EVENT_NONE) {
                    cb(e->ident, mask);
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
