#include "../inc/Server.hpp"
#include "../inc/HttpRequest.hpp"

//Aina


//param constructor with client fd
HttpRequest::HttpRequest(int fd) :clientfd(fd) {}


void HttpRequest::setContentType(int postCheck)
{
	size_t dot;
	std::string fileExtension;
	std::string responseContentType;

	dot = completePath.rfind(".");
	if (dot == std::string::npos)
		throw ErrorResponseException(415);
	fileExtension = completePath.substr(dot + 1, completePath.length());
	if (postCheck == 1) //dont allow posting of scripts 
	{
		if (fileExtension != "jpg" && fileExtension != "jpeg" && fileExtension != "png"
				&& fileExtension != "gif" && fileExtension != "pdf")
			throw ErrorResponseException(415);
	}
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
	else if (fileExtension == "pdf")
		responseContentType = "application/pdf";
	else
		throw ErrorResponseException(415);
	httpResponse.setResponseHeader("content-type", responseContentType);
}


void HttpRequest::ResponseBodyIsDirectoryListing(void)
{
	std::string html_content;
	DIR* dir;
	std::string responseBody;

	dir = opendir(completePath.c_str());
	if (dir == nullptr) //500?
		throw ErrorResponseException(500);
	
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
	httpResponse.setResponseBody(responseBody);
	closedir(dir);
}

int HttpRequest::checkPathIsDirectory(void)
{
	struct stat path_stat;
	if (stat(completePath.c_str(), &path_stat) != 0)
		throw ErrorResponseException(404);
	return (S_ISDIR(path_stat.st_mode));
}

//! poll for read and open
void HttpRequest::methodGet(void)
{
	ssize_t charsRead;
	int fd;
	char buffer[1000];
	std::string responseBody;
	if (completePath.back() == '/' || checkPathIsDirectory() == 1)
	{
		if (!currentLocation.index.empty())
			completePath = completePath + currentLocation.index;
		else if (currentLocation.dir_listing)
		{
			ResponseBodyIsDirectoryListing();
			httpResponse.setResponseHeader("content-type", "text/html");
			httpResponse.setStatus(200);
			httpResponse.sendResponse(clientfd);
			return ;
		}
		else
			ErrorResponseException(403);
	}

	fd = open(completePath.c_str(), O_RDONLY); //nonblock?
	if (fd == -1)
		throw ErrorResponseException(404);
	while ((charsRead = read(fd, buffer, sizeof(buffer))) > 0)
		responseBody.append(buffer, charsRead);
	close(fd);
	if (charsRead == -1) //500?
		throw ErrorResponseException(500);
	setContentType();
	httpResponse.setResponseBody(responseBody);
	httpResponse.setStatus(200);
	httpResponse.sendResponse(clientfd);
}

//EPOLL!!!
void HttpRequest::methodPost(void)
{
	//post a directory?
	ssize_t charsWritten;
	int fd;

	setContentType(1);
	fd = open(completePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); //last is chmod persmissions, owner=read and write, others=read, O_CREAT???
	if (fd == -1) //500
		throw ErrorResponseException(500);
	charsWritten = write(fd, body.c_str(), body.size());
	close(fd);
	if (charsWritten == -1) //500?
		throw ErrorResponseException(500);
	httpResponse.setStatus(200);
	httpResponse.sendResponse(clientfd);
}

void HttpRequest::methodDelete(void)
{
	//delete a directory?
	bool removed;

	try 
	{
		removed = std::filesystem::remove(completePath);
		if (removed)
		{
			httpResponse.setStatus(204);
			httpResponse.sendResponse(clientfd);
		}
		else
			throw ErrorResponseException(404);
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		throw ErrorResponseException(403);
	}
}

void HttpRequest::findCurrentLocation(ServerConfig config)
{
	int longest_match_len = 0;
	int match_len = 0;
	bool match_found = false;

	for (auto location : config.locations)
	{
		if (originalPath.find(location.path) == 0) //path starts with location path
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
		throw ErrorResponseException(404);
}

void HttpRequest::checkPathIsSafe(void) //??
{
	std::filesystem::path canonicalPath;
	try 
	{
		canonicalPath = std::filesystem::weakly_canonical(completePath);
		//weakly_canonical allows us to make a path canonical, 
		//even tho it does not exist (a path does not exist when i try to POST)
	}
	catch (const std::filesystem::filesystem_error& e) 
	{
		throw ErrorResponseException(403);
	}
	if (canonicalPath.string().find(currentLocation.root) != 0)
		throw ErrorResponseException(403);
}

void HttpRequest::makeRootAbsolute(std::string& myRoot) //??
{
	std::filesystem::path root(myRoot);
	try
	{
		if (root.is_relative())
			root = std::filesystem::current_path() / root;
		if (!std::filesystem::exists(root))
			throw ErrorResponseException(404);
		myRoot = std::filesystem::canonical(root);
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		throw ErrorResponseException(403);
	}
}
void HttpRequest::setErrorPages(std::map<int, std::string> pages, std::string root)
{
	if (pages.empty())
		return ;
	for (auto& [status, path] : pages)
		path = root + path; //same name as in httprequest!!?
	errorPages = pages;
}

char HttpRequest::hexToChar(char c)
{
	if ('0' <= c && c <= '9')
		return (c - '0');
	else if ('a' <= c && c <= 'f')
		return (c - 'a' + 10);
	else if ('A' <= c && c <= 'F')
		return (c - 'A' + 10);
	//throw???
	return (0);
}

void HttpRequest::urlToRealPath(void)
{
	std::string realPath;

	for (size_t i = 0; i < originalPath.length(); ++i)
	{
		if (originalPath[i] == '%')
		{
			if (i + 2 >= originalPath.length())
				; //throw??
			char first = originalPath[i + 1];
			char second = originalPath[i + 2];
			realPath += (hexToChar(first) << 4) | hexToChar(second);
			i += 2;
		}
		else if (originalPath[i] == '+')
			realPath += " ";
		else 
			realPath += originalPath[i];
		
	}
	originalPath = realPath;
}

std::vector<char *>HttpRequest::setupCgiEnv(ServerConfig config, std::string pathInfo)
{
	std::vector<char *> envp;
	std::string header;

	envVariables.push_back("REQUEST_METHOD=" + method);
	envVariables.push_back("SCRIPT_NAME=" + originalPath);
	envVariables.push_back("SCRIPT_FILENAME=" + completePath);
	envVariables.push_back("SERVER_PROTOCOL=HTTP/1.1");
	envVariables.push_back("SERVER_NAME=" + config.server_names.at(0));
	envVariables.push_back("SERVER_PORT=" + std::to_string(config.listen_port));
	envVariables.push_back("PATH_INFO=" + pathInfo);

	if (method == "GET")
		envVariables.push_back("QUERY_STRING=" + queryString);
	else if (method == "POST")
	{
		//what if none?
		if (headers.find("content-length") != headers.end())
			header = headers.at("content-length");
		else
			header = "0"; 
		envVariables.push_back("CONTENT_LENGTH=" + header);
		if (headers.find("content-type") != headers.end())
			header = headers.at("content-type");
		else 
			header = ""; //??
		envVariables.push_back("CONTENT_TYPE=" + header);
		envVariables.push_back("REQUEST_BODY=" + body);
	}
	else
		throw ErrorResponseException(405);
	for (size_t i = 0; i < envVariables.size(); ++i)
	{
		std::cout << "ENV VAR: " << envVariables[i] << std::endl;
		envp.push_back(const_cast<char *>(envVariables[i].c_str()));
	}
	envp.push_back(nullptr);
	return (envp);
}

std::string HttpRequest::getPathInfo(int interpreterCheck)
{
	std::string pathInfo;
	int lenOfPos = 0;
	size_t posOfPathInfo = std::string::npos;
	if (interpreterCheck == 1)
	{
		posOfPathInfo = completePath.find(".php");
		lenOfPos = 4;
	}
	else if (interpreterCheck == 2)
	{
		posOfPathInfo = completePath.find(".py");
		lenOfPos = 3;
	}
	if (posOfPathInfo == std::string::npos)
		throw ErrorResponseException(500);
	if (posOfPathInfo + 1 < completePath.size())
	{
		pathInfo = completePath.substr(posOfPathInfo + lenOfPos);
		//remove "/" if its the first character?
		completePath = completePath.substr(0, posOfPathInfo + lenOfPos);
	}
	std::cout << "PATH INFO: " << pathInfo << std::endl;
	std::cout << "PATH TO SCRIPT: " << completePath << std::endl;
	return (pathInfo);
}

void HttpRequest::checkCgiPaths(std::string interpreterPath)
{
	struct stat scriptStat;
	if (stat(completePath.c_str(), &scriptStat) == -1) //file not exist or stat failed
		throw ErrorResponseException(404); //NOT ALWAYS
	if (!S_ISREG(scriptStat.st_mode))
		throw ErrorResponseException(404);
	if (access(completePath.c_str(), X_OK) == -1)
		throw ErrorResponseException(403); 

	struct stat interpreterStat;
	if (stat(interpreterPath.c_str(), &interpreterStat) == -1)
    	throw ErrorResponseException(404);
	if (!S_ISREG(interpreterStat.st_mode))
    	throw ErrorResponseException(404);
	if (access(interpreterPath.c_str(), X_OK) == -1)
		throw ErrorResponseException(403);
}

void HttpRequest::doCgi(std::string interpreterPath, ServerConfig config, int interpreterCheck, const Server& server)
{
	std::string pathInfo;
	pathInfo = getPathInfo(interpreterCheck);
	checkCgiPaths(interpreterPath);
	std::cout << "INTERPRETER PATH: " << interpreterPath << std::endl;
	std::cout << "COMPLETE PATH IN ARGV: " << completePath << std::endl;
	char *argv[] =
	{
		const_cast<char *>(interpreterPath.c_str()),
		const_cast<char *>(completePath.c_str()),
		nullptr
	};
	std::vector<char *> envp = setupCgiEnv(config, pathInfo);
	int pipeFd[2];

	if (pipe(pipeFd) == -1)
		throw ErrorResponseException(500);
	
	pid_t pid = fork();
	if (pid == -1)
	{
		close(pipeFd[0]);
		close(pipeFd[1]);
		throw ErrorResponseException(500);
	}
	//interpreterPath = "/abcd"; - do this to make execve fail
	if (pid == 0)
	{
		(void)argv;
		close(pipeFd[0]);
		if (dup2(pipeFd[1], STDOUT_FILENO) == -1)
			_Exit(1);  //ALLOWED??!!
		close (pipeFd[1]);
		execve(interpreterPath.c_str(), argv, envp.data());
		std::cerr << "Execve call fail, cleaning fds...\n";
		std::vector<int> fds_to_close = server.get_open_fds();
		for (size_t i = 0; i < fds_to_close.size(); ++i)
    		close(fds_to_close[i]);
		_Exit(1);
	}
	else
	{
		close(pipeFd[1]);

		std::string cgiOutput;
		char buffer[1000];
		ssize_t charsRead;
		while ((charsRead = read(pipeFd[0], buffer, sizeof(buffer))) > 0)
			cgiOutput.append(buffer, charsRead);
		if (charsRead == -1)
		{
			close(pipeFd[0]);
			throw ErrorResponseException(500);
		}
		
		close(pipeFd[0]);

		int status;
		if (waitpid(pid, &status, 0) == -1)
			throw ErrorResponseException(500);

		std::cout << "CHILD STATUS: " << status << std::endl;
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			throw ErrorResponseException(500);

		size_t pos = cgiOutput.find("\r\n\r\n");
		size_t findLen = 4;

		if (pos == std::string::npos) 
		{
			pos = cgiOutput.find("\n\n");
			findLen = 2;
		}
		if (pos != std::string::npos)
			cgiOutput = cgiOutput.substr(pos + findLen);

		httpResponse.setResponseHeader("content-type", "text/html"); //setContentType(); find it in output headers instead and split cgiOutput to body and headers
		httpResponse.setResponseBody(cgiOutput);
		httpResponse.setStatus(200);
		httpResponse.sendResponse(clientfd);
	}
}

void HttpRequest::checkQueryString(void)
{
	size_t pos = originalPath.find('?');
	if (pos == std::string::npos)
		return ;
	queryString = originalPath.substr(0 + pos + 1);
	originalPath = originalPath.substr(0, pos);
	std::cout << "MY QUERY: " << queryString << std::endl;
	std::cout << "MY PATH AFTER QUERY: " << originalPath << std::endl;
}

void HttpRequest::doRequest(ServerConfig config, const Server& server)
{
	try
	{
		if (path.empty())
		{
			std::cout << "no path incoming to doRequest...stopping request" << std::endl;
			return ;
		}
		std::cout << "INCOMING PATH: " << path << std::endl; 
		originalPath = path;
		path.clear();
		makeRootAbsolute(config.root);
		setErrorPages(config.error_pages, config.root);
		checkQueryString(); //where have it??!!
		//dump();
		findCurrentLocation(config);
		makeRootAbsolute(currentLocation.root);
		urlToRealPath();
		completePath = currentLocation.root + originalPath;
		checkPathIsSafe();
		std::cout << "COMPLETE PATH: " << completePath << std::endl;
		std::cout << "I PRINT CGI PATHS, PHP: " << currentLocation.cgi_path_php << " AND PYTHON: " << currentLocation.cgi_path_python << std::endl;
		if (completePath.find(".php") != std::string::npos) //check up, try std::filesystem::path(filename).extension() != ".py")
			doCgi(currentLocation.cgi_path_php, config, 1, server);
		else if (completePath.find(".py") != std::string::npos) //check up
			doCgi(currentLocation.cgi_path_python, config, 2, server);
		else if (method == "GET" && 
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
		{
			Response::buildErrorResponse(405, 1, clientfd, errorPages);
			//method not allowed
		}
	}
	catch (ErrorResponseException &e)
	{
		Response::buildErrorResponse(e.getResponseStatus(), 1, clientfd, errorPages);
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << " WAS CATCHED IN DOREQUEST!!!" << std::endl;
		Response::buildErrorResponse(500, 1, clientfd, errorPages);
	}
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

std::string HttpRequest::getMethod() const{
	return method;
}

std::string HttpRequest::getBody(){
	return body;
}

std::string HttpRequest::getHttpVersion(){
	return httpVersion;
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
    std::cout << "Body:\n" << body << "\n";
	std::cout << "Keep alive/ close boolean: " << isKeepAlive << std::endl;
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