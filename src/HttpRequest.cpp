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
	std::cout << "\nFrom parsing:\n";
	std::cout << "method: " << method << std::endl;
	std::cout << "path: " << path << std::endl;
	std::cout << "http ver: " << httpVersion << std::endl;
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
	for (const auto& header : headers)
	{
		std::cout << header.first << ": " << header.second << "\n";
	}
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
	if (!body.empty())
	{
        std::cout << "Body: " << body << "\n";
    }
}

//Aina

//param constructor with client fd
HttpRequest::HttpRequest(int fdOfClient) :clientfd(fdOfClient) {}

void HttpRequest::sendResponse(std::string status, std::string body)
{
	ssize_t sending;
	std::string response;
	std::string contentType;
	std::string contentLength;

	contentType = "Content-Type: text/html"; //get based on file extension
	contentLength = "Content-Length: " + std::to_string(body.size());

	response =
	"HTTP/1.1 " + status + "\r\n" +
	contentType + "\r\n" +
	contentLength + "\r\n" +
	"\r\n" +
	body;
	sending = send(clientfd, response.c_str(), response.size(), 0);
	//error check
}

void HttpRequest::methodGet(void)
{
	ssize_t charsRead;
	int fd;
	std::string fileContent;
	char buffer[1000];

	path = "." + path;
	fd = open(path.c_str(), O_RDONLY); //nonblock?
	//error check
	while ((charsRead = read(fd, buffer, sizeof(buffer))) > 0)
		fileContent.append(buffer, charsRead);
	//error check read
	close(fd);
	sendResponse("200 OK", fileContent);
}

void HttpRequest::methodPost(void){}

void HttpRequest::methodDelete(void){}

void HttpRequest::doCgi(void){};
void HttpRequest::doRequest(void)
{
	if (path == "/")
		path = "/index.html";
	if (method == "GET")
	{
		//if (path.ends_with(".py"))
		//	;//CGI
		//else
			methodGet();
	}
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