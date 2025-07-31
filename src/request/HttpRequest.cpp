
#include "../../inc/HttpRequest.hpp"


//param constructor with client fd
HttpRequest::HttpRequest(int fd) :clientfd(fd) {}


void HttpRequest::setMethod(const std::string& m) 
{
	method = m;
}

std::string HttpRequest::getMethod() const
{
	return method;
}

void HttpRequest::setPath(const std::string& p)
{ 
	path = p;
}

void HttpRequest::setHttpVersion(const std::string& v)
{
	httpVersion = v; 
}

void HttpRequest::addHeader(const std::string& key, const std::string& value)
{
	headers[key] = value;
}

void HttpRequest::setBody(const std::string& b)
{
	body = b;
}

void HttpRequest::appendBody(const std::string& data)
{
    body += data;
}

void HttpRequest::setKeepAlive(bool isAlive)
{
	isKeepAlive = isAlive;
}

std::string HttpRequest::getHeader(const std::string& key) const
{ 
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    return (it != headers.end()) ? it->second : "";
}

void HttpRequest::dump() const {
    std::cout << "Method: " << method << "\n";
    std::cout << "Path: " << path << "\n";
    std::cout << "Version: " << httpVersion << "\n";
    std::cout << "Headers:\n";
    for (const auto& [key, value] : headers) {
        std::cout << "  " << key << ": " << value << "\n";
    }
	//std::cout << "Body: " << body << std::endl;
	std::cout << "Keep alive/ close boolean: " << isKeepAlive << std::endl;
}
