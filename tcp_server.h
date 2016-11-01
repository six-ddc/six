#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <iostream>
#include <string>

#include "event_loop.h"
#include "util.h"
#include "channel.h"

namespace TD {

template <typename ChType = Channel>
class TcpServer {
    typedef std::function<void(std::shared_ptr<ChType> ch, std::string ip, unsigned short port)> connect_handler;
    typedef std::function<void(std::shared_ptr<ChType> ch, std::string msg)> message_handler;
    typedef std::function<void(std::shared_ptr<ChType> ch)> close_handler;
    typedef std::function<void(std::shared_ptr<ChType> ch, int state)> error_handler;
public:
    typedef ChType Session;

    TcpServer(const char* bindaddr, unsigned short port, int backlog = 1024, int af = AF_INET);

    bool start();
    void stop();

    void onConnect(connect_handler handler);
    void onMessage(message_handler handler);
    void onClose(close_handler handler);
    void onError(error_handler handler);

    std::string getLastError();

    const char* getPollName();
    std::size_t getChannelSize();

private:
    void acceptableProc(std::shared_ptr<ChType> ch);
    void readableProc(std::shared_ptr<ChType> ch);
    void closableProc(std::shared_ptr<ChType> ch);
    void errorProc(std::shared_ptr<ChType> ch);

protected:
    std::string     bindaddr;
    unsigned short  port;
    int             backlog;
    int             af;
    
    std::string     buffer;
    std::string     errmsg;

    connect_handler connectCallback;
    message_handler messageCallback;
    close_handler   closeCallback;
    error_handler   errorCallback;

    EventLoop       loop;
};

typedef TcpServer<> SampleTcpServer;

template <typename ChType>
inline std::string TcpServer<ChType>::getLastError() {
    return errmsg;
}

template <typename ChType>
inline void TcpServer<ChType>::stop() {
    loop.stop();
}

template <typename ChType>
inline void TcpServer<ChType>::onConnect(connect_handler handler) {
    connectCallback = handler;
}

template <typename ChType>
inline void TcpServer<ChType>::onMessage(message_handler handler) {
    messageCallback = handler;
}

template <typename ChType>
inline void TcpServer<ChType>::onClose(close_handler handler) {
    closeCallback = handler;
}

template <typename ChType>
inline void TcpServer<ChType>::onError(error_handler handler) {
    errorCallback = handler;
}

template <typename ChType>
inline const char* TcpServer<ChType>::getPollName() {
    return loop.getPollName();
}

template <typename ChType>
inline std::size_t TcpServer<ChType>::getChannelSize() {
    return loop.getChannelSize();
}

template <typename ChType>
TcpServer<ChType>::TcpServer(const char* _bindaddr, unsigned short _port, int _backlog/* = 1024*/, int _af/* = AF_INET*/)
        : bindaddr(_bindaddr), port(_port), backlog(_backlog), af(_af) {
    buffer.resize(128);
}

template <typename ChType>
bool TcpServer<ChType>::start() {

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

    std::shared_ptr<ChType> ch = std::make_shared<ChType>(&loop, sock);
    ch->setReadCallback(std::bind(&TcpServer<ChType>::acceptableProc, this, std::placeholders::_1));
    ch->enableReading();
    loop.addChannel(ch);

    std::cout<<"fd="<<sock<<" listening..."<<std::endl;

    loop.loop();

    return true;
}

template <typename ChType>
void TcpServer<ChType>::acceptableProc(std::shared_ptr<ChType> ch) {
    int newfd;
    std::string ip; unsigned short port;
    if(!TD::accept(ch->getFd(), newfd, &ip, &port)) {
        std::cout<<"accept error"<<std::endl;
        return;
    }
    TD::setNonblocking(newfd);
    std::shared_ptr<ChType> new_ch = std::make_shared<ChType>(&loop, newfd);
    new_ch->setReadCallback(std::bind(&TcpServer<ChType>::readableProc, this, std::placeholders::_1));
    new_ch->setCloseCallback(std::bind(&TcpServer<ChType>::closableProc, this, std::placeholders::_1));
    new_ch->enableReading();
    loop.addChannel(new_ch);
    if(connectCallback) {
        connectCallback(ch, std::move(ip), port);
    }
}

template <typename ChType>
void TcpServer<ChType>::closableProc(std::shared_ptr<ChType> ch) {
    int fd = ch->getFd();
    if(closeCallback) closeCallback(ch);
    loop.delChannel(ch);
}

template <typename ChType>
void TcpServer<ChType>::readableProc(std::shared_ptr<ChType> ch) {
    int fd = ch->getFd();
    std::size_t len = ::read(fd, &buffer[0], buffer.size());
    int what = STATE_READING;
    if(len > 0) {
        if(messageCallback) messageCallback(ch, std::string(buffer.data(), len));
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
            if(errorCallback) errorCallback(ch, what);
            loop.delChannel(ch);
        }
    }
}

}

#endif // _TCP_SERVER_H_
