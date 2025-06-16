#pragma once
#include <string>
#include <sstream>
#include <map>
#include <fcntl.h>     // open
#include <filesystem> //for remove in DELETE method, allowed?
#include "ConfigParse.hpp"

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
		LocationConfig currentLocation;
		void methodGet();
		void methodPost();
		void methodDelete();
		void doCgi();
		void sendResponse(std::string status);
		void setContentType(std::string path);
		void findCurrentLocation(ServerConfig config);
		void ResponseBodyIsDirectoryListing(void);

	public:
		void		parse(const std::string& request);
		std::string	getMethod();
		std::string	getPath();
		std::string	getHttpVersion();
		const std::map<std::string, std::string>& getHeaders() const;
		void 		setMethod(const std::string& m);
    	void 		setPath(const std::string& p);
    	void 		setHttpVersion(const std::string& v);
    	void 		addHeader(const std::string& key, const std::string& value);
    	void 		setBody(const std::string& b);
		std::string	getHeader(const std::string& key) const;
		void		doRequest(ServerConfig config);
		
		HttpRequest(int fd);
		
		void dump() const;
};
