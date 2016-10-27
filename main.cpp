#include <iostream>
#include "tcp_server.h"

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <initializer_list>

void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::nullptr_t v) {
    writer.Null();
}

void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, bool v) {
    writer.Bool(v);
}

void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, int32_t v) {
    writer.Int(v);
}

void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, uint32_t v) {
    writer.Uint(v);
}

void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, int64_t v) {
    writer.Int64(v);
}

void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, uint64_t v) {
    writer.Uint64(v);
}

void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, double v) {
    writer.Double(v);
}

void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, const char* v) {
    writer.String(v);
}

void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::string& v) {
    writer.String(v.data(), v.size());
}

template<typename T>
void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::initializer_list<T>& v) {
    writer.StartArray();
    for(auto& i :  v) {
        _JsonWriter(writer, i);
    }
    writer.EndArray();
}

template<typename T>
void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::vector<T>& v) {
    writer.StartArray();
    for(auto& i :  v) {
        _JsonWriter(writer, i);
    }
    writer.EndArray();
}

template<typename T, typename K>
void _JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::map<T, K>& v) {
    writer.StartObject();
    for(auto& i :  v) {
        writer.Key(i.first);
        _JsonWriter(writer, i.second);
    }
    writer.EndObject();
}

void JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer) {
}

template <typename T, typename... Args>
void JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>& writer, const T& arg1, Args&&... args) {
    _JsonWriter(writer, arg1);
    JsonWriter(writer, args...);
}

template <typename... Args>
void call(const char* func, Args... args) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    writer.StartObject();
    writer.Key(func);
    writer.StartArray();
    JsonWriter(writer, args...);
    writer.EndArray();
    writer.EndObject();
    std::cout<<s.GetString()<<std::endl;
}

int main(int argc, char** argv) {

    std::map<const char*, int> mm;
    mm["a"] = 1;
    mm["b"] = 2;
    auto i = {1,2,3};
    call("add", 123, 456.0, std::string("str"), "str2", nullptr, mm);

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
        std::cout<<"error:"<<fd<<" ERROR"<<std::endl;
    });
    serv.onClose([](int fd){
        std::cout<<"error:"<<fd<<" EOF"<<std::endl;
    });
    bool ret = serv.start();
    if(!ret) {
        std::cout<<"error:"<<serv.getLastError()<<std::endl;
    }
    return 0;
}
