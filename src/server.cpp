#include "../inc/Server.hpp"
#include "../inc/HttpRequest.hpp"
#include "../inc/ConfigParse.hpp"
#include "../inc/ClientConnection.hpp"
#include "../inc/ErrorResponseException.hpp"
#include <arpa/inet.h> // for inet_ntop, illegal function remove before submitting
#include <stdlib.h>
#include <cstring>
#include <errno.h>
#include <ctime>

 Server::Server() : _on(1), _epollfd(0), _read_count(0), _testflag(0){
	for (int i = 0; i < 5; i++){
		_serverfd[i] = 0;}
 }

 Server::~Server(){
	for (size_t k = 0; k < connections.size(); k++)
	{
		std::cout << k << " " << connections[k].getFd() << std::endl;
		if (connections[k].getFd() != -1)	
			close(connections[k].getFd());
	}
	for (size_t s = 0; s < 5 ; s++)
	{	
		if (_serverfd[s] != -1)
			close(_serverfd[s]);
	}
	if (_epollfd != -1)
		close(_epollfd);
 }

/*  ...Fcntl
	int fcntl(int fd, int op, ...) arg
	Used to modify behavior of already opened file descriptors.
	F_GETFL (void)
        Return (as the function result) the file access mode and the file status flags; arg is ignored.
	F_SETFL (int)
        Set the file status flags to the value specified by arg.
	O_NONBLOCK
 		This prevents open from blocking for a “long time” to open the file.
 	Non-blocking I/O 
 		Means that read() and write() will immediately return -1 if there is "nothing" to do
		and does not wait in the read or write call and can do something else while it waits.

		This youtube video has some nice visuals on how non blocking I/O works.
		https://www.youtube.com/watch?v=wB9tIg209-8
	*/
int Server::set_non_blocking(int fd) 
{
    int check = fcntl(fd, F_SETFL, O_NONBLOCK); // We set the new flags of the fd to be = old flags + the O_NONBLOCK flag using bitwise OR (|).
	if (check == -1)
		return check;
	check = fcntl(fd, F_SETFD, FD_CLOEXEC); // Magical flag that will make FD's close after forking or execv ( we shall see)
	if (check == -1)
		return check;
	return 0;
}
/*
	Here we loop through the events if epoll_wait() returned positive value, which means we have fd's that are ready to be handled. Number of events
	to be handled is _read_count. If the event is coming from the main server socket we know it's a new connection
	and we add it to the epoll list as new socket that we can listen. If the event is coming from different fd we know
	the even is coming from one of the connections we previously added to the epoll interest list. We can check the type of the event
	example (EPOLLIN) and then handle it how we choose (In this case we know its a message and we can start parsing).
	EPOLLHUP should mean the connection got closed (I think), but currently I can't get it working, challenge try to fix it :).
	If we read zero bytes from recv() we remove the fd from epoll list and close the fd -> this will close the connnection NOTE! this should
	probably not work like this, but so far the only way I have managed to close the connections in correct time.

	.......Recv
	ssize_t recv(int socket, void *buffer, size_t length, int flags);
	The only difference between recv() and read(2) is the presence of
       flags.  With a zero flags argument, recv() is generally equivalent
       to read(2). The recv() call is normally used only on a connected socket

*/

void Server::close_connection(int fd, int flag){
	if ((epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, NULL)) < 0)
		std::cerr << "Error! Failed to remove fd from epoll" << std::endl;
	if (close(fd) < 0)
		std::cerr << "Error! close_connection close()" << std::endl;
	if (flag == ERASECON)
		connections.erase(fd);
	std::cout << "Connection closed: " << fd << std::endl;

}

void Server::handle_epoll_event(struct epoll_event *events, std::vector<ServerConfig> servers)
{
	int fd;
	struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
	std::vector<ServerConfig> matching_servers;
	for(int i = 0; i < _read_count; i++)
	{	
		_testflag = 0;
		matching_servers.clear();
		fd = events[i].data.fd;
		for(size_t f = 0; f < servers.size(); f++)
		{
			if (fd == _serverfd[f])
			{
				_testflag = 1;
				int clientfd = accept(fd, (struct sockaddr *)&addr, &addr_len);
				if (clientfd < 0){
					std::cerr << "Failed to accept connection" << std::endl;
					continue;
				}
				if (set_non_blocking(clientfd) < 0){
					std::cerr << "Failed to set connection to non blocking" << std::endl;
					close(clientfd);
					continue;
				}
				struct epoll_event ev;
				ev.events = EPOLLIN;
				ev.data.fd = clientfd;
				if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, clientfd, &ev) < 0 ){
					std::cerr << "Failed to add new connection to epoll" << std::endl;
					close(clientfd);
					continue;
				}
				for (size_t j = 0; j < servers.size(); ++j){
					if (servers[j].getHost() == servers[f].getHost() && servers[j].getPort() == servers[f].getPort())
						matching_servers.push_back(servers[j]);
				}
				if (!connections.try_emplace(clientfd, clientfd, matching_servers).second){
					std::cerr << "Failed to insert connection for fd " << clientfd << std::endl;
					close_connection(clientfd,ERASECON);
					continue;
				}
				auto it = connections.find(clientfd);
				if (it != connections.end()){
					it->second.setLastActivity();
				}
				// check that nthos and inet_ntop are allowed functions when intra works again
				uint16_t src_port = ntohs(addr.sin_port); //All this is just printing information do we want int??
				in_addr_t saddr = addr.sin_addr.s_addr;
				char src_ip_buf[sizeof("xxx.xxx.xxx.xxx")];
				const char* cip = inet_ntop(AF_INET, &saddr, src_ip_buf ,sizeof("xxx.xxx.xxx.xxx"));
				std::cout << "New connection ip: " << cip;
				std::cout << " Port: " << src_port << std::endl;
			}
		}
		if ((events[i].events & EPOLLIN) && _testflag == 0)
		{
			char buffer[1024] = {0};
			int bytes_read = recv(fd, buffer, sizeof(buffer),0);
			if (bytes_read < 0){
				std::cerr << "recv failed on: " << fd << std::endl;
				close_connection(fd,ERASECON);
				continue ;
			}
			auto it = connections.find(fd);
			if (it == connections.end())
    			std::cerr << "No connection for fd " << fd << "\n";
			else
			{
    			auto &conn = it->second;
				try 
				{
					int result = conn.parseData(buffer, bytes_read, *this);
					if (result == 2 || result == 1)
					{
						struct epoll_event ev;
						ev.events = EPOLLIN | EPOLLOUT;
						ev.data.fd = fd;
						if (epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev) < 0 ){
							std::cerr << "Failed to modify epoll: " << fd << std::endl; 
							close_connection(fd,ERASECON);
						}
					}
				}
				catch (ChildError& e)
				{
					throw ChildError(500);
				}
				catch (std::runtime_error& e)
				{
					std::cout << "runtime error in parse" << e.what() << std::endl;
					conn.resetState();
				}
				catch (...)
				{
					std::cout << "unusual error with parsing" << std::endl;
					conn.resetState();
				}
			}
			if (bytes_read == 0)
			 	close_connection(fd,ERASECON);
		}
		if ((events[i].events & EPOLLOUT))
		{
			std::cout << "EPOLLOUT TRIGGERED, WE ARE SENDIND MESSAGE NOW" << std::endl;
			auto it = connections.find(fd);
			if (it == connections.end())
    			std::cerr << "Not found: " << fd << "\n";
			else
			{
    			auto &conn = it->second;
				std::cout << "EPOLLOUT triggered for fd " << fd << "\n";
				std::cout << conn.getResponse().getStatusCode() << std::endl;
				std::cout << conn.getResponse().getStatusMessage() << std::endl;
				try
				{
					if (!conn.getResponse().isSent)
					{
						conn.getResponse().sendResponse(fd, conn.getIsAlive());
					}
					if (conn.getResponse().isSent)
					{
						if (!conn.getIsAlive())
							close_connection(fd,ERASECON);
						else
						{
							conn.resetState();
							struct epoll_event ev;
							ev.events = EPOLLIN;
							ev.data.fd = fd;
							if(epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
								std::cerr << "Failed to modify epoll: " << fd << std::endl; 
								close_connection(fd,ERASECON);}
						}
					}
				}
				catch (ChildError& e)
				{
					throw ChildError(500);
				}
				catch (std::runtime_error& e)
				{
					std::cout << "sending error " << e.what() << std::endl;
					close_connection(fd,ERASECON);
				}
				catch (...)
				{
					std::cerr << "Failed to send response for fd " << fd << std::endl;
					close_connection(fd,ERASECON);
				}
			}
		}
	}
}

/*...Create a new epoll instance
	int epoll_create(int size);
	Since Linux 2.6.8, the size argument is ignored, but must be greater than zero; 
	returns a file descriptor referring to the new epoll instance. 
	This file descriptor is used for all the subsequent calls to the epoll interface.

	......Adding/removing or modifying epoll fd list;
       int epoll_ctl(int epfd, int op, int fd,struct epoll_event *_Nullable event);
	   int epfd = fd of the epoll instance.
	   int op = What we want to do (EPOLL_CTL_ADD,EPOLL_CTL_DEL or EPOLL_CTL_MOD);
	   int fd = fd we want to add/delete/modify.
	   strcut epoll_events:
	   
       struct epoll_event {
           uint32_t      events;  Epoll events
           epoll_data_t  data;    /User data variable 
       };

       union epoll_data {
           void     *ptr;
           int       fd;
           uint32_t  u32;
           uint64_t  u64;
       };
		The epoll_event structure specifies data that the kernel should
       save and return when the corresponding file descriptor becomes
       ready. 	

	...Epoll wait.(This is where our webserver sits and waits for something to happen)
	int epoll_wait(int epfd, struct epoll_event events[.maxevents],int maxevents, int timeout);
	 The epoll_wait() system call waits for events on the epoll(7)
       instance referred to by the file descriptor epfd.  The buffer
       pointed to by events is used to return information from the ready
       list about file descriptors in the interest list that have some
       events available.On success, epoll_wait() returns the number of file descriptors
       ready for the requested I/O operation.
	epfd = fd of our socket.
	maxevents = No idea I guess we can decide it.
	timeout = -1 means it will block indefinetly, untill something happens.
	..Events..
	EPOLLIN
              The associated file is available for read(2) operations.
			  Or there is a new connection coming
	EPOLLHUP
              Hang up happened on the associated file descriptor.

              epoll_wait(2) will always wait for this event; it is not
              necessary to set it in events when calling epoll_ctl().
	 EPOLLET
              Requests edge-triggered notification for the associated
              file descriptor.  The default behavior for epoll is level-
              triggered.  See epoll(7) for more detailed information
              about edge-triggered and level-triggered notification.
	( Not sure we need this EPOLLOT, but it stopped the epoll_wait to constantly calling handle_epoll_events)
	*/
int Server::start_epoll(std::vector<ServerConfig> servers)
{
	if ((_epollfd = epoll_create(42)) < 0)
		throw std::runtime_error("Error! Failed to create epoll");

	struct epoll_event events[1200]; // FIgure better number here, Numeber of events epoll_wait can return?
	struct epoll_event ev;
	ev.events = EPOLLIN; 

	for(size_t i = 0; i < servers.size(); i++){
		ev.data.fd = _serverfd[i];
		if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, _serverfd[i], &ev) < 0)
			throw std::runtime_error("Error! Failed to add server to epoll");}

	struct tm datetime{};
	datetime.tm_mday = 1;
  	if (mktime(&datetime) == -1)
		throw std::runtime_error("Error! Failed to create timestamp");

	while(gSignalClose == false)
	{
		if ((_read_count = epoll_wait(_epollfd, events, 1000, 1000)) < 0 && errno != EINTR) // returns number of events that are ready to be handled
			throw std::runtime_error("Error! Epoll wait failed");
		if (_read_count != 0)
		try{
			handle_epoll_event(events, servers);}
		catch (ChildError& e){
			throw ChildError(500);}
		for (auto it = connections.begin(); it != connections.end(); ) {
			int fd = it->first;
			auto& conn = it->second;
			if (conn.getFd() != -1) {
				std::time_t now = std::time(nullptr);
				int time_out_timer = now - conn.getLastActivity();
				if (time_out_timer > 160) {
					conn.setIsAlive(false);
					conn.getResponse().buildErrorResponse(408, fd);
					struct epoll_event ev;
    				ev.events = EPOLLOUT;
    				ev.data.fd = fd;
    				if ((epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev)) < 0){
						close_connection(fd,KEEPCON);
						it = connections.erase(it);
						continue;
					}}
			++it;
		}
	}}
	return 0;
}
/* ....Initiliazing Socket. 
		socket(int domain, int type, int protocol), creates an endpoint for communication and returns a file
       descriptor that refers to that endpoint.
	   AF_INET(IPv4 Internet protocols) =  The domain argument specifies a communication domain; this selects
       the protocol family which will be used for communication.
	   SOCK_STREAM = Provides sequenced, reliable, two-way, connection-based byte streams.
	   protocol =  The protocol specifies a particular protocol to be used with the
       socket.  Normally only a single protocol exists to support a
       particular socket type within a given protocol family, in which
       case protocol can be specified as 0.
	   ***socket(AF_INET, SOCK_STREAM, 0), Creaters tcp socket that listens IPv4 and uses default protocols.***

	   ...Manipulate options of the socket
	   int setsockopt(int socket, int level, int option_name,const void *option_value, socklen_t option_len);
		socket = fd of the socket,
		level = To manipulate options at the sockets API level, level is specified as SOL_SOCKET.
		option_name = tells the kernel that even if this port is busy (in the TIME_WAIT state), go ahead and reuse it anyway.
		https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
		option value = 1;
		***This allows the socket reuse the port in TIME_WAIT state after beign closed.***
        
		...Biding.
		int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
		Binds the socket to specific IP and port, so it knows what to listen.
		The htons() function converts the unsigned short integer hostshort from host byte order 
		to network byte order.
		INADDR_ANY = binds to all available IP addresses.
		Need to use sockaddr_int struct to give information to the bind.
		// ???? Should we bind the webserver to specific ip address ?????

		...Listening.
		int listen(int sockfd, int backlog); marks the socket referred to by sockfd as a passive
       socket, that is, as a socket that will be used to accept incoming
       connection requests using accept(2).
		The backlog argument defines the maximum length to which the queue
       of pending connections for sockfd may grow.  If a connection
       request arrives when the queue is full, the client may receive an
       error (I am not sure what would be correct size for the quee).
	   */
int32_t Server::get_networkaddress(std::string host)
{
	int i = 0;
	std::string segment;
	std::stringstream host1(host);
	std::vector<int> seglist;
	while(std::getline(host1, segment, '.'))
	{	
		i = std::stoi(segment);
		if (i >= 256)
			throw std::runtime_error("Error! Ip out of range");
		seglist.push_back(i);
		i = 0;
	}
	uint32_t ip_host_order = (seglist[0] << 24) | (seglist[1] << 16) | (seglist[2] << 8) | seglist[3];
	return ip_host_order;
}

void Server::startServer(std::vector<ServerConfig> servers)
{
	try{
	for(size_t i = 0; i < servers.size(); i++)
	{
		if ((_serverfd[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			throw std::runtime_error("Error! Failed to create socket"); 

		if (set_non_blocking(_serverfd[i]) < 0)
			throw std::runtime_error("Error! Failed ot set socket non blocking");

		if ((setsockopt(_serverfd[i], SOL_SOCKET, SO_REUSEADDR, (char *)&_on, sizeof(_on))) < 0)
			throw std::runtime_error("Error! Failed to create setsockopt");
	
		if ((setsockopt(_serverfd[i], SOL_SOCKET, SO_REUSEPORT, (char *)&_on, sizeof(_on))) < 0)
			throw std::runtime_error("Error! Failed to share port, how selfish");
	
		struct sockaddr_in serverAddress;
		memset(&serverAddress, 0, sizeof(sockaddr_in));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(servers[i].listen_port); 
		uint32_t ip_address = get_networkaddress(servers[i].host);
		serverAddress.sin_addr.s_addr = htonl(ip_address);

		if (bind(_serverfd[i], (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
			throw std::runtime_error("Error! Failed to bind server socket");
		
		if (listen(_serverfd[i], 128) < 0)
			throw std::runtime_error("Error! Failed to start listening server socket");
	}
	} catch (const std::runtime_error& e){
		throw;
	}
	int check1 = 0;
	try{
	check1 = start_epoll(servers);
	}
	catch (ChildError& e)
	{
		throw ChildError(500);
	}
	if (check1 < 0){
		close (_serverfd[0]);
		throw std::runtime_error("Error! epoll_ctl failed");
	}
}


std::vector<int> Server::get_open_fds() const
{
	std::vector<int> open_fds;
	for (std::map<int, ClientConnection>::const_iterator it = connections.begin(); it != connections.end(); ++it)
	{
		int client_fd = it->second.getFd();
        if (client_fd != -1)
            open_fds.push_back(client_fd);
	}
	for (int i = 0; i < 5 && _serverfd[i] != 0; ++i)
	{
        open_fds.push_back(_serverfd[i]);
    }
	if (_epollfd != -1)
        open_fds.push_back(_epollfd);

    return open_fds;
}