/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 19:02:38 by llaakson          #+#    #+#             */
/*   Updated: 2025/08/18 12:45:40 by aalbrech         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <sys/wait.h>
#include <string>
#include <sstream>
#include <map>
#include <fcntl.h>
#include <filesystem>
#include "ConfigParse.hpp"
#include <cstring>
#include <sys/stat.h>
#include "Response.hpp"
#include "ErrorResponseException.hpp"
#include <string>
#include <cstdlib>

class Server;

class HttpRequest
{
	private:
		std::string method;
		std::string path;
		std::string httpVersion;
		std::map<std::string, std::string> headers;
		std::string	body;
		std::map<int, std::string> errorPages;

		int clientfd;
		LocationConfig currentLocation;
		std::string originalPath;
		std::string completePath;
		std::string queryString;
		std::vector<std::string> envVariables;
		bool isKeepAlive = true;
		size_t max_client_body_size;
		Response httpResponse;
		void methodGet(ServerConfig config);
		void methodPost(ServerConfig config);
		void methodDelete();
		void doCgi(ServerConfig config, std::string cgiExtension, const Server& server);
		void setContentType(int postCheck = 0);
		void findCurrentLocation(ServerConfig config);
		void ResponseBodyIsDirectoryListing(void);
		int checkPathIsDirectory(void);
		void checkPathIsSafe(void);
		void decodeUrl(std::string& decodeThis);
		std::vector<char *>setupCgiEnv(ServerConfig config, std::string pathInfo);
		void checkQueryString(void);
		std::string getPathInfo(std::string cgiExtension);
		void checkCgiPath(std::string checkThisPath, bool tryIsExecutable = false);
		void checkContentType(std::string responseContentType);
		void parseCgiOutput(std::string cgiOutput);
		void checkMethodAllowed();
		std::string checkRequestIsCgi(void);
		struct stat safeStat(std::string statThis);
		void isRedirection(void);
		void escapeHtml(std::string& encodeThis);
		void encodeUrl(std::string& encodeThis);
		void postIsMultipartBody();
		void fixMultipleSlashes(std::string &fixThis);

	public:
		std::map<std::string, std::string> bodyHeaders;
		std::map<std::string, std::string> formFields;
		void 		setMethod(const std::string& m);
		std::string	getMethod() const;
    	void 		setPath(const std::string& p);
    	void 		setHttpVersion(const std::string& v);
    	void 		addHeader(const std::string& key, const std::string& value);
    	void 		setBody(const std::string& b);
		void		appendBody(const std::string& data);
		void		setKeepAlive(bool isAlive);
		std::string	getHeader(const std::string& key) const;
		Response	doRequest(ServerConfig config, const Server& server);// we might not need server
		void setErrorPages(std::map<int, std::string> pages);
		std::map<int, std::string> getErrorPages(void);
		static void makeRootAbsolute(std::string& myRoot);
		
		HttpRequest(int fd);
		~HttpRequest() {};
		
		void dump() const;
};

