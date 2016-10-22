#include "poll.h"

#ifdef __linux__
    #include "poll_epoll.h"
#else
    #if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
        #include "poll_kqueue.h"
    #else
        #include "poll_select.h"
    #endif
#endif

namespace TD {

Poll* Poll::newPoll() {
#ifdef __linux__
    return new EpollPoll();
#else
    #if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
        return new KqueuePoll();
    #else
        return new SelectPoll();
    #endif
#endif
}

}
