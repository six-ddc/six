#include <iostream>
#include "tcp_server.h"

using namespace TD;

int main(int argc, char** argv) {

    SampleTcpServer serv{"0.0.0.0", 10085};
    // std::cout<<"poll:"<<serv.getPollName()<<std::endl;

    serv.onConnect([](std::shared_ptr<SampleTcpServer::Session> ch, std::string ip, unsigned short port){
        std::cout<<"connected:"<<ch->getFd()<<" accept "<<ip<<":"<<port<<std::endl;
    });
    serv.onMessage([&serv](std::shared_ptr<SampleTcpServer::Session> ch, std::string msg){
        std::cout<<"message:"<<ch->getFd()<<" channelSize:"<<serv.getChannelSize()<<" msg:"<<msg<<std::endl;
        ch->send(msg);
    });
    serv.onError([](std::shared_ptr<SampleTcpServer::Session> ch, int what){
        std::cout<<"error:"<<ch->getFd()<<" ERROR"<<std::endl;
    });
    serv.onClose([](std::shared_ptr<SampleTcpServer::Session> ch){
        std::cout<<"close:"<<ch->getFd()<<" EOF"<<std::endl;
    });
    serv.setThreadNum(5);
    bool ret = serv.start();
    if(!ret) {
        std::cout<<"error:"<<serv.getLastError()<<std::endl;
    }
    return 0;
}
