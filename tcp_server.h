#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <iostream>
#include <string>
#include <thread>

#include "event_loop.h"
#include "util.h"
#include "channel.h"

namespace Six {

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

    void setThreadNum(int num);

private:
    void acceptableProc(std::shared_ptr<ChType> ch);
    void readableProc(std::shared_ptr<ChType> ch);
    void closableProc(std::shared_ptr<ChType> ch);
    void errorProc(std::shared_ptr<ChType> ch);

    void workerReadableProc(std::shared_ptr<ChType> ch, int threadIdx);
    void wakeupWorker(int threadIdx);

    bool startListenThread();
    bool startWorkerThread();

    void workerInitCallback(std::shared_ptr<EventLoop> loop, int idx);
    std::shared_ptr<EventLoop> getNextLoop();

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

    int             listenFd;
    std::shared_ptr<EventLoop> listenLoop;

    int             threadNum;
    int             nextIdx;
    std::vector<std::shared_ptr<EventLoop>>   loops;
    std::vector<std::pair<std::shared_ptr<ChType>, int>> notifyPair;
    std::vector<std::shared_ptr<std::thread>> threads;

};

typedef TcpServer<> SampleTcpServer;

template <typename ChType>
inline std::string TcpServer<ChType>::getLastError() {
    return errmsg;
}

template <typename ChType>
inline void TcpServer<ChType>::stop() {
    // TODO listenLoop.stop();
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
    return listenLoop->getPollName();
}

template <typename ChType>
inline std::size_t TcpServer<ChType>::getChannelSize() {
    return listenLoop->getChannelSize();
}

}

#include "tcp_server.cpp"

#endif // _TCP_SERVER_H_
