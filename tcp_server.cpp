#ifndef _TCP_SERVER_IMPL_H_
#define _TCP_SERVER_IMPL_H_

#include <iostream>
#include <string>
#include <thread>

#include "event_loop.h"
#include "util.h"
#include "channel.h"

namespace TD {

template <typename ChType>
TcpServer<ChType>::TcpServer(const char* _bindaddr, unsigned short _port, int _backlog/* = 1024*/, int _af/* = AF_INET*/)
        : bindaddr(_bindaddr), port(_port), backlog(_backlog), af(_af), threadNum(0), nextIdx(0) {
    buffer.resize(1024);
    listenLoop = std::make_shared<EventLoop>();
}

template <typename ChType>
bool TcpServer<ChType>::start() {
    bool reval = true;
    if(!(reval = startListenThread())) return false;
    if(!(reval = startWorkerThread())) return false;

    std::cout<<"fd="<<listenFd<<" listening..."<<std::endl;

    listenLoop->loop();
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
    auto new_ch = std::make_shared<ChType>(listenLoop, newfd);
    new_ch->setReadCallback(std::bind(&TcpServer<ChType>::readableProc, this, std::placeholders::_1));
    new_ch->setCloseCallback(std::bind(&TcpServer<ChType>::closableProc, this, std::placeholders::_1));
    new_ch->enableReading();
    listenLoop->addChannel(new_ch);
    if(connectCallback) {
        connectCallback(ch, std::move(ip), port);
    }
    wakeupWorker(0);
}

template <typename ChType>
void TcpServer<ChType>::closableProc(std::shared_ptr<ChType> ch) {
    int fd = ch->getFd();
    if(closeCallback) closeCallback(ch);
    listenLoop->delChannel(ch);
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
            listenLoop->delChannel(ch);
        }
    }
}

template <typename ChType>
void TcpServer<ChType>::setThreadNum(int num) {
    threadNum = num > 0 ? num : 0;
}

template <typename ChType>
bool TcpServer<ChType>::startListenThread() {

    if(!listenLoop->init()) {
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
    for(struct addrinfo* p = servinfo; p != nullptr; p = p->ai_next) {
        if((listenFd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        if(af == AF_INET6) {
            int opt = 1;
            if(::setsockopt(listenFd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) == -1) {
                int eno = errno;
                ::close(listenFd);
                freeaddrinfo(servinfo);
                errmsg.assign(::strerror(eno));
                return false;
            }
        }
        if(!(TD::setNonblocking(listenFd) && TD::setKeepAlive(listenFd) && TD::setReuseAddr(listenFd))) {
            int eno = errno;
            ::close(listenFd);
            freeaddrinfo(servinfo);
            errmsg.assign(::strerror(eno));
            return false;
        }
        if(!TD::listen(listenFd, p->ai_addr, p->ai_addrlen, backlog)) {
            int eno = errno;
            ::close(listenFd);
            freeaddrinfo(servinfo);
            errmsg.assign(::strerror(eno));
            return false;
        }
        break;
    }
    freeaddrinfo(servinfo);

    auto ch = std::make_shared<ChType>(listenLoop, listenFd);
    ch->setReadCallback(std::bind(&TcpServer<ChType>::acceptableProc, this, std::placeholders::_1));
    ch->enableReading();
    listenLoop->addChannel(ch);

    return true;
}

template <typename ChType>
bool TcpServer<ChType>::startWorkerThread() {
    if(!threadNum) return true;

    for(int i = 0; i < threadNum; ++i) {
        auto loop = std::make_shared<EventLoop>();
        if(!loop->init()) break;

        int notifyFd[2];

        if(socketpair(AF_UNIX, SOCK_STREAM, 0, notifyFd) == -1) {
            int eno = errno;
            std::cerr<<"socketpair error. "<<eno<<":"<<strerror(eno)<<std::endl;
            return false;
        }
        setNonblocking(notifyFd[0]);
        setNonblocking(notifyFd[1]);

        auto ch1 = std::make_shared<ChType>(loop, notifyFd[1]);
        ch1->setReadCallback(std::bind(&TcpServer<ChType>::workerReadableProc, this, std::placeholders::_1, i));
        ch1->enableReading();
        loop->addChannel(ch1);

        notifyPair.push_back(std::make_pair(ch1, notifyFd[0]));

        auto th = std::make_shared<std::thread>([this, loop, i]{
            this->workerInitCallback(loop, i);
        });

        loops.push_back(loop);
        threads.push_back(th);
    }

    return loops.size() == threadNum;
}

template <typename ChType>
void TcpServer<ChType>::workerInitCallback(std::shared_ptr<EventLoop> loop, int idx) {
    std::cout<<"start worker thread ["<<idx<<"] thread_id "<<std::this_thread::get_id()<<std::endl;
    loop->loop();
}

template <typename ChType>
std::shared_ptr<EventLoop> TcpServer<ChType>::getNextLoop() {
    if(loops.size() == 0) return listenLoop;

    auto loop = loops[nextIdx];
    nextIdx = (++nextIdx) % loops.size();
    return loop;
}

template <typename ChType>
void TcpServer<ChType>::workerReadableProc(std::shared_ptr<ChType> ch, int threadIdx) {
    int fd = ch->getFd();
    char buf[1] = {0};
    std::size_t len = ::read(fd, buf, 1);
    std::cout<<"recv..."<<buf[0]<<std::endl;
}

template <typename ChType>
void TcpServer<ChType>::wakeupWorker(int threadIdx) {
    int fd = notifyPair[threadIdx].second;
    char buf[1] = {'w'};
    ::write(fd, buf, 1);
}

}

#endif // _TCP_SERVER_IMPL_H_
