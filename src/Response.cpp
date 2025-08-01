/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/06 13:19:50 by hskrzypi          #+#    #+#             */
/*   Updated: 2025/07/23 15:52:26 by aalbrech         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Response.hpp"

const std::unordered_map<int, std::string> Response::reasonPhrases = {
	{200, "OK"},
	{204, "No Content"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
	{406, "Not Acceptable"},
    {405, "Method Not Allowed"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {411, "Length Required"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {418, "I'm a teapot"},
    {431, "Request Header Fields Too Large"},
	{301, "Moved Permanently"},
	{302, "Found"},
	{303, "See Other"},
	{307, "Temporary Redirect"},
	{308, "Permanent Redirect"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {503, "Service Unavailable"},
    {505, "HTTP Version Not Supported"}
};

void Response::setStatus(int code, const std::string& message)
{
	statusCode = code;

	if (!message.empty()) {
		statusMessage = message;
		return;
	}
	auto it = reasonPhrases.find(code);
	if (it != reasonPhrases.end())
		statusMessage = it->second;
	else 
	{
		statusCode = 418;
		statusMessage = "I'm a teapot";
	}
}

void Response::setResponseHeader(const std::string &key, const std::string &value)
{
	headers[key] = value;
}

void Response::setResponseBody(std::string &bodyContent)
{
	body = bodyContent;
}

int Response::getStatusCode() const
{
	return statusCode;
}

std::string Response::getStatusMessage() const
{
	return statusMessage;
}

const std::string Response::getHeader(const std::string &key) const
{
    auto it = headers.find(key);
    return (it != headers.end()) ? it->second : "";
}

std::string Response::getBody() const
{
	return body;
}

std::string Response::toString() const
{
	std::string response;
	response += httpVersion + " " + std::to_string(statusCode) + " " + statusMessage + "\r\n";

  	for (const auto& header : headers)
        response += header.first + ": " + header.second + "\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "\r\n";
    response += body;

    return response;
}


void Response::buildErrorResponse(int statusCode, int clientFd, std::map<int, std::string> chosenErrorPages)
{
	(void)clientFd;
	//WHAT IF text/html is not allowed in request headers???
	isSent = false;
	std::cout << "build error response call\n";
	setStatus(statusCode);
	setResponseHeader("content-type", "text/html");\
	std::string responseBody;
	std::map<int, std::string>::const_iterator it = chosenErrorPages.find(statusCode);
	if (it != chosenErrorPages.end())
		responseBody = it->second;
	else 
	{
		errorPages = getSetDefaultErrorPages();
		std::map<int, std::string>::const_iterator it = errorPages.find(statusCode);
		if (it != errorPages.end())
			responseBody = it->second;
	}
	if (responseBody.empty())
	{
		responseBody = "<h1>500 Internal Server Error</h1>";
		setStatus(500);
	} 	
	

	
	// std::cout << "build error test\n";
	// std::cout << statusCode << "-what funcrion received\n";
	// std::cout << res.getStatusCode() << "-what is inside response\n";
	// std::string errorFile;
	// if (!errorPages.empty() && errorPages.find(statusCode) != errorPages.end())
	// 	errorFile = errorPages[statusCode];
	// else
	// 	errorFile = "./www/error/" + std::to_string(statusCode) + ".html";
	//ssize_t chars_read;
	//char buffer[1000];
	// std::string responseBody;
	// std::map<int, std::string>::const_iterator it = errorPages.find(statusCode);
	// if (it != errorPages.end())
	// 	responseBody = it->second;
	//epoll?
	// std::cout << "TRYING TO OPEN ERROR FILE " << errorFile << std::endl;
	// int fd = open(errorFile.c_str(), O_RDONLY);
	// if (fd == -1) //but it means not found?!
	// {
	// 	std::cout << "BUiLD ERROR ERROR FILE COULDNT GET OPENED" << std::endl;
	// 	std::string safetyError = "<h1>500 - Internal Server Error</h1>";
	// 	setResponseBody(safetyError);
	// 	setStatus(500);
	// 	// if (sendNow)
	// 	// 	res.sendResponse(clientFd);
	// 	return ;
	// }
	// while ((chars_read = read(fd, buffer, sizeof(buffer))) > 0)
	// 	responseBody.append(buffer, chars_read);
	// close(fd);
	// if (chars_read == -1)
	// {
	// 	std::cout << "RESPONSE BODY FILE IN BUILD ERROR COULDNT GET OPENED" << std::endl;
	// 	std::string safetyError = "<h1>500 - Internal Server Error</h1>";
	// 	setResponseBody(safetyError);
	// 	setStatus(500);
	// 	// if (sendNow)
	// 	// 	res.sendResponse(clientFd);
	// 	return ;
	// }
	setResponseBody(responseBody);
	std::cout << "what i built in built error response\n";
	std::cout << httpVersion << " " << statusCode << std::endl;
	//can this send already???
	// if (sendNow)
	// 	res.sendResponse(clientFd);
}

void Response::sendResponse(int clientFd)
{
	std::cout << "SENDING RESPONSE HIHIHI" << std::endl;
	//ADD CONNECTION CLOSE OR KEEP-ALIVE
	isSent = false;
	ssize_t sending;
	std::string responseHeaders;
	std::string contentLength;
	contentLength = std::to_string(body.size());

	//.at for reasonPhrases can fail, but should not happen since its hardcoded so that statuscode always exists in reasonPhrases
	responseHeaders = httpVersion + " " + std::to_string(statusCode) + " " + reasonPhrases.at(statusCode) + "\r\n";
	if (statusCode != 204)
	{
		responseHeaders += "Content-Type: " + getHeader("content-type") + "\r\n" +
						"Content-Length: " + contentLength + "\r\n";
	}
	if (statusCode > 299 && statusCode < 400)
		responseHeaders += "Location: " + getHeader("location") + "\r\n";	
	
	
	//responseHeaders += std::string("Connection: close") + "\r\n";
	responseHeaders += "\r\n";
	
	sending = send(clientFd, responseHeaders.c_str(), responseHeaders.size(), MSG_NOSIGNAL);
	if (sending == -1)
	{
		std::cout << "SEND FAILED" << std::endl;
		throw std::runtime_error("");
	}
	if (statusCode != 204 && (statusCode < 300 || statusCode > 399))
	{
		sending = send(clientFd, body.c_str(), body.size(), MSG_NOSIGNAL);
		if (sending == -1)
		{
			std::cout << "SEND FAILED" << std::endl;
			throw std::runtime_error("");
		}
	}
	if (sending != -1)
		isSent = true;
}