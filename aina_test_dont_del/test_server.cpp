#include "../inc/Server.hpp"

void startServer()
{
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;  // ipV4
	serverAddress.sin_port = htons(1234); // random working port in Hive
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	bind(serverfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	listen(serverfd, 5);

	while (true)
	{
		int clientfd = accept(serverfd, nullptr, nullptr);

		char buffer[1024] = {0};
		recv(clientfd, buffer, sizeof(buffer),0);

		//PARSING THE HTTP MESSAGE STATRS HERE ??
		//handle http request

		std::cout << "Message: " << buffer << std::endl;

		//need to send message back here if it was valid message
	}

	close(serverfd);
}

/*

- Need an array for the one and only poll call
Pseudo code for handling http requests

//clientFd and std::string response and std::vector<char> fileContent  in a class

sendResponse(std::string status)
{
	ssize_t sending;

	response =
	"HTTP/1.1 " + status + "\r\n" +
	//weird headers +
	"\r\n" +
	fileContent;

	sending = send(clientFd, response, response.size(), 0);
	if (sending < 0)
		;//error
}

in while(true)
if (method == "GET")
{
	ssize_t charsRead;

	if (path.ends_with(".py"))
	{
		...CGI
	}
	fd = open(path);
	charsRead = read(fd, fileContent, ...);
	if (read < 0)
		;//error
	sendResponse("200 OK");
}

*/
