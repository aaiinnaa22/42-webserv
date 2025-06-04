#include "../inc/Server.hpp"
#include "../inc/HttpRequest.hpp"

void	HttpRequest::parse(const std::string &request)
{
 	std::istringstream stream(request);
	std::string line;
	//Method, path, version
	std::getline(stream, line);
    size_t methodEnd = line.find(' ');
    size_t pathEnd = line.find(' ', methodEnd + 1);
    method = line.substr(0, methodEnd);
	//check if method is GET POST OR DELETE, 405 if it's not
    path = line.substr(methodEnd + 1, pathEnd - methodEnd - 1);
    httpVersion = line.substr(pathEnd + 1);
	//the prints are just checks - comment out if necessary
	//std::cout << "\nFrom parsing:\n";
	//std::cout << "method: " << method << std::endl;
	//std::cout << "path: " << path << std::endl;
	//std::cout << "http ver: " << httpVersion << std::endl;
	//throw 505 if version is not 1.1????
	//headers
	//std::map<std::string, std::string> headers;
	while(getline(stream, line))
	{
		if (line == "\r" || line.empty())
   	    	break;
		size_t colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 2);
			value.erase(value.find_last_not_of("\r\n") + 1);
			headers[key] = value;
		}
	}
	//another print for checking
	//for (const auto& header : headers)
	//{
	//	std::cout << header.first << ": " << header.second << "\n";
	//}
	if (headers.count("Content-Length"))
	{
		int length = std::stoi(headers["Content-Length"]);
		body.resize(length);
		stream.read(&body[0], length);
		if (stream.gcount() < length)
		{
            throw std::runtime_error("400 Bad Request: Body incomplete");
        }// incomplete read check
	}
	//another print for checking
	//if (!body.empty())
	//{
    //    std::cout << "Body: " << body << "\n";
    //}
}

//Aina

//param constructor with client fd
HttpRequest::HttpRequest(int fd) :clientfd(fd) {}


void HttpRequest::sendResponse(std::string status)
{
	ssize_t sending;
	std::string response;
	std::string contentLength;

	contentLength = std::to_string(responseBody.size());

	response =
	"HTTP/1.1 " + status + "\r\n" +
	"Content-Type: " + responseContentType + "\r\n" +
	"Content-Length: " + contentLength + "\r\n" +
	"\r\n" + responseBody;
	sending = send(clientfd, response.c_str(), response.size(), 0);
	//error check
}

void HttpRequest::setContentType(std::string path)
{
	size_t dot;
	std::string fileExtension;

	dot = path.rfind(".");
	fileExtension = path.substr(dot + 1, path.length()); //try catch in main
	if (fileExtension == "html")
		responseContentType = "text/html";
	else
		throw std::runtime_error("415 Unsupported Media Type"); //?
}

//! poll for read and open
void HttpRequest::methodGet(void)
{
	ssize_t charsRead;
	int fd;
	char buffer[1000];

	path = "." + path;
	fd = open(path.c_str(), O_RDONLY); //nonblock?
	if (fd == -1)
		throw std::runtime_error("404 Not found");
	std::cout << "file " << path << " opened" << std::endl;
	while ((charsRead = read(fd, buffer, sizeof(buffer))) > 0)
		responseBody.append(buffer, charsRead);
	close(fd);
	if (charsRead == -1)
		throw std::runtime_error("500 Internal Server Error"); //?
	setContentType(path);
	sendResponse("200 OK");
}

//EPOLL!!!
void HttpRequest::methodPost(void)
{
	//file name has to be like hey, not /hey????
	ssize_t charsWritten;
	int fd;
	unsigned long contentLength;

	while (true) //clean up ./ or similar
	{
		size_t pos = path.find("./");
		if (pos == std::string::npos)
			break ;
		path.erase(pos, 2);
	}
	auto it = headers.find("Content-Length");
	if (it != headers.end())
		contentLength = std::stoul(it->second);
	else 
		throw std::runtime_error("411 Length Required");
	fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
	if (fd == -1)
		throw std::runtime_error("500 Internal Server Error"); //?
	charsWritten = write(fd, body.c_str(), contentLength);
	close(fd);
	if (charsWritten == -1)
		throw std::runtime_error("500 Internal Server Error"); //?
}

void HttpRequest::methodDelete(void)
{
	bool removed;

	try 
	{
		removed = std::filesystem::remove(path);
		if (removed)
			throw std::runtime_error("200 OK"); //???
		else 
			throw std::runtime_error("404 Not Found");
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		throw std::runtime_error("403 Forbidden");
	}
}

void HttpRequest::doCgi(void){}

void HttpRequest::doRequest(void)
{
	//security issue check
	if (path.empty() || 
	path.find("/..") != std::string::npos || 
	path.find("../") != std::string::npos ||
	path == "..")
		throw std::runtime_error("400 Bad Request");
	if (path == "/")
		path = "/index.html";
	if (method == "GET")
	{
		//if (path.ends_with(".py"))
		//	;//CGI
		//else
			methodGet();
	}
	else if (method == "POST")
		methodPost();
	else if (method == "DELETE")
		methodDelete();
}

//Aina end

//ERRORS:
//CLIENT ERROR RESPONSES
//400 Bad Request
//403 Forbidden
//404 Not found
//405 Method not allowed
//408 Request timeout
//409 Conflict
//411 Length required
//413 Payload too large
//414 URI Too Long
//415 Unsupported Media Type
//418 I'm a teapot - MUST HAVE FOR US  :D
//431 Request Header Fields Too Large
//SERVER ERROR RESPONSES
//500 Internal Server Error
//501 Not Implemented
//503 Service Unavailable
//505 HTTP Version Not Supported