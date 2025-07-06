#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>

class Response
{
	private:
		int statusCode;
		std::string statusMessage;
		std::string httpVersion = "HTTP/1.1";
		std::map<std::string, std::string> headers;
		std::string body;
		static const std::unordered_map<int, std::string> reasonPhrases;

	public:
		void setStatus(int code, const std::string& message = "");
		void setHeader(const std::string &key, const std::string &value);
		void setBody(std::string &bodyContent);

		int getStatusCode() const;
		std::string getStatusMessage() const;
		std::string getHeader(std::string &key) const;
		std::string getBody() const;
		std::string toString() const;

		static Response buildErrorResponse(int statusCode, const std::string& message = "");

		Response() = default;
		Response(int code);
		Response(int code, const std::string& body);
		~Response() {};
};