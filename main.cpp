#include <iostream>
#include "tcp_server.h"

int main(int argc, char** argv) {

    TD::TcpServer serv{"0.0.0.0", 10085};
    std::cout<<"poll:"<<serv.getPollName()<<std::endl;

    serv.onConnect([](int fd, std::string ip, unsigned short port){
        std::cout<<"connected:"<<fd<<" accept "<<ip<<":"<<port<<std::endl;
    });
    serv.onMessage([](int fd, std::string msg){
        std::cout<<"message:"<<fd<<" len:"<<msg.size()<<" msg:"<<msg<<std::endl;
        ::write(fd, msg.data(), msg.size());
    });
    serv.onError([](int fd, int what){
        if(what & STATE_EOF) {
            std::cout<<"error:"<<fd<<" EOF"<<std::endl;
        } else if(what & STATE_ERROR) {
            std::cout<<"error:"<<fd<<" ERROR"<<std::endl;
        }
        ::close(fd);
        ::close(fd);
    });
    bool ret = serv.start();
    if(!ret) {
        std::cout<<"error:"<<serv.getLastError()<<std::endl;
    }
    return 0;
}
