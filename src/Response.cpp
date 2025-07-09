/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hskrzypi <hskrzypi@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/06 13:19:50 by hskrzypi          #+#    #+#             */
/*   Updated: 2025/07/06 13:53:35 by hskrzypi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Response.hpp"

const std::unordered_map<int, std::string> Response::reasonPhrases = {
	{200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {411, "Length Required"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {418, "I'm a teapot"},
    {431, "Request Header Fields Too Large"},
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

Response Response::buildResponse(int statusCode, bool sendNow, int clientFd)
{
	std::cout << "build error response call\n";
	Response res;
	res.setStatus(statusCode);
	res.setResponseHeader("content-type", "text/plain");

	std::cout << "what i built in built error response\n";
	std::cout << res.httpVersion << res.statusCode << std::endl;
	//can this send already???
	if (sendNow)
	{
		ssize_t sending;

		res.sendResponse(clientFd);
	}
	return res;
}

void Response::sendResponse(int clientFd)
{
	ssize_t sending;
	std::string responseHeaders;
	std::string contentLength;
	std::string responseBody;
	contentLength = std::to_string(body.size());

	std::cout << "i am in sendResponse?" << std::endl;
	responseHeaders =
	httpVersion + " " + std::to_string(statusCode) + " " + reasonPhrases.at(statusCode) + "\r\n" +
	"Content-Type: " + getHeader("content-type") + "\r\n" +
	"Content-Length: " + contentLength + "\r\n" +
	"\r\n";

	std::cout << "THESE ARE MY RESPONSE HEADERS: " << responseHeaders << std::endl;
	std::cout << "hello? from sendResponse" << std::endl;
	//std::cout << "SENDING HEADERS: " << responseHeaders << std::endl;
	sending = send(clientFd, responseHeaders.c_str(), responseHeaders.size(), MSG_NOSIGNAL);
	if (sending == -1)
	{
		std::cout << "SEND FAILED" << std::endl;
		throw std::runtime_error("");
	}
	//error check
	//IF RESPONSE IS ERROR, PUT RESPONSEBODY TO ERROR HTML CONTENT IN buildResponse
	responseBody = body;
	//std::cout << "SENDING BODY..." << std::endl;
	if (responseBody.empty())
		std::cout << "MY RESPONSE BODY IS EMPTY" << std::endl;
	else 
		std::cout << "THIS IS MY RESPONSE BODY: " << responseBody << std::endl;
	sending = send(clientFd, responseBody.c_str(), responseBody.size(), MSG_NOSIGNAL);
	if (sending == -1)
	{
		std::cout << "SEND FAILED" << std::endl;
		throw std::runtime_error("");
	}
	//error check DEFINETLY NEEDED THIS WILL CRASH THE SERVER T.Leo 6.7.2025 :)
	//if (sending == -1)
	// teapot?
}