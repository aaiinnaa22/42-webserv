#include "../inc/Server.hpp"

void set_non_blocking(int fd) 
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void handle_epoll_event(int epollfd, struct epoll_event *events, int read_count, int serverfd, char *buffer)
{
	int fd;
	struct epoll_event ev;
    ev.events = EPOLLIN;
    
	int clientfd;
	struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

	for(int i = 0; i < read_count; i++)
	{
		fd = events[i].data.fd;
		//set_non_blocking(fd);
		if ((fd == serverfd) && (events[i].events & EPOLLIN))
		{
			std::cout << "fd index is: " << fd << std::endl;
			clientfd = accept(fd, (struct sockaddr *)&addr, &addr_len);
			//set_non_blocking(clientfd);
			ev.data.fd = clientfd;
			epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
			
			std::cout << "New connection: " << std::endl;
			//std::string response = "Viva la 42\n"; // test message
			//send(clientfd, response.c_str(), response.size(), 0);
		}
		else if ((events[i].events & EPOLLIN))
		{{{
			//read(fd,buffer,1024);
			recv(clientfd, buffer, sizeof(buffer),0);
			//NEW SPOT FOR PARSING
			std::cout << "Message: " << buffer << std::endl;
			epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
		}}}
	}
}

void start_epoll(int serverfd)
{
	int epollfd;
	struct epoll_event events[100]; // FIgure better number here, Numeber of events epoll_wait can return?
	int read_count;
	char buffer[1024] = {0};

	epollfd = epoll_create(42); // creates new epoll instance and returns fd for it;

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = serverfd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &ev);
	
	while(1)
	{
		read_count = epoll_wait(epollfd, events, 42, -1); // returns number of events that are ready to be handled
		handle_epoll_event(epollfd, events, read_count, serverfd, buffer);
		//std::cout << read_count << std::endl;
	}
	//close(serverfd); 
}

void startServer()
{
	int on = 1;

	int serverfd = socket(AF_INET, SOCK_STREAM, 0);
	set_non_blocking(serverfd);

	int rc = setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

	sockaddr_in serverAddress; // memset struct to 0 ??
	serverAddress.sin_family = AF_INET;  // ipV4
	serverAddress.sin_port = htons(1234); // random working port in Hive
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	bind(serverfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	listen(serverfd, 5);
	start_epoll(serverfd);
	close(serverfd);
}
