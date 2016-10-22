#ifndef _POLL_SELECT_H_
#define _POLL_SELECT_H_

#include <set>
#include <map>
#include <cstdio>
#include <unistd.h>
#include "event_loop.h"

namespace TD {

class SelectPoll : public Poll {
public:
    SelectPoll() {}
    virtual ~SelectPoll() {
    }
    virtual bool init() override {
        FD_ZERO(&rfds);   
        FD_ZERO(&wfds);   
        return true;
    }
    virtual bool resize(std::size_t setsize) override {
        return setsize < FD_SETSIZE;
    }
    virtual bool addEvent(int fd, int mask) override {
        if(fd >= FD_SETSIZE) return false;
        if(mask == EVENT_NONE) return false;
        if(mask & EVENT_READABLE) FD_SET(fd, &rfds);
        if(mask & EVENT_WRITABLE) FD_SET(fd, &wfds);
        maxfds.insert(fd);
        int& omask = masks[fd];
        omask |= mask;
        return true;
    }
    virtual bool delEvent(int fd, int mask) override {
        auto iter = masks.find(fd);
        if(iter == masks.end()) {
            return false;
        }
        if(mask & EVENT_READABLE) FD_CLR(fd, &rfds);
        if(mask & EVENT_WRITABLE) FD_CLR(fd, &wfds);
        // 更新最大fd
        int& omask = iter->second;
        omask &= (~mask);
        if(omask == EVENT_NONE) {
            masks.erase(iter);
            maxfds.erase(fd);
        }

        return true;
    }
    virtual int  poll(struct timeval* tvp, std::function<void(int,int)> cb) override {
        int eventnum = 0;
        /* We need to have a copy of the fd sets as it's not safe to reuse
         * FD sets after select(). */
        std::memcpy(&_rfds, &rfds, sizeof(fd_set));
        std::memcpy(&_wfds, &wfds, sizeof(fd_set));

        int maxfd = -1;
        auto iter = maxfds.rbegin();
        if(iter != maxfds.rend()) {
            maxfd = *iter;
        }
        int retval = ::select(maxfd + 1, &_rfds, &_wfds, NULL, tvp);
        if(retval > 0) {
            auto _backMasks(masks);         // 可能内部删除导致迭代器失效，需要备份数据
            for(auto iter : _backMasks) {
                int fd = iter.first;
                int evmask = iter.second;
                int mask = EVENT_NONE;
                if(evmask & EVENT_READABLE && FD_ISSET(fd, &_rfds)) mask |= EVENT_READABLE;
                if(evmask & EVENT_WRITABLE && FD_ISSET(fd, &_wfds)) mask |= EVENT_WRITABLE;
                if(mask != EVENT_NONE) {
                    cb(fd, mask);
                    ++eventnum;
                }
            }
        } else if(retval < 0) {
            std::cout<<"poll maxfd="<<maxfd<<"error="<<errno<<"--"<<strerror(errno)<<std::endl;
        }

        return eventnum;
    }
    virtual const char* name() override {
        return "select";
    }

private:
    fd_set rfds, wfds;
    fd_set _rfds, _wfds;
    std::map<int, int> masks;       // pair(fd, mask)
    std::set<int>      maxfds;      // fd
};

}

#endif // _POLL_SELECT_H_
