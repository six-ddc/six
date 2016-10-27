#ifndef _EVENT_H_
#define _EVENT_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>

#include <functional>
#include <tuple>

namespace TD {

class EventLoop;

#define EVENT_NONE      0x00
#define EVENT_READABLE  0x01
#define EVENT_WRITABLE  0x02
#define EVENT_CLOSABLE  0x04
#define EVENT_ERROR     0x08

#define STATE_READING   0x01
#define STATE_WRITING   0x02
#define STATE_EOF       0x04
#define STATE_ERROR     0x08
#define STATE_TIMEOUT   0x10

typedef std::function<void(int)>                        rw_callback;    // void rw_callback(int fd)
typedef std::tuple<int, int, rw_callback, rw_callback>  rw_event;       // fd, mask, readable_cb, writable_cb

bool listen(int sock, struct sockaddr *sa, socklen_t len, int backlog);
bool accept(int sock, int& accept_fd, std::string* ip, unsigned short* port);
bool setReuseAddr(int fd);
bool setKeepAlive(int fd);
bool setNonblocking(int fd);
bool setCloseOnExec(int fd);

}

#endif
