/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   doRequest.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 12:53:48 by aalbrech          #+#    #+#             */
/*   Updated: 2025/07/24 15:21:53 by aalbrech         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/HttpRequest.hpp"

void HttpRequest::setErrorPages(std::map<int, std::string> pages, std::string root)
{
	if (pages.empty())
		return ;
	for (auto& [status, errorPath] : pages)
		errorPath = root + errorPath;
	errorPages = pages;
}

void HttpRequest::checkMethodAllowed()
{
	if (std::find(currentLocation.methods.begin(), currentLocation.methods.end(), method) != 
			currentLocation.methods.end())
		return ;
	throw ErrorResponseException(405);
}


void HttpRequest::ResponseBodyIsDirectoryListing(void)
{
	std::string html_content;
	DIR* dir;
	std::string responseBody;

	dir = opendir(completePath.c_str());
	if (dir == nullptr)
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
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || entry->d_name[0] == '.')
			continue ;
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



//! poll for read and open
void HttpRequest::methodGet(void)
{
	ssize_t charsRead;
	int fd;
	char buffer[1000];
	std::string responseBody;
	if (checkPathIsDirectory() == 1)
	{
		if (!currentLocation.index.empty())
			completePath = completePath + currentLocation.index;
		else if (currentLocation.dir_listing)
		{
			ResponseBodyIsDirectoryListing();
			checkContentType("text/html");
			httpResponse.setResponseHeader("content-type", "text/html");
			httpResponse.setStatus(200);
			return ;
		}
		else
			throw ErrorResponseException(403);
	}

	fd = open(completePath.c_str(), O_RDONLY);
	if (fd == -1)
		throw ErrorResponseException(404);
	while ((charsRead = read(fd, buffer, sizeof(buffer))) > 0)
		responseBody.append(buffer, charsRead);
	close(fd);
	if (charsRead == -1)
		throw ErrorResponseException(500);
	setContentType();
	httpResponse.setResponseBody(responseBody);
	httpResponse.setStatus(200);
}


void HttpRequest::methodPost(void) //has to get changed for web browser requests (multipart body)
{
	//post a directory?
	ssize_t charsWritten;
	int fd;

	//dont allow to post to directory in case of "normal request" (aka not multipart)
	setContentType(1);
	//truncate??
	fd = open(completePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); //last is chmod persmissions, owner=read and write, others=read, O_CREAT???
	if (fd == -1)
		throw ErrorResponseException(500);
	charsWritten = write(fd, body.c_str(), body.size());
	close(fd);
	if (charsWritten == -1)
		throw ErrorResponseException(500);
	httpResponse.setStatus(200);
}

void HttpRequest::methodDelete(void)
{
	bool removed;

	if (checkPathIsDirectory() == 1)
		throw ErrorResponseException(405);
	try 
	{
		removed = std::filesystem::remove(completePath);
		if (removed)
		{
			httpResponse.setStatus(204);
			return ;
		}
		else
			throw ErrorResponseException(404);
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		throw ErrorResponseException(403);
	}
}


void HttpRequest::isRedirection(void)
{
	if (originalPath.starts_with(currentLocation.path))
		originalPath.erase(0, currentLocation.path.size());
	std::string newPath = currentLocation.redirect_target + originalPath;
	std::cout << "NEW PATH :" << newPath << std::endl;
	std::cout << "STATUS: " << currentLocation.redirect_code << std::endl;
	httpResponse.setResponseHeader("location", newPath);
	httpResponse.setStatus(currentLocation.redirect_code);
}

Response HttpRequest::doRequest(ServerConfig config, const Server& server)
{
	dump();
	try
	{
		originalPath = path;
		path.clear();
		makeRootAbsolute(config.root);
		setErrorPages(config.error_pages, config.root);
		findCurrentLocation(config);
		if (currentLocation.redirect_code != -1)
		{
				isRedirection();
				return (httpResponse);
		}
		decodeUrl(originalPath);
		checkQueryString();
		makeRootAbsolute(currentLocation.root);
		completePath = currentLocation.root + originalPath;
		std::cout << "COMPLETE PATH: " << completePath << std::endl;
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
			doCgi(config, cgiExtension, server);
		else if (method == "GET")
			methodGet();
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
		httpResponse.buildErrorResponse(e.getResponseStatus(), clientfd, errorPages);
		return (httpResponse);
	} 
	catch (std::exception& e)
	{
		std::cout << e.what() << " WAS CATCHED IN DOREQUEST!" << std::endl;
		httpResponse.buildErrorResponse(500, clientfd, errorPages);
		return (httpResponse);
	}
	return (httpResponse);
}

