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

void startServer();

#endif
