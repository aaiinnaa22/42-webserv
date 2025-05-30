#include "../inc/Server.hpp"
#include "../inc/HttpRequest.hpp"

void	HttpRequest::parse(const std::string &request)
{
 	std::istringstream stream(request);
	std::string line;
	
	std::getline(stream, line);
    size_t methodEnd = line.find(' ');
    size_t pathEnd = line.find(' ', methodEnd + 1);
    method = line.substr(0, methodEnd);
    path = line.substr(methodEnd + 1, pathEnd - methodEnd - 1);
    httpVersion = line.substr(pathEnd + 1);

	std::cout << "method: " << getMethod() << std::endl;
	std::cout << "path: " << getPath() << std::endl;
	std::cout << "http ver: " << getHttpVersion() << std::endl;
}

std::string HttpRequest::getMethod(){
	return method;
}

std::string HttpRequest::getPath(){
	return path;
}

std::string HttpRequest::getHttpVersion(){
	return httpVersion;
}