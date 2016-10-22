#include "tcp_server.h"

#include <cstring>
#include <cstdio>

using TD::TcpServer;

#define DEFAULT_BUFFER_SIZE 128

TcpServer::TcpServer(const char* _bindaddr, unsigned short _port, int _backlog/* = 1024*/, int _af/* = AF_INET*/)
    : bindaddr(_bindaddr), port(_port), backlog(_backlog), af(_af) {
    buffer.resize(128);
}

bool TcpServer::start() {

    if(!loop.init()) {
        return false;
    }

    char _port[6];
    struct addrinfo hints, *servinfo;
    std::snprintf(_port, 6, "%d", port);
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int ret;
    if((ret = getaddrinfo(bindaddr.c_str(), _port, &hints, &servinfo)) != 0) {
        errmsg.assign(::gai_strerror(ret));
        return false;
    }
    int sock;
    for(struct addrinfo* p = servinfo; p != nullptr; p = p->ai_next) {
        if((sock = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        if(af == AF_INET6) {
            int opt = 1;
            if(::setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) == -1) {
                ::close(sock);
                freeaddrinfo(servinfo);
                errmsg.assign(::strerror(errno));
                return false;
            }
        }
        if(!(TD::setNonblocking(sock) && TD::setKeepAlive(sock) && TD::setReuseAddr(sock))) {
            ::close(sock);
            freeaddrinfo(servinfo);
            errmsg.assign(::strerror(errno));
            return false;
        }
        if(!TD::listen(sock, p->ai_addr, p->ai_addrlen, backlog)) {
            ::close(sock);
            freeaddrinfo(servinfo);
            errmsg.assign(::strerror(errno));
            return false;
        }
        break;
    }
    freeaddrinfo(servinfo);

    auto func = std::bind(&TcpServer::acceptableProc, this, std::placeholders::_1);
    if(!loop.addFileEvent(sock, EVENT_READABLE, func, nullptr)) {
        return false;
    }

    std::cout<<"fd="<<sock<<" listening..."<<std::endl;

    loop.loop();

    return true;
}

void TcpServer::acceptableProc(int fd) {
    int newfd;
    std::string ip; unsigned short port;
    if(!TD::accept(fd, newfd, &ip, &port)) {
        std::cout<<"accept error"<<std::endl;
        return;
    }
    if(connectProc) {
        connectProc(newfd, std::move(ip), port);
    }
    TD::setNonblocking(newfd);
    auto func = std::bind(&TcpServer::readableProc, this, std::placeholders::_1);
    if(!loop.addFileEvent(newfd, EVENT_READABLE, func, nullptr)) {
        std::cout<<"addFileEvent error"<<std::endl;
        return;
    }
}

void TcpServer::readableProc(int fd) {
    int len = ::read(fd, &buffer[0], buffer.size());
    int what = STATE_READING;
    if(len > 0) {
        if(messageProc) { messageProc(fd, std::string(buffer.data(), len));
        }
        if(len == buffer.size()) {
            readableProc(fd);
        }
    } else {
        if(len == 0) {
            what |= STATE_EOF;
        } else {
            if(errno == EINTR || errno == EAGAIN) {
                return;
            }
            what |= STATE_ERROR;
        }
        if(errorProc) {
            errorProc(fd, what);
        }
        loop.delFileEvent(fd, EVENT_READABLE);
    }
}
