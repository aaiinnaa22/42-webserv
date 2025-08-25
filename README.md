# 42-webserv

Hive Helsinki project

☆ INFO ☆ ☆ ☆ ☆ ☆ ☆ ☆ ☆ ☆ ☆

This project is about writing a fully functional HTTP server in C++, according to the HTTP/1.1 protocol.
The server handles multiple clients concurrently using non-blocking I/O, and stays intact under stress tests.

The program will:

-  Parse a configuration file (inspired by Nginx configuration files)
-  Accept connections on many ports, including virtual hosts
-  Handle multiple clients at the same time without blocking
-  Respond with correct HTTP status codes
-  Handle errors gracefully (the program will never crash or hang forever)

The server works like a real web server:
-  Serves static files
-  Allows file uploads
-  Supports methods GET, POST and DELETE
-  Supports CGI execution (PHP-cgi and python)
-  Provides default error pages
-  Supports directory listing
-  Handles HTTP redirections

☆ KEY CONCEPTS ☆ ☆ ☆ ☆ ☆ ☆

-  Non blocking network programming (sockets + epoll)
-  HTTP/1.1 request and response handling
-  Concurrent clients on a single process
-  CGI execution with execve
-  File handling (opening, reading, uploading, deleting and sending)
-  Configuration parsing

☆ RUN THE CODE ☆ ☆ ☆ ☆ ☆ ☆

In terminal, clone the repo and compile:

```
git clone https://github.com/aaiinnaa22/42-webserv.git
cd 42-webserv
make
```

You can run the program with a configuration file from conf/, or give no program aruments and run with the default configuration. (For the best user experience, I would recommend running with basic.conf).

```
./webserver conf/basic.conf
```

Now the webserver is running. You can test it in two ways.

With a **web browser**:

Open a web browser (recommended: chrome) and type in http://localhost:8081 (or another port supported).

With **telnet**:

Open a new terminal window and start a telnet session: 
```
telnet localhost 8081
```
(Or another supported port).

Now you can write in a request in the telnet session.
For example:
```
GET / HTTP/1.1
host: name
```
And press enter twice. The client (telnet) will recieve a response from the server.
