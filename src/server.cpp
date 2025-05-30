#include "../inc/Server.hpp"
#include "../inc/HttpRequest.hpp"

void startServer()
{
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;  // ipV4
	serverAddress.sin_port = htons(1234); // random working port in Hive
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	bind(serverfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	listen(serverfd, 5);

	int clientfd = accept(serverfd, nullptr, nullptr);

	char buffer[1024] = {0};
	recv(clientfd, buffer, sizeof(buffer),0);

	//PARSING THE HTTP MESSAGE STATRS HERE ??

	HttpRequest req1;
	std::cout << "Message: " << buffer << std::endl;
	req1.parse(buffer);
	//need to send message back here if it was valid message
	std::string response = "Viva la 42\n"; // test message
	send(clientfd, response.c_str(), response.size(), 0);

	close(serverfd);
}
