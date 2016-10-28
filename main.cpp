#include <iostream>
#include "tcp_server.h"

int main(int argc, char** argv) {

    TD::TcpServer serv{"0.0.0.0", 10085};
    std::cout<<"poll:"<<serv.getPollName()<<std::endl;

    serv.onConnect([](std::shared_ptr<TD::Channel> ch, std::string ip, unsigned short port){
        std::cout<<"connected:"<<ch->getFd()<<" accept "<<ip<<":"<<port<<std::endl;
    });
    serv.onMessage([](std::shared_ptr<TD::Channel> ch, std::string msg){
        std::cout<<"message:"<<ch->getFd()<<" len:"<<msg.size()<<" msg:"<<msg<<std::endl;
        ::write(ch->getFd(), msg.data(), msg.size());
    });
    serv.onError([](std::shared_ptr<TD::Channel> ch, int what){
        std::cout<<"error:"<<ch->getFd()<<" ERROR"<<std::endl;
    });
    serv.onClose([](std::shared_ptr<TD::Channel> ch){
        std::cout<<"close:"<<ch->getFd()<<" EOF"<<std::endl;
    });
    bool ret = serv.start();
    if(!ret) {
        std::cout<<"error:"<<serv.getLastError()<<std::endl;
    }
    return 0;
}
