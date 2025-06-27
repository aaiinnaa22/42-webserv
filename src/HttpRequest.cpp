#include "../inc/Server.hpp"
#include "../inc/HttpRequest.hpp"

// void	HttpRequest::parse(const std::string &request)
// {
//  	std::istringstream stream(request);
// 	std::string line;
// 	//Method, path, version
// 	std::getline(stream, line);
//     size_t methodEnd = line.find(' ');
//     size_t pathEnd = line.find(' ', methodEnd + 1);
//     method = line.substr(0, methodEnd);
// 	//check if method is GET POST OR DELETE, 405 if it's not
//     path = line.substr(methodEnd + 1, pathEnd - methodEnd - 1);
//     httpVersion = line.substr(pathEnd + 1);
// 	//the prints are just checks - comment out if necessary
// 	//std::cout << "\nFrom parsing:\n";
// 	//std::cout << "method: " << method << std::endl;
// 	//std::cout << "path: " << path << std::endl;
// 	//std::cout << "http ver: " << httpVersion << std::endl;
// 	//throw 505 if version is not 1.1????
// 	//headers
// 	//std::map<std::string, std::string> headers;
// 	while(getline(stream, line))
// 	{
// 		if (line == "\r" || line.empty())
//    	    	break;
// 		size_t colon = line.find(':');
// 		if (colon != std::string::npos)
// 		{
// 			std::string key = line.substr(0, colon);
// 			std::string value = line.substr(colon + 2);
// 			value.erase(value.find_last_not_of("\r\n") + 1);
// 			headers[key] = value;
// 		}
// 	}
// 	//another print for checking
// 	//for (const auto& header : headers)
// 	//{
// 	//	std::cout << header.first << ": " << header.second << "\n";
// 	//}
// 	if (headers.count("Content-Length"))
// 	{
// 		int length = std::stoi(headers["Content-Length"]);
// 		body.resize(length);
// 		stream.read(&body[0], length);
// 		if (stream.gcount() < length)
// 		{
//             throw std::runtime_error("400 Bad Request: Body incomplete");
//         }// incomplete read check
// 	}
// 	//another print for checking
// 	//if (!body.empty())
// 	//{
//     //    std::cout << "Body: " << body << "\n";
//     //}
// }

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

void HttpRequest::setContentType(void)
{
	size_t dot;
	std::string fileExtension;

	dot = completePath.rfind(".");
	if (dot == std::string::npos)
		throw std::runtime_error("415 Unsupported Media Type");
	fileExtension = completePath.substr(dot + 1, completePath.length()); //try catch in main
	if (fileExtension == "html" || fileExtension == "css")
		responseContentType = "text/" + fileExtension;
	else if (fileExtension == "png" || fileExtension == "gif")
		responseContentType = "image/" + fileExtension;
	else if (fileExtension == "jpg" || fileExtension == "jpeg")
		responseContentType = "image/jpeg";
	else if (fileExtension == "txt")
		responseContentType = "text/plain";
	else if (fileExtension == "ico")
		responseContentType = "image/x-icon";
	else
		throw std::runtime_error("415 Unsupported Media Type"); //?
}


void HttpRequest::ResponseBodyIsDirectoryListing(void)
{
	std::string html_content;
	DIR* dir;

	dir = opendir(completePath.c_str());
	if (dir == nullptr)
		throw std::runtime_error("500 Internal Server Error"); //?
	
	html_content = 
	"<html>\n"
	"<head><title>Directory Listing</title></head>\n"
	"<body>\n"
	"<h1>Directory Listing</h1>\n"
	"<ul>\n";

	responseBody = html_content;
	
	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) //skip entries . and ..
			continue ;
		//what about .hiddenfiles?
		std::string strEntry(entry->d_name);
		html_content = "<li><a href=\"" + strEntry + "\">" + strEntry + "</a></li>\n"; 
		responseBody += html_content;
	}

	html_content = 
	"</ul>\n"
	"</body>\n"
	"</html>\n";
	
	responseBody += html_content;
	closedir(dir);
}

int HttpRequest::checkPathIsDirectory(void)
{
	struct stat path_stat;
	if (stat(completePath.c_str(), &path_stat) != 0)
		throw std::runtime_error("404 Not Found");
	return (S_ISDIR(path_stat.st_mode));
}

//! poll for read and open
void HttpRequest::methodGet(void)
{
	ssize_t charsRead;
	int fd;
	char buffer[1000];
	//std::cout << "LETS TRY TO GET" << completePath << std::endl;
	if (completePath.back() == '/' || checkPathIsDirectory() == 1)
	{
		//if (!currentLocation.index.empty())
		//	path = path + currentLocation.index; COMPLETE_PATH
		/*else*/ if (currentLocation.dir_listing)
		{
			ResponseBodyIsDirectoryListing();
			responseContentType = "text/html";
			sendResponse("200 OK");
			return ;
		}
		//else?? 404 not found??
	}

	//cgi script??

	//std::cout << "FINDING FILE..." << std::endl;
	fd = open(completePath.c_str(), O_RDONLY); //nonblock?
	if (fd == -1)
		throw std::runtime_error("404 Not found");
	//std::cout << "FILE FOUND!" << std::endl;
	while ((charsRead = read(fd, buffer, sizeof(buffer))) > 0)
		responseBody.append(buffer, charsRead);
	close(fd);
	if (charsRead == -1)
		throw std::runtime_error("500 Internal Server Error"); //?
	setContentType();
	sendResponse("200 OK");
}

//EPOLL!!!
void HttpRequest::methodPost(void)
{
	ssize_t charsWritten;
	int fd;

	fd = open(completePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); //last is chmod persmissions, owner=read and write, others=read, O_CREAT???
	if (fd == -1)
		throw std::runtime_error("500 Internal Server Error"); //?
	charsWritten = write(fd, body.c_str(), body.size());
	close(fd);
	if (charsWritten == -1)
		throw std::runtime_error("500 Internal Server Error"); //?
	setContentType();
	sendResponse("200 OK");
}

void HttpRequest::methodDelete(void)
{
	bool removed;

	try 
	{
		removed = std::filesystem::remove(completePath);
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

void HttpRequest::doCgi(void)
{}

void HttpRequest::findCurrentLocation(ServerConfig config)
{
	int longest_match_len = 0;
	int match_len = 0;
	bool match_found = false;

	for (auto location : config.locations)
	{
		if (path.find(location.path) == 0) //path starts with location path
		{
			match_len = location.path.length();
			if (match_len > longest_match_len)
			{
				currentLocation = location;
				longest_match_len = match_len;
				match_found = true;
			}
		}
		//in case of exact match, return that?
	}
	if (!match_found)
		throw std::runtime_error("404 Not found");
}

void HttpRequest::checkPathIsSafe(std::string locationRoot)
{
	std::vector<std::string> pathParts;
	int start = 0;
	int end = 0;

	while (end <= completePath.size())
	{
		if (end == completePath.size() || completePath[end] == '/')
		{
			if (end > start)
			{
				std::string part = completePath.substr(start, end - start);
				if (part == ".")
					;
				else if (part == "..")
				{
					if (!pathParts.empty())
						pathParts.pop_back();
				}
				else
					pathParts.push_back(part);
			}
			start = end + 1;
		}
		++end;
	}
	std::string normalizedPath = "/";
	for (int i = 0; i < pathParts.size(); ++i)
	{
		normalizedPath += pathParts[i];
		if (i != pathParts.size() - 1)
			normalizedPath += "/";
	}
	if (normalizedPath.find(locationRoot) != 0)
		throw std::runtime_error("403 Forbidden");
}

void HttpRequest::doRequest(ServerConfig config)
{
	//make sure all response stuff, like response body, is cleared out/empty for every request
	//dump();
	std::cout << "path in do request without root: " << path << std::endl;
	if (path.empty())
	{
		std::cout << "no path incoming to doRequest...stopping request" << std::endl;
		return ;
	}
	findCurrentLocation(config);
	completePath = currentLocation.root + path;
	path.clear();
	std::cout << "path from do request: " << completePath << std::endl;
	checkPathIsSafe(currentLocation.root);
	if (method == "GET" && 
		std::find(currentLocation.methods.begin(), currentLocation.methods.end(), "GET") != 
		currentLocation.methods.end())
		methodGet();
	else if (method == "POST" &&
		std::find(currentLocation.methods.begin(), currentLocation.methods.end(), "POST") != 
		currentLocation.methods.end())
		methodPost();
	else if (method == "DELETE" &&
		std::find(currentLocation.methods.begin(), currentLocation.methods.end(), "DELETE") != 
		currentLocation.methods.end())
		methodDelete();
	else 
		throw std::runtime_error("405 Method not allowed");
}

//Aina end

const std::map<std::string, std::string>& HttpRequest::getHeaders() const 
{ 
	return headers; 
}

void HttpRequest::setMethod(const std::string& m) 
{
	method = m;
}

void HttpRequest::setPath(const std::string& p)
{ 
	path = p;
}


std::string HttpRequest::getPath(){
	return path;
}

std::string HttpRequest::getPath() const{
	return path;
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
    std::cout << "Body:\n" << body << "\n";
}


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