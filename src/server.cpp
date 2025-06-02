#include "../inc/Server.hpp"
#include "../inc/HttpRequest.hpp"

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

void Server::handle_epoll_event(struct epoll_event *events)
{
	int fd;
	char buffer[1024] = {0};
	struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLET;

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

	for(int i = 0; i < _read_count; i++)
	{
		fd = events[i].data.fd;

		if ((events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)))
		{
			std::cout << "Connection closed: " << fd << std::endl;
			epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, NULL);
			close(fd);
			continue ;
		}

		else if ((fd == _serverfd) && (events[i].events & EPOLLIN))
		{
			int clientfd = accept(fd, (struct sockaddr *)&addr, &addr_len);
			if (clientfd < 0){
				//add error here
				close(clientfd);
			}
			set_non_blocking(clientfd);

			
			ev.data.fd = clientfd;
			if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, clientfd, &ev) < 0 ){
				//error
				close(clientfd);
			}
			
			// Just here to print information;
			uint16_t src_port = ntohs(addr.sin_port);
			std::cout << "New connection ip: " << (struct sockaddr *)&addr;
			std::cout << "Port: " << src_port << std::endl;
		}
		else if ((events[i].events & EPOLLIN))
		{

			int bytes_read = recv(fd, buffer, sizeof(buffer),0);
			
			//NEW SPOT FOR PARSING
			
			HttpRequest req1(fd);
			std::cout << "Message from startServer: \n" << buffer << std::endl;
			req1.parse(buffer);
			//Aina
			req1.doRequest();
			
			// if buffer is empty after recv it means client closed the connection???
			if (bytes_read == 0) {
                // Connection closed by client
                std::cout << "Connection closed by client: " << fd << std::endl;
                epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
			}
			else	
				std::cout << "Message: " << buffer << std::endl;

			//epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev); 'Saved this here not sure if will be needed'
		}
	}
}

void Server::start_epoll()
{
	struct epoll_event events[100]; // FIgure better number here, Numeber of events epoll_wait can return?
	

	_epollfd = epoll_create(42); // creates new epoll instance and returns fd for it;

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLHUP | EPOLLET;
	ev.data.fd = _serverfd;
	epoll_ctl(_epollfd, EPOLL_CTL_ADD, _serverfd, &ev);
	
	while(1)
	{
		_read_count = epoll_wait(_epollfd, events, 42, -1); // returns number of events that are ready to be handled
		if (_read_count != 0)
			handle_epoll_event(events);
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
