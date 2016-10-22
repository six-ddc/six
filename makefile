all:
	g++ -std=c++11 -g -ggdb -o main main.cpp event_loop.cpp poll.cpp tcp_server.cpp util.cpp
run: all 
	./main
