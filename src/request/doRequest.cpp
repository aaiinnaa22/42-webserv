/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   doRequest.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 12:53:48 by aalbrech          #+#    #+#             */
/*   Updated: 2025/08/04 20:09:58 by aalbrech         ###   ########.fr       */
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
		std::string strEntryLink(entry->d_name);
		std::string strEntryName = strEntryLink;
		encodeUrl(strEntryLink);
		escapeHtml(strEntryName);
		html_content = "<li><a href=\"" + strEntryLink + "\">" + strEntryName + "</a></li>\n"; 
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
	//what if both dir listing and index is on in conf?
	std::cout << "hello from method GET" << std::endl;
	ssize_t charsRead;
	int fd;
	char buffer[1000];
	std::string responseBody;
	if (checkPathIsDirectory() == 1)
	{
		if (!currentLocation.index.empty())
		{
			completePath += currentLocation.index;
			std::cout << "index html path: " << completePath << std::endl;
			try 
			{
				checkPathIsSafe();
			}
			catch (ErrorResponseException& e) 
			{
				throw ErrorResponseException(500);
			}
		}
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
	while ((charsRead = read(fd, buffer, sizeof(buffer))) > 0)//READ CHECKS
		responseBody.append(buffer, charsRead);
	close(fd);
	if (charsRead == -1)
		throw ErrorResponseException(500);
	setContentType();
	httpResponse.setResponseBody(responseBody);
	httpResponse.setStatus(200);
}

static bool fileNameIsSafe(std::string fileName)
{
	//url safe naming?
	for (auto c : fileName)
	{
		if (!isalpha(c) && !isdigit(c) && c != '.' && c != '-' && c != '_')
			return (false);
	}
	return (true);
}

static std::string generateSuccessPostHtml(std::string pathToNewFile)
{
	std::string html = 
		"<!DOCTYPE html>\n"
		"<html lang=\"en\">\n"
		"<head>\n"
		"  <meta charset=\"UTF-8\" />\n"
		"  <title>Upload Successful</title>\n"
		"</head>\n"
		"<body>\n"
		"  <h1>File Uploaded Successfully</h1>\n"
		"  <p>Your file was uploaded and is now available at:</p>\n"
		"  <a href=\"" + pathToNewFile + "\">View the uploaded file</a>\n"
		"</body>\n"
		"</html>\n";

    return html;
}

void HttpRequest::postIsMultipartBody()
{
	//make a dir or what? always /upload? 
	if (formFields.find("filename") != bodyHeaders.end())
	{
		std::string nameOfNewFile = formFields.at("filename"); //what if name is ../../?
		if (fileNameIsSafe(nameOfNewFile))
			completePath += "/" + nameOfNewFile;
		else 
			throw ErrorResponseException(400); //bad request?
	}
	else 
		throw ErrorResponseException(400); //?
	if (bodyHeaders.find("Content-Type") != bodyHeaders.end())
		headers["content-type"] = bodyHeaders.at("Content-Type");
	else 
		headers.erase("content-type");
}

void HttpRequest::methodPost(ServerConfig config) //has to get changed for web browser requests (multipart body)
{
	ssize_t charsWritten;
	int fd;
	std::string requestContentType;

	std::cout << "TIME TO POST!" << std::endl;
	if (headers.find("content-type") != headers.end())
		requestContentType = headers.at("content-type");
	if (requestContentType.find("multipart/form-data") != std::string::npos) //enough?
		postIsMultipartBody();
	if (completePath.ends_with('/'))
		throw ErrorResponseException(403);
	size_t posOfFile = completePath.rfind('/');
	if (posOfFile != std::string::npos) //what if npos? or empty?
	{
		if (!currentLocation.upload_dir.empty()) //what if multipart?
		{
			std::filesystem::path locationRoot = currentLocation.root;
			std::filesystem::path uploadHere = locationRoot / currentLocation.upload_dir;
			completePath = uploadHere.string() + completePath.substr(posOfFile);
		}
		std::cout << "PATH TO POST: " << completePath << std::endl;
		posOfFile = completePath.rfind('/'); //what if no pos or empty?
		if (posOfFile != std::string::npos)
		{
			std::string pathToPostTo = completePath.substr(0, posOfFile + 1);
			safeStat(pathToPostTo);
		}
	}
	try
	{
		std::cout << "this path is safe?: " << completePath << std::endl;
		checkPathIsSafe();
	}
	catch (ErrorResponseException& e) //the upload dir root is bad or filename for multipart is bad?
	{
		std::cout << "check path is safe failed with code " << e.getResponseStatus() << std::endl;
		throw ErrorResponseException(500);
	}
	setContentType(1); //to check content type of file is valid
	//truncate??
	fd = open(completePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); //last is chmod persmissions, owner=read and write, others=read, O_CREAT???
	if (fd == -1)
		throw ErrorResponseException(500);
	charsWritten = write(fd, body.c_str(), body.size());//checks for 0 
	close(fd);
	if (charsWritten == -1)
		throw ErrorResponseException(500);
	std::filesystem::path getRelativePath = std::filesystem::relative(completePath, config.root);
	//errcheck?
	std::string relativePath = "/" + getRelativePath.string();
	std::cout << "REALTIVE PATH: " << relativePath << std::endl;
	fixMultipleSlashes(relativePath);
	encodeUrl(relativePath);
	httpResponse.setResponseHeader("content-type", "text/html");
	httpResponse.setResponseHeader("location", relativePath);
	std::string htmlBody = generateSuccessPostHtml(relativePath);
	httpResponse.setResponseBody(htmlBody);
	httpResponse.setStatus(201);
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
	if (currentLocation.redirect_code < 301 || currentLocation.redirect_code > 308)
		throw ErrorResponseException(500);
	if (currentLocation.redirect_code > 303 && currentLocation.redirect_code < 307)
		throw ErrorResponseException(500); 
	fixMultipleSlashes(currentLocation.redirect_target);
	if (originalPath.starts_with(currentLocation.path))
		originalPath.erase(0, currentLocation.path.size());
	std::string newPath = currentLocation.redirect_target + originalPath;
	encodeUrl(newPath);
	if (!queryString.empty())
		newPath += "?" + queryString;
	std::cout << "NEW PATH :" << newPath << std::endl;
	std::cout << "STATUS: " << currentLocation.redirect_code << std::endl;
	httpResponse.setResponseHeader("location", newPath);
	httpResponse.setStatus(currentLocation.redirect_code);
}

Response HttpRequest::doRequest(ServerConfig config, const Server& server)
{
	//cleanup multiple slashes?
	//when to add query string??!
	dump();
	try
	{
		originalPath = path;
		path.clear();
		
		makeRootAbsolute(config.root);
		setErrorPages(config.error_pages, config.root);
		checkQueryString();
		decodeUrl(originalPath);
		fixMultipleSlashes(originalPath);
		findCurrentLocation(config);
		std::cout << "BASED ON " << originalPath << ", current loc is: " << currentLocation.path << std::endl; 
		if (currentLocation.redirect_code != -1)
		{
				isRedirection();
				return (httpResponse);
		}
		//checkQueryString();
		//decodeUrl(originalPath);
		if (!currentLocation.root.empty())
		{
			makeRootAbsolute(currentLocation.root);
			if (currentLocation.root.find(config.root) != 0)
			{
				std::cout << "LOC ROOT OUTSIDE OF SERVER ROOT" << std::endl;
				throw ErrorResponseException(500);
			}
		}
		else
			currentLocation.root = config.root;
		//ALWAYS USE STD::FILEPATH FOR PATHS? instead of std::string
		std::cout << "CURRENT LOC IS : " << currentLocation.root << std::endl;
		std::cout << "ORIGINAL PATH IS : " << originalPath << std::endl;
		std::string pathWithoutLoc = originalPath.substr(currentLocation.path.length());
		std::filesystem::path pathToUse = std::filesystem::path(currentLocation.root) / pathWithoutLoc;
		completePath = pathToUse.string();
		std::cout << "COMPLETE PATH: " << completePath << std::endl;
		checkPathIsSafe();
		checkMethodAllowed();
		if (headers.find("content-type") != headers.end())
		{
			std::string contentTypeValue = headers.at("content-type");
			if (contentTypeValue.find("application/x-www-form-urlencoded") != std::string::npos)
				decodeUrl(body);
		}
		max_client_body_size = config.max_client_body_size;
		std::string cgiExtension = checkRequestIsCgi();
		if (cgiExtension != "")
			doCgi(config, cgiExtension, server);
		else if (method == "GET")
			methodGet();
		else if (method == "POST")
			methodPost(config);
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

