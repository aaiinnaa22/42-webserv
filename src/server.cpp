#include "../inc/Server.hpp"

 Server::Server(){
	 int _on = 1;
	 int _serverfd = 0;
	 int _epollfd = 0;
	 int _read_count = 0;
	 int _clientfd = 0;
 }

 Server::~Server(){}

void Server::set_non_blocking(int fd) 
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::handle_epoll_event(struct epoll_event *events, char *buffer)
{
	int fd;
	struct epoll_event ev;
    ev.events = EPOLLIN;
    
	struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

	for(int i = 0; i < _read_count; i++)
	{
		fd = events[i].data.fd;
		//set_non_blocking(fd);
		if ((fd == _serverfd) && (events[i].events & EPOLLIN))
		{
			std::cout << "fd index is: " << fd << std::endl;
			_clientfd = accept(fd, (struct sockaddr *)&addr, &addr_len);
			//set_non_blocking(_clientfd);
			ev.data.fd = _clientfd;
			epoll_ctl(_epollfd, EPOLL_CTL_ADD, _clientfd, &ev);
			
			std::cout << "New connection: " << std::endl;
			//std::string response = "Viva la 42\n"; // test message
			//send(_clientfd, response.c_str(), response.size(), 0);
		}
		else if ((events[i].events & EPOLLIN))
		{{{
			//read(fd,buffer,1024);
			recv(_clientfd, buffer, sizeof(buffer),0);
			//NEW SPOT FOR PARSING
			std::cout << "Message: " << buffer << std::endl;
			epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev);
		}}}
	}
}

void Server::start_epoll()
{
	struct epoll_event events[100]; // FIgure better number here, Numeber of events epoll_wait can return?
	char buffer[1024] = {0};

	_epollfd = epoll_create(42); // creates new epoll instance and returns fd for it;

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = _serverfd;
	epoll_ctl(_epollfd, EPOLL_CTL_ADD, _serverfd, &ev);
	
	while(1)
	{
		_read_count = epoll_wait(_epollfd, events, 42, -1); // returns number of events that are ready to be handled
		handle_epoll_event(events, buffer);
		//std::cout << _read_count << std::endl;
	}
	//close(_serverfd); 
}

void Server::startServer()
{
	_serverfd = socket(AF_INET, SOCK_STREAM, 0);
	set_non_blocking(_serverfd);

	int rc = setsockopt(_serverfd, SOL_SOCKET, SO_REUSEADDR, (char *)&_on, sizeof(_on));

	sockaddr_in serverAddress; // memset struct to 0 ??
	serverAddress.sin_family = AF_INET;  // ipV4
	serverAddress.sin_port = htons(1234); // random working port in Hive
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	bind(_serverfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	listen(_serverfd, 5);
	start_epoll();
	close(_serverfd);
}
