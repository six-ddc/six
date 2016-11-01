cmake_minimum_required(VERSION 3.6)
project(net)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -ggdb")

include_directories("/usr/local/include")

set(SOURCE_FILES main.cpp event_loop.cpp poll.cpp util.cpp channel.cpp)
add_executable(net ${SOURCE_FILES})