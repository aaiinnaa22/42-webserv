#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <fcntl.h>

class Server {
    private:
        int _on;
        int _serverfd;
        int _epollfd;
        int _read_count;
        int _clientfd; //unused???
    public:
        Server();
        ~Server();

        int set_non_blocking(int fd);
        void handle_epoll_event(struct epoll_event *events);
        int start_epoll();
        void startServer(int listen_port, std::string host);
        int32_t get_networkaddress(std::string host);
};

#endif
