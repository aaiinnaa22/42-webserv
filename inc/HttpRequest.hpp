#pragma once
#include <string>
#include <sstream>
#include <map>
#include <fcntl.h>     // open

class HttpRequest
{
	//requestLine method path httpVersion
	// one or more headers
	// optional body
	private:
		std::string method;
		std::string path;
		std::string httpVersion;
		std::map<std::string, std::string> headers;
		std::string	body;
		std::string responseBody;
		std::string responseContentType;
		int clientfd;
		void methodGet();
		void methodPost();
		void methodDelete();
		void doCgi();
		void sendResponse(std::string status);
		void setContentType(std::string path);

	public:
		void	parse(const std::string& request);
		std::string	getMethod();
		std::string	getPath();
		std::string	getHttpVersion();
		void	doRequest(void);
		HttpRequest(int fd);
};
