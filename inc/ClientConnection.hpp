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
		std::vector<ServerConfig> bound_servers; 
		const ServerConfig* selected_server;
	public:
		ClientConnection() : fd(-1), state(REQUEST_LINE), 
			expected_body_len(0), request(-1), 
			isKeepAlive(true), bound_servers(), selected_server(nullptr) {}
		ClientConnection(int fd, const std::vector<ServerConfig>& servers) : fd(fd), state(REQUEST_LINE), 
			expected_body_len(0), request(fd), 
			isKeepAlive(true),	bound_servers(servers), selected_server(nullptr)  {}
		
		int getFd() const { return fd; }
		bool parseData(const char* data, size_t len);
		bool getIsAlive() const { return isKeepAlive; }
		void resetState();
		
};
