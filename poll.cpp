#include "poll.h"

#ifdef __linux__
    #include "poll_epoll.h"
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
    #include "poll_kqueue.h"
#endif

namespace TD {

Poll* Poll::newPoll() {
#ifdef __linux__
    return new EpollPoll();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
    return new KqueuePoll();
#else
    return NULL;
#endif
}

}
