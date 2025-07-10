#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

class Response
{
	private:
		int statusCode;
		std::string statusMessage;
		std::string httpVersion = "HTTP/1.1";
		std::map<std::string, std::string> headers;
		std::string body;
		std::map<int, std::string> errorPages;
		static const std::unordered_map<int, std::string> reasonPhrases;

	public:
		void setStatus(int code, const std::string& message = "");
		void setResponseHeader(const std::string &key, const std::string &value);
		void setResponseBody(std::string &bodyContent);

		int getStatusCode() const;
		std::string getStatusMessage() const;
		const std::string getHeader(const std::string &key) const;
		std::string getBody() const;
		std::string toString() const;

		static Response buildErrorResponse(int statusCode, bool sendNow, int clientFd, std::map<int, std::string> errorPages = {});
		void sendResponse(int clientFd); //public?

		Response() = default;
		Response(int code);
		Response(int code, const std::string& body);
		~Response() {};
};