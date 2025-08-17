/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: llaakson <llaakson@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 19:01:37 by llaakson          #+#    #+#             */
/*   Updated: 2025/08/11 19:01:41 by llaakson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
#include <map>
#include <signal.h>
#include "ConfigParse.hpp"
#include "ClientConnection.hpp"
#include "../inc/ErrorResponseException.hpp"
#include <cstring>
#include <errno.h>
#include <ctime>

class ClientConnection;

extern bool gSignalClose;

class Server {
    private:
        int _on;
        int _serverfd[5] = {0,0,0,0,0};
        int _epollfd;
        int _read_count;
        std::map<int, ClientConnection> connections;
        int _testflag;
    public:
        Server();
        ~Server();

        int set_non_blocking(int fd);
        void handle_epoll_event(struct epoll_event *events, std::vector<ServerConfig> servers);
        int start_epoll(std::vector<ServerConfig> servers);
        void startServer(std::vector<ServerConfig> servers);
        int32_t get_networkaddress(std::string host);
        std::vector<int> get_open_fds() const;

        void close_connection(int fd,int flag);

        enum eraseconnection
		{
			KEEPCON,
            ERASECON
		};
};

#endif
