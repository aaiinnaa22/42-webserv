#pragma once
#include <sys/wait.h> //waitpid
#include <string>
#include <sstream>
#include <map>
#include <fcntl.h>     // open
#include <filesystem> //for remove in DELETE method, allowed?
#include "ConfigParse.hpp"
#include <dirent.h> //opendir, closedir....
#include <cstring> //strcmp
#include <sys/stat.h> //stat()
#include "Response.hpp"
#include "ErrorResponseException.hpp"
#include "string" //FOR ENDS_WITH???
#include <cstdlib>

#include "Server.hpp" //maybe remove
#include "ClientConnection.hpp"

class Server;
//class ClientConnection;

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
		//std::string responseBody;
		//std::string responseContentType;
		std::map<int, std::string> errorPages;
		int clientfd;
		LocationConfig currentLocation;
		std::string originalPath;
		std::string completePath;
		Response httpResponse;
		std::string queryString;
		std::vector<std::string> envVariables;
		bool isKeepAlive = true;
		void methodGet();
		void methodPost();
		void methodDelete();
		void doCgi(std::string interpreterPath, ServerConfig config, int interpreterCheck, const Server& server);
		void setContentType(int postCheck = 0);
		void findCurrentLocation(ServerConfig config);
		void ResponseBodyIsDirectoryListing(void);
		int checkPathIsDirectory(void);
		void checkPathIsSafe(void);
		void makeRootAbsolute(std::string& myRoot);
		void setErrorPages(std::map<int, std::string> pages, std::string root);
		void urlToRealPath(void);
		char hexToChar(char c);
		std::vector<char *>setupCgiEnv(ServerConfig config, std::string pathInfo);
		void checkQueryString(void);
		std::string getPathInfo(int interpreterCheck);
		void checkCgiPaths(std::string interpreterPath);

	public:
		void		parse(const std::string& request);
		std::string	getMethod() const;
		std::string	getPath();
		std::string	getPath() const;
		std::string	getHttpVersion();
		std::string getBody();
		const std::map<std::string, std::string>& getHeaders() const;
		void 		setMethod(const std::string& m);
    	void 		setPath(const std::string& p);
    	void 		setHttpVersion(const std::string& v);
    	void 		addHeader(const std::string& key, const std::string& value);
    	void 		setBody(const std::string& b);
		void		setKeepAlive(bool isAlive);
		void		appendBody(const std::string& data);
		std::string	getHeader(const std::string& key) const;
		void		doRequest(ServerConfig config, const Server& server);
		
		HttpRequest(int fd);
		~HttpRequest() {};
		
		void dump() const;
};

