#pragma once
#include <string>
#include "HttpRequest.hpp"
#include "ConfigParse.hpp"

class ClientConnection
{
	private:
		int fd;
		std::string buffer;
		enum ParseState
		{
			REQUEST_LINE,
			HEADERS,
			BODY,
			COMPLETE
		} state;
		HttpRequest request;
		size_t expected_body_len;
		bool isKeepAlive;
	public:
		ClientConnection() : fd(-1), state(REQUEST_LINE), expected_body_len(0), request(-1), isKeepAlive(true) {}
		ClientConnection(int fd) : fd(fd), state(REQUEST_LINE), expected_body_len(0), request(fd), isKeepAlive(true) {}
		int getFd() const { return fd; }
		bool parseData(const char *data, size_t len, std::vector<ServerConfig> servers);
		bool getIsAlive() const { return isKeepAlive; }
		void resetState();
};
