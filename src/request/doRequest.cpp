/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   doRequest.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 12:53:48 by aalbrech          #+#    #+#             */
/*   Updated: 2025/08/06 15:15:29 by aalbrech         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/HttpRequest.hpp"


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
	while ((entry = readdir(dir)) != nullptr) //errcheck?
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
	if (formFields.find("filename") != bodyHeaders.end())
	{
		std::string nameOfNewFile = formFields.at("filename");
		if (fileNameIsSafe(nameOfNewFile))
			completePath += "/" + nameOfNewFile;
		else
			throw ErrorResponseException(400);
	}
	else 
		throw ErrorResponseException(400);
	if (bodyHeaders.find("Content-Type") != bodyHeaders.end())
		headers["content-type"] = bodyHeaders.at("Content-Type");
	else 
		headers.erase("content-type");
}

void HttpRequest::methodPost(ServerConfig config)
{
	ssize_t charsWritten;
	int fd;
	std::string requestContentType;

	if (headers.find("content-type") != headers.end())
		requestContentType = headers.at("content-type");
	if (requestContentType.find("multipart/form-data") != std::string::npos)
		postIsMultipartBody();
	if (completePath.ends_with('/'))
		throw ErrorResponseException(403);
	size_t posOfFile = completePath.rfind('/');
	if (posOfFile == std::string::npos)
		throw ErrorResponseException(500);
	else
	{
		if (!currentLocation.upload_dir.empty())
		{
			std::filesystem::path locationRoot = currentLocation.root;
			std::filesystem::path uploadHere = locationRoot / currentLocation.upload_dir;
			try
			{
				safeStat(uploadHere.string());
			}
			catch (ErrorResponseException& e) 
			{
				throw ErrorResponseException(500);
			}
			completePath = uploadHere.string() + completePath.substr(posOfFile);
		}
		else
		{
			std::string clientPathToPostTo = completePath.substr(0, posOfFile);
			safeStat(clientPathToPostTo);
		} 
	}
	try
	{
		checkPathIsSafe();
	}
	catch (ErrorResponseException& e)
	{
		throw ErrorResponseException(500);
	}
	setContentType(1);
	fd = open(completePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		throw ErrorResponseException(500);
	charsWritten = write(fd, body.c_str(), body.size());//checks for 0 
	close(fd);
	if (charsWritten == -1)
		throw ErrorResponseException(500);
	std::filesystem::path getRelativePath = std::filesystem::weakly_canonical(std::filesystem::relative(completePath, config.root));
	std::string urlLocOfPost = "/" + getRelativePath.string();
	fixMultipleSlashes(urlLocOfPost);
	encodeUrl(urlLocOfPost);
	httpResponse.setResponseHeader("content-type", "text/html");
	httpResponse.setResponseHeader("location", urlLocOfPost);
	std::string htmlBody = generateSuccessPostHtml(urlLocOfPost);
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

	std::string target = currentLocation.redirect_target;
	if (target.back() != '/')
		target += '/';

	if (target.starts_with("./"))
		target.erase(0, 2);

	std::string absOriginalPath = currentLocation.path + originalPath;
	std::string absTargetPath = target;

	if (!target.starts_with("/"))
		absTargetPath = currentLocation.path + target;

	if (absOriginalPath.back() != '/')
		absOriginalPath += '/';
	if (absTargetPath.back() != '/')
		absTargetPath += '/';

	std::string newPath;

	if (absOriginalPath.starts_with(absTargetPath)) 
		newPath = absOriginalPath;
	else 
		newPath = absTargetPath + originalPath;

	encodeUrl(newPath);

	if (!queryString.empty())
		newPath += "?" + queryString;

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
		checkQueryString();
		decodeUrl(originalPath);
		fixMultipleSlashes(originalPath);
		findCurrentLocation(config); 
		if (currentLocation.redirect_code != -1)
		{
				isRedirection();
				return (httpResponse);
		}
		if (!currentLocation.root.empty())
		{
			makeRootAbsolute(currentLocation.root);
			if (currentLocation.root.find(config.root) != 0)
				throw ErrorResponseException(500);
		}
		else
			currentLocation.root = config.root;
		std::filesystem::path origPath = originalPath;
		std::filesystem::path locPath = currentLocation.path;
		if (origPath.string().find(locPath.string()) == 0) 
		{
			std::filesystem::path remaining = origPath.lexically_relative(locPath);
			completePath = (std::filesystem::path(currentLocation.root) / remaining).lexically_normal().string();
		} 
		else 
			throw ErrorResponseException(403);
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
		throw ChildError(500);
	}
	catch (ErrorResponseException &e)
	{
		httpResponse.buildErrorResponse(e.getResponseStatus(), clientfd, errorPages);
		return (httpResponse);
	} 
	catch (std::exception& e)
	{
		httpResponse.buildErrorResponse(500, clientfd, errorPages);
		return (httpResponse);
	}
	return (httpResponse);
}

