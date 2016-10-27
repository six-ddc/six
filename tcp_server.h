#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <string>

#include "event_loop.h"
#include "util.h"

namespace TD {

class TcpServer {
    typedef std::function<void(int, std::string, unsigned short port)> connect_handler;     // fd, ip, port
    typedef std::function<void(int, std::string)> message_handler;       // fd
    typedef std::function<void(int)> close_handler;  // fd
    typedef std::function<void(int, int)> error_handler;   // fd, STATE_**
public:
    TcpServer(const char* bindaddr, unsigned short port, int backlog = 1024, int af = AF_INET);

    bool start();
    void stop();

    void onConnect(connect_handler handler);
    void onMessage(message_handler handler);
    void onClose(close_handler handler);
    void onError(error_handler handler);

    std::string getLastError();
    const char* getPollName();

private:
    void acceptableProc(int fd);
    void readableProc(int fd, Channel* ch);
    void closableProc(int fd, Channel* ch);

protected:
    std::string     bindaddr;
    unsigned short  port;
    int             backlog;
    int             af;
    
    std::string     buffer;
    std::string     errmsg;

    connect_handler connectProc;
    message_handler messageProc;
    close_handler   closeProc;
    error_handler   errorProc;

    EventLoop       loop;
};

inline std::string TcpServer::getLastError() {
    return errmsg;
}

inline void TcpServer::stop() {
    loop.stop();
}

inline void TcpServer::onConnect(connect_handler handler) {
    connectProc = handler;
}

inline void TcpServer::onMessage(message_handler handler) {
    messageProc = handler;
}

inline void TcpServer::onClose(close_handler handler) {
    closeProc = handler;
}

inline void TcpServer::onError(error_handler handler) {
    errorProc = handler;
}

inline const char* TcpServer::getPollName() {
    return loop.getPollName();
}

}

#endif // _TCP_SERVER_H_
