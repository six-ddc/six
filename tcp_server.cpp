#include "tcp_server.h"
#include "channel.h"

#include <cstring>
#include <cstdio>

using TD::TcpServer;
using TD::Channel;

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

    std::shared_ptr<Channel> ch = std::make_shared<Channel>(&loop, sock);
    ch->setReadCallback(std::bind(&TcpServer::acceptableProc, this, std::placeholders::_1));
    ch->enableReading();
    loop.addChannel(ch);

    std::cout<<"fd="<<sock<<" listening..."<<std::endl;

    loop.loop();

    return true;
}

void TcpServer::acceptableProc(std::shared_ptr<Channel> ch) {
    int newfd;
    std::string ip; unsigned short port;
    if(!TD::accept(ch->getFd(), newfd, &ip, &port)) {
        std::cout<<"accept error"<<std::endl;
        return;
    }
    TD::setNonblocking(newfd);
    std::shared_ptr<Channel> new_ch = std::make_shared<Channel>(&loop, newfd);
    new_ch->setReadCallback(std::bind(&TcpServer::readableProc, this, std::placeholders::_1));
    new_ch->setCloseCallback(std::bind(&TcpServer::closableProc, this, std::placeholders::_1));
    new_ch->enableReading();
    loop.addChannel(new_ch);
    if(connectProc) {
        connectProc(ch, std::move(ip), port);
    }
}

void TcpServer::closableProc(std::shared_ptr<Channel> ch) {
    int fd = ch->getFd();
    if(closeProc) closeProc(ch);
    loop.delChannel(ch);
    ::close(fd);
}

void TcpServer::readableProc(std::shared_ptr<Channel> ch) {
    int fd = ch->getFd();
    std::size_t len = ::read(fd, &buffer[0], buffer.size());
    int what = STATE_READING;
    if(len > 0) {
        if(messageProc) messageProc(ch, std::string(buffer.data(), len));
        if(len == buffer.size()) {
            readableProc(ch);
        }
    } else {
        if(len == 0) {
            what |= STATE_EOF;
            closableProc(ch);
        } else {
            if(errno == EINTR || errno == EAGAIN) {
                return;
            }
            what |= STATE_ERROR;
            if(errorProc) errorProc(ch, what);
            loop.delChannel(ch);
            ::close(fd);
        }
    }
}
