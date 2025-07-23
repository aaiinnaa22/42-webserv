#include "../inc/Server.hpp"
#include "../inc/HttpRequest.hpp"

//Aina


//param constructor with client fd
HttpRequest::HttpRequest(int fd) :clientfd(fd) {}


void HttpRequest::checkContentType(std::string responseContentType)
{
	//make cleaner? like parse into actual header values instead of just
	//trying to find within a string?
	if (headers.find("accept") != headers.end())
	{
		std::string acceptTheseContentTypes = headers.at("accept");
		if (acceptTheseContentTypes.find("*/*") != std::string::npos)
			return ;
		size_t pos = responseContentType.find('/');
		if (pos != std::string::npos)
		{
			std::string wildCard;
			wildCard = responseContentType.substr(0, pos + 1);
			wildCard += '*';
			if (acceptTheseContentTypes.find(wildCard) != std::string::npos)
				return ;
		}
		if (acceptTheseContentTypes.find(responseContentType) != std::string::npos)
			return ;
		throw ErrorResponseException(406);
	}
}

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
		//DOES NOT WORK??!!
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
	checkContentType(responseContentType);
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
	struct stat path_stat = safeStat(completePath);
	std::cout << "IS PATH DIR?!" << std::endl;
	return (S_ISDIR(path_stat.st_mode));
}

//! poll for read and open
void HttpRequest::methodGet(void)
{
	ssize_t charsRead;
	int fd;
	char buffer[1000];
	std::string responseBody;
	if (checkPathIsDirectory() == 1)
	{
		std::cout << "path is directory in method get" << std::endl; 
		if (!currentLocation.index.empty())
			completePath = completePath + currentLocation.index;
		else if (currentLocation.dir_listing)
		{
			ResponseBodyIsDirectoryListing();
			checkContentType("text/html");
			httpResponse.setResponseHeader("content-type", "text/html");
			httpResponse.setStatus(200);
			//httpResponse.sendResponse(clientfd);
			return ;
		}
		else
			throw ErrorResponseException(403);
	}

	fd = open(completePath.c_str(), O_RDONLY); //nonblock?
	if (fd == -1)
		throw ErrorResponseException(404);
	while ((charsRead = read(fd, buffer, sizeof(buffer))) > 0)
		responseBody.append(buffer, charsRead);
	close(fd);
	if (charsRead == -1) //500?
	{
		std::cout << "do we fail from method get??\n";
		throw ErrorResponseException(500);
	}
	setContentType();
	httpResponse.setResponseBody(responseBody);
	httpResponse.setStatus(200);
	//httpResponse.sendResponse(clientfd);
}

//EPOLL!!!
void HttpRequest::methodPost(void) //has to get changed for web browser requests (multipart body)
{
	//post a directory?
	ssize_t charsWritten;
	int fd;

	//dont allow to post to directory in case of "normal request" (aka not multipart)
	setContentType(1);
	fd = open(completePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); //last is chmod persmissions, owner=read and write, others=read, O_CREAT???
	if (fd == -1) //500?
		throw ErrorResponseException(500);
	charsWritten = write(fd, body.c_str(), body.size());
	close(fd);
	if (charsWritten == -1) //500?
		throw ErrorResponseException(500);
	httpResponse.setStatus(200);
	//httpResponse.sendResponse(clientfd);
}

void HttpRequest::methodDelete(void)
{
	bool removed;

	if (checkPathIsDirectory() == 1)
		throw ErrorResponseException(405); //method not allowed?
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

void HttpRequest::decodeUrl(std::string& decodeThis)
{
	std::string decoded;

	for (size_t i = 0; i < decodeThis.length(); ++i)
	{
		if (decodeThis[i] == '%')
		{
			if (i + 2 >= decodeThis.length())
			{
				; //throw??
			}
			char first = decodeThis[i + 1];
			char second = decodeThis[i + 2];
			decoded += (hexToChar(first) << 4) | hexToChar(second);
			i += 2;
		}
		else if (decodeThis[i] == '+') //???
			decoded += " ";
		else 
			decoded += decodeThis[i];
		
	}
	decodeThis = decoded;
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
		if (headers.find("content-length") != headers.end())
			header = headers.at("content-length");
		else
			header = "0"; 
		envVariables.push_back("CONTENT_LENGTH=" + header);
		if (headers.find("content-type") != headers.end())
			header = headers.at("content-type");
		else 
			header = "";
		envVariables.push_back("CONTENT_TYPE=" + header);
	}
	for (size_t i = 0; i < envVariables.size(); ++i)
	{
		std::cout << "ENV VAR: " << envVariables[i] << std::endl;
		envp.push_back(const_cast<char *>(envVariables[i].c_str()));
	}
	envp.push_back(nullptr);
	return (envp);
}

std::string HttpRequest::getPathInfo(std::string cgiExtension)
{
	std::string pathInfo;
	int lenOfPos = 0;
	size_t posOfPathInfo = std::string::npos;
	if (cgiExtension == ".php")
	{
		posOfPathInfo = completePath.find(".php");
		lenOfPos = 4;
	}
	else if (cgiExtension == ".py")
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

struct stat HttpRequest::safeStat(std::string statThis)
{
	struct stat st;
	if (stat(statThis.c_str(), &st) == -1)
	{
		if (errno == ENOENT || errno == ENOTDIR)
			throw ErrorResponseException(404);
		if (errno == EACCES)
			throw ErrorResponseException(403);
		if (errno == ENAMETOOLONG)
			throw ErrorResponseException(414);
		else
		{
			std::cout << "ERRROOOOORR: " << strerror(errno) << std::endl;
			throw ErrorResponseException(500);
		}
	}
	return (st);
}

void HttpRequest::checkCgiPath(std::string checkThisPath)
{
	struct stat pathStat;
	pathStat = safeStat(checkThisPath);
	if (!S_ISREG(pathStat.st_mode))
		throw ErrorResponseException(404);
	if (access(checkThisPath.c_str(), X_OK) == -1)
		throw ErrorResponseException(403);
}

void HttpRequest::sendCgiOutput(std::string cgiOutput)
{
	//miniparser for cgioutput
	std::string cgiBody;
	std::string cgiHeaders;

	size_t pos = cgiOutput.find("\r\n\r\n");
	size_t findLen = 4;

	if (pos == std::string::npos) 
	{
		pos = cgiOutput.find("\n\n");
		findLen = 2;
	}
	if (pos != std::string::npos)
		cgiBody = cgiOutput.substr(pos + findLen);
	if (pos == std::string::npos)
		throw ErrorResponseException(500);


	cgiHeaders = cgiOutput.substr(0, pos + (findLen / 2));

	std::string contentType;
	std::string contentTypeHeader;
	std::string contentTypeValue;
	std::string status;
	size_t endOfHeader = std::string::npos;
	int intStatus = -1;
	std::cout << "CGI HEADERS OUTPUT: \"" << cgiHeaders << "\"" << std::endl;
	//if no status, default to 200 ok
	pos = cgiHeaders.find("Status"); //check that the line is correctly formatted? same goes for the other header
	if (pos != std::string::npos)
	{
		if (findLen == 2)
			endOfHeader = cgiHeaders.find('\n', pos);
		else if (findLen == 4)
			endOfHeader = cgiHeaders.find("\r\n", pos);
		if (endOfHeader != std::string::npos)
		{
			if (pos != 0 || (pos > 0 && cgiHeaders.at(pos - 1) != '\n')) //?only newline before?
			{
				std::cout << "something was before Status!! THROWINg" << std::endl;
				throw ErrorResponseException(500);
			}
			status = cgiHeaders.substr(pos, endOfHeader);
			size_t delimitor = status.find(": ");
			if (delimitor != std::string::npos)
			{
				if (status.substr(0, delimitor) != "Status")
				{
					std::cout << "SOMETHING WAS AFTER STATUS BUT BEFOre COLON" << std::endl;
					throw ErrorResponseException(500);
				}
				status = status.substr(delimitor + 2);
				status.erase(status.begin(), std::find_if(status.begin(), status.end(), [](unsigned char ch) {
        			return !std::isspace(ch);
   					}));
				if (!std::all_of(status.begin(), status.end(), [](unsigned char c){return std::isdigit(c);}))
						throw ErrorResponseException(500);
				std::cout << "THIS IS THE STR STATUS FROM CGI: " << status << std::endl;
				intStatus = std::stoi(status); //overflow?
			}
			else 
			{
				throw ErrorResponseException(500);
			}
		}
		else
			throw ErrorResponseException(500);
	}
	if (intStatus == -1)
		intStatus = 200;
	if (intStatus > 599 || intStatus < 100)
		throw ErrorResponseException(500);
	if (intStatus != 200)
		throw ErrorResponseException(intStatus);

	//if no content type -> THROW 500
	pos = cgiHeaders.find("Content-Type");
	if (pos != std::string::npos)
	{
		if (findLen == 2)
			endOfHeader = cgiHeaders.find("\n", pos);
		else if (findLen == 4)
			endOfHeader = cgiHeaders.find("\r\n", pos);
		if (endOfHeader != std::string::npos)
		{
			contentType = cgiHeaders.substr(pos, endOfHeader);
			size_t delimitor = contentType.find(":");
			if (delimitor != std::string::npos)
			{
				contentType.erase(
    				std::remove_if(contentType.begin(), contentType.end(), [](unsigned char c) {
        			return std::isspace(c);}), contentType.end());
				contentTypeHeader = contentType.substr(0, delimitor);
				contentTypeValue = contentType.substr(delimitor + 1);
				if (contentTypeHeader != "Content-Type")
				{
					throw ErrorResponseException(500);
				}
				for (auto &c : contentTypeHeader)
					c = tolower(c);
			}
			else 
				throw ErrorResponseException(500);
		}
		else 
			throw ErrorResponseException(500);
	}
	else 
		throw ErrorResponseException(500);
	//error? in case of npos

	std::cout << "CGI PARSING IS DONE\nSTATUS: " << std::to_string(intStatus) << "\nCONTENT TYPE HEADER: \"" 
	<< contentTypeHeader << "\"\nCONTENT TYPE VALUE: \"" << contentTypeValue << "\"" << std::endl;
	std::cout << "CGI RESPONSE BODY: " << cgiBody << std::endl;
	checkContentType(contentTypeValue);
	httpResponse.setResponseHeader(contentTypeHeader, contentTypeValue);
	httpResponse.setResponseBody(cgiBody);
	httpResponse.setStatus(200);
	//httpResponse.sendResponse(clientfd);
}

void HttpRequest::doCgi(ServerConfig config, std::string cgiExtension, const Server& server)
{
	if (method != "GET" && method != "POST")
		throw ErrorResponseException(405);

	std::string pathInfo;
	ssize_t charsWritten;
	std::string interpreterPath;
	if (cgiExtension == ".py")
		interpreterPath = currentLocation.cgi_path_python;
	else if (cgiExtension == ".php")
		interpreterPath = currentLocation.cgi_path_php; 

	checkCgiPath(interpreterPath);
	pathInfo = getPathInfo(cgiExtension);
	std::cout << "INTERPRETER PATH: " << interpreterPath << std::endl;
	std::cout << "COMPLETE PATH IN ARGV: " << completePath << std::endl;
	char *argv[] =
	{
		const_cast<char *>(interpreterPath.c_str()),
		const_cast<char *>(completePath.c_str()),
		nullptr
	};
	std::vector<char *> envp = setupCgiEnv(config, pathInfo);
	
	//REMOVE THE TEMP FILES AFTER USE??!
	int stdinWriteFd = open("tempStdin", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (stdinWriteFd == -1)
		throw ErrorResponseException(500);
	charsWritten = write(stdinWriteFd, body.c_str(), body.size());
	if (charsWritten == -1)
	{
		close(stdinWriteFd);
		throw ErrorResponseException(500);
	}
	close (stdinWriteFd);
	int stdinFd;
	int stdoutFd;
	stdinFd = open("tempStdin", O_RDONLY);
	if (stdinFd == -1)
		throw ErrorResponseException(500);
	stdoutFd = open("tempStdout", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (stdoutFd == -1)
	{
		close(stdinFd);
		throw ErrorResponseException(500);
	}
	pid_t pid = fork();
	if (pid == -1)
		throw ErrorResponseException(500);
	// interpreterPath = "/abcd"; //- do this to make execve fail
	if (pid == 0)
	{
		(void)argv;
		(void)server;
		try
		{
			if (dup2(stdinFd, STDIN_FILENO) == -1)
			{
				close(stdinFd);
				close(stdoutFd);
				throw ChildError(500, "dup2");
			}
			if (dup2(stdoutFd, STDOUT_FILENO) == -1)
			{
				close(stdinFd);
				close(stdoutFd);
				throw ChildError(500, "dup2");
			}
			close(stdinFd);
			close(stdoutFd);
			execve(interpreterPath.c_str(), argv, envp.data());
			std::cerr << "Execve call fail, cleaning fds...\n";
			throw ChildError(500, "execve");
			
			// std::vector<int> fds_to_close = server.get_open_fds();
			// for (size_t i = 0; i < fds_to_close.size(); ++i)
			// 	close(fds_to_close[i]);
			// server.~Server();
		}
		catch (ChildError& e)
		{
			throw ChildError(500);
		}
	}
	else
	{
		close(stdinFd);
		close(stdoutFd);
		int status;
		if (waitpid(pid, &status, 0) == -1)
			throw ErrorResponseException(500);
		std::cout << "CHILD STATUS: " << status << std::endl;
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			throw ErrorResponseException(500);

		int stdoutReadFd = open("tempStdout", O_RDONLY);
		if (stdoutReadFd == -1)
			throw ErrorResponseException(500);
		std::string cgiOutput;
		char buffer[1000];
		ssize_t charsRead;
		//read from stdout
		while ((charsRead = read(stdoutReadFd, buffer, sizeof(buffer))) > 0)
			cgiOutput.append(buffer, charsRead);
		if (charsRead == -1)
		{
			close(stdoutReadFd);
			throw ErrorResponseException(500);
		}
		close(stdoutReadFd);
		sendCgiOutput(cgiOutput);
	}
}

void HttpRequest::checkQueryString(void)
{
	size_t pos = originalPath.find('?');
	if (pos == std::string::npos)
		return ;
	queryString = originalPath.substr(0 + pos + 1);
	originalPath = originalPath.substr(0, pos);
}

void HttpRequest::checkMethodAllowed()
{
	if (std::find(currentLocation.methods.begin(), currentLocation.methods.end(), method) != 
			currentLocation.methods.end())
		return ;
	throw ErrorResponseException(405);
}

std::string HttpRequest::checkRequestIsCgi(void)
{	
	size_t pos = completePath.size();
	bool isCgi = false;
	std::string cgiExtension;
	while (pos > 0)
	{
		std::string pathToTry = completePath.substr(0, pos);
		try 
		{
			checkCgiPath(pathToTry);
		}
		catch (ErrorResponseException& e)
		{
			if (e.getResponseStatus() == 500)
				throw ErrorResponseException(500);
			pos = completePath.rfind('/', pos -1);
			continue ;
		}
		if (pathToTry.ends_with(".py"))
		{
			isCgi = true;
			cgiExtension = ".py";
			break ;
		}
		else if (pathToTry.ends_with(".php"))
		{
			isCgi = true;
			cgiExtension = ".php";
			break ;
		}
		pos = completePath.rfind('/', pos -1); //size_t can become huge in case of negative value!!!
	}
	if (isCgi == false)
		return ("");
	if (cgiExtension == ".py" && !currentLocation.cgi_path_python.empty())
		return (cgiExtension);
	if (cgiExtension == ".php" && !currentLocation.cgi_path_php.empty())
		return (cgiExtension);
	return ("");
}

void HttpRequest::sendRedirection(void)
{
	std::cout << "HELLO FROM SEND_REDIRECTION" << std::endl;
	//what if no location? or location but no redirect code?
	if (originalPath.starts_with(currentLocation.path))
		originalPath.erase(0, currentLocation.path.size());
	std::string newPath = currentLocation.redirect_target + originalPath;
	std::cout << "NEW PATH :" << newPath << std::endl;
	std::cout << "STATUS: " << currentLocation.redirect_code << std::endl;
	httpResponse.setResponseHeader("location", newPath);
	httpResponse.setStatus(currentLocation.redirect_code);
	//httpResponse.sendResponse(clientfd);
}

Response HttpRequest::doRequest(ServerConfig config, const Server& server)
{
	//TRY OUT!
	//INCOMING PATH: /grr/
	//COMPLETE PATH: /home/aalbrech/aina_gits/42-webserv/aina_website/grr/grr/

	dump();
	try
	{
		//if (path.empty())
		//{
		//	std::cout << "no path incoming to doRequest...stopping request" << std::endl;
		//	return(httpResponse);
		//}
		std::cout << "INCOMING PATH: " << path << std::endl; 
		originalPath = path;
		path.clear();
		makeRootAbsolute(config.root);
		setErrorPages(config.error_pages, config.root);
		findCurrentLocation(config);
		std::cout << "REDIR CODE: " << currentLocation.redirect_code << std::endl;
		std::cout << "REDIR PATH: " << currentLocation.redirect_target << std::endl;
		if (currentLocation.redirect_code != -1)
		{
				sendRedirection();
				return (httpResponse);
		}
		decodeUrl(originalPath);
		checkQueryString(); //where have it??!!
		makeRootAbsolute(currentLocation.root);
		completePath = currentLocation.root + originalPath;
		std::cout << "COMPLETE PATH: " << completePath << std::endl;
		std::cout << "QUERY STRING: " << queryString << std::endl;
		checkPathIsSafe();
		checkMethodAllowed();
		if (headers.find("content-type") != headers.end())
		{
			std::string contentTypeValue = headers.at("content-type");
			if (contentTypeValue.find("application/x-www-form-urlencoded") != std::string::npos)
				decodeUrl(body);
		}
		std::string cgiExtension = checkRequestIsCgi();
		if (cgiExtension != "")
		{
			std::cout << "WE ARE DOING CGI NOW!" << std::endl;
			doCgi(config, cgiExtension, server);
			return (httpResponse);
		}
		else if (method == "GET")
		{
			std::cout << "WE ARE DOING METHOD GET" << std::endl;
			methodGet();
		}
		else if (method == "POST")
			methodPost();
		else if (method == "DELETE")
			methodDelete();
		else
			httpResponse.buildErrorResponse(405, clientfd, errorPages);
	}
	catch (ChildError& e)
	{
		std::cerr << "do we get here2\n";
		throw ChildError(500);
	}
	catch (ErrorResponseException &e)
	{
		std::cout << "do we get here1\n";
		std::cout << "ERROR CATCHED, ERRNO: " << strerror(errno) << ", ERROR STATUS: " << e.getResponseStatus() << std::endl;
		httpResponse.buildErrorResponse(e.getResponseStatus(), clientfd, errorPages);
		std::cout << httpResponse.getStatusCode() << " and " << httpResponse.getStatusMessage() << " resp rest\n";
		return (httpResponse);
	} 
	catch (std::exception& e)
	{
		std::cout << e.what() << " WAS CATCHED IN DOREQUEST!!!" << std::endl;
		httpResponse.buildErrorResponse(500, clientfd, errorPages);
		return (httpResponse);
	}
	return (httpResponse);
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