#include "../inc/Server.hpp"
#include "../inc/HttpRequest.hpp"

 Server::Server(){
	 int _on = 1;
	 int _serverfd = 0;
	 int _epollfd = 0;
	 int _read_count = 0;
	 int _clientfd = 0; // unusedd??
 }

 Server::~Server(){}

/*  ...Fcntl
	int fcntl(int fd, int op, ...) /* arg
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
void Server::set_non_blocking(int fd) 
{
    int flags = fcntl(fd, F_GETFL, 0); // F_GETFL asks for the current flags on the file descriptor fd
    fcntl(fd, F_SETFL, flags | O_NONBLOCK); // We set the new flags of the fd to be = old flags + the O_NONBLOCK flag using bitwise OR (|).
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
void Server::handle_epoll_event(struct epoll_event *events)
{
	int fd;
	struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
	struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

	for(int i = 0; i < _read_count; i++)
	{
		fd = events[i].data.fd;
		if ((fd == _serverfd) && (events[i].events & EPOLLIN)) // Probably no need to check EPOLLIN
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
			char buffer[1024] = {0};
			int bytes_read = recv(fd, buffer, sizeof(buffer),0);
			//NEW SPOT FOR PARSING
			HttpRequest req1(fd);
			std::cout << "Message from startServer: \n" << buffer << std::endl;
			req1.parse(buffer);
			//Aina
			req1.doRequest();
			// if buffer is empty after recv it means client closed the connection???
			if (bytes_read == 0) {
                std::cout << "Connection closed by client: " << fd << std::endl;
                epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
			}
			else	
				std::cout << "Message: " << buffer << std::endl;
			//epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev); 'Saved this here not sure if will be needed'
		}
		else if ((events[i].events & EPOLLHUP )) // Not working ????????????????????????
		{
			std::cout << "Connection closed: " << fd << std::endl;
			epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, NULL);
			close(fd);
			continue ;
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
void Server::start_epoll()
{
	struct epoll_event events[100]; // FIgure better number here, Numeber of events epoll_wait can return?
	_epollfd = epoll_create(42); // creates new epoll instance and returns fd for it;
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET; // not sure if I need this here.
	ev.data.fd = _serverfd;
	epoll_ctl(_epollfd, EPOLL_CTL_ADD, _serverfd, &ev);
	while(1)
	{
		_read_count = epoll_wait(_epollfd, events, 42, -1); // returns number of events that are ready to be handled
		if (_read_count != 0)
			handle_epoll_event(events);
	}
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
void Server::startServer()
{
	_serverfd = socket(AF_INET, SOCK_STREAM, 0);
	set_non_blocking(_serverfd);

	int rc = setsockopt(_serverfd, SOL_SOCKET, SO_REUSEADDR, (char *)&_on, sizeof(_on));

	sockaddr_in serverAddress; // memset struct to 0 ??
	serverAddress.sin_family = AF_INET;  // ipV4
	serverAddress.sin_port = htons(1234); // random working port in Hive
	serverAddress.sin_addr.s_addr = INADDR_ANY; // All possible available ip addresses

	bind(_serverfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	listen(_serverfd, 5);
	start_epoll();
	close(_serverfd);
}
