#pragma once
#include <string>
#include "HttpRequest.hpp"
#include "ConfigParse.hpp"
#include "Response.hpp"
#include <ctime>

class ClientConnection
{
	private:
		int fd;
		std::string buffer;
		std::string chunkedBodyBuffer;
		enum ParseState
		{
			REQUEST_LINE,
			HEADERS,
			BODY,
			CHUNKED_BODY,
			COMPLETE
		} state;
		enum chunkParseState
		{
			READ_CHUNK_SIZE,
        	READ_CHUNK_DATA,
        	READ_CHUNK_CRLF
		};
		HttpRequest request;
		size_t expected_body_len;
		bool isKeepAlive;
		bool reading_chunk_size;
		int chunk_size;
		std::vector<ServerConfig> bound_servers; 
		const ServerConfig* selected_server;
		Response response;
		int _lastactivity;
	public:
		enum parseResult
		{
			INCOMPLETE,
			DONE,
			ERROR
		};
		ClientConnection() : fd(-1), state(REQUEST_LINE), 
			request(-1), expected_body_len(0), 
			isKeepAlive(true), bound_servers(), selected_server(nullptr), _lastactivity(0)  {}
		ClientConnection(int fd, const std::vector<ServerConfig>& servers) : fd(fd), state(REQUEST_LINE), 
			request(fd), expected_body_len(0),
			isKeepAlive(true),	bound_servers(servers), selected_server(nullptr), _lastactivity(0)  {}
		
		~ClientConnection() {};
		int getFd() const { return fd; }
		parseResult parseData(const char* data, size_t len, const Server& server);
		int	parseRequestLine(std::string& buffer, size_t len);
		int parseHeaders(std::string buffer);
		bool getIsAlive() const { return isKeepAlive; }
		void resetState();

		Response& getResponse();   

		void setLastActivity();
		int getLastActivity();
		
};
