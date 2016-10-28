#include "util.h"

#include <string>
#include <iostream>

bool TD::listen(int sock, struct sockaddr *sa, socklen_t len, int backlog) {
    if (::bind(sock, sa, len) == -1) {
        return false;
    }

    if (::listen(sock, backlog) == -1) {
        return false;
    }
    return true;
}

bool TD::accept(int sock, int& fd, std::string* ip, unsigned short* port) {
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);
    while(1) {
        fd = ::accept(sock, reinterpret_cast<struct sockaddr*>(&sa), &salen);
        if (fd == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == ECONNABORTED)
                continue;
            else {
                return false;
            }
        }
        if(sa.ss_family == AF_INET) {
            auto s = reinterpret_cast<struct sockaddr_in*>(&sa);
            if (ip) {
                char ip_buf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(s->sin_addr), ip_buf, INET_ADDRSTRLEN);
                ip->assign(ip_buf);
            }
            if (port) *port = ntohs(s->sin_port);
        } else {
            auto s = reinterpret_cast<struct sockaddr_in6*>(&sa);
            if (ip) {
                char ip_buf[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET, &(s->sin6_addr), ip_buf, INET6_ADDRSTRLEN);
                ip->assign(ip_buf);
            }
            if (port) *port = ntohs(s->sin6_port);
        }
        break;
    }
    return true;
}

bool TD::setReuseAddr(int fd) {
    int opt = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
}

bool TD::setKeepAlive(int fd) {
    int opt = 1;
    return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) == 0;
}

bool TD::setNonblocking(int fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return false;
    }
    return true;
}

bool TD::setCloseOnExec(int fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFD, NULL)) < 0) {
        return false;
    }
    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
        return false;
    }
    return true;
}

