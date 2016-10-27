all:
	g++ -std=c++11 -I/usr/local/include  -g -ggdb -o main main.cpp event_loop.cpp poll.cpp tcp_server.cpp util.cpp channel.cpp
run: all 
	./main
