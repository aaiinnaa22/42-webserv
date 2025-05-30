#pragma once
#include <string>
#include <sstream>
#include <map>

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
	public:
		void	parse(const std::string& request);
		std::string	getMethod();
		std::string	getPath();
		std::string	getHttpVersion();
};
