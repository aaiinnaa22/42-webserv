#include "../inc/ClientConnection.hpp"
#include "../inc/ConfigParse.hpp"
//#include "../inc/HttpRequest.hpp"
#include "../inc/Response.hpp"

void	normalize_case(std::string &key)
{
	transform(key.begin(), key.end(), key.begin(), ::tolower);
}

bool is_valid_http_version_syntax(const std::string &version)
{
	if (version.size() < 8 || version.size() > 10)
		return false;
	if (version.substr(0,5) != "HTTP/")
		return false;
	std::string numbers = version.substr(5);
	size_t dot_pos = numbers.find('.');
	if (dot_pos == std::string::npos)
		return false;

    std::string main = numbers.substr(0, dot_pos);
    std::string minor = numbers.substr(dot_pos + 1);

    if (main.empty() || minor.empty())
		return false;

    if (!std::all_of(main.begin(), main.end(), ::isdigit))
		return false;
    if (!std::all_of(minor.begin(), minor.end(), ::isdigit))
		return false;
    return true;
}

bool is_ascii(const std::string& s)
{
	for (unsigned char c : s)
	{
        if (c > 127)
            return false;
    }
    return true;
}

void ClientConnection::resetState()
{
	state = REQUEST_LINE;
	buffer.clear();
	expected_body_len = 0;
	request = HttpRequest(fd);
	selected_server = nullptr;
	setLastActivity();
	chunkedBodyBuffer.erase();
}

const ServerConfig* selectServerByHost(const std::vector<ServerConfig>& servers, const std::string& hostHeader) 
{
	std::string reqHost = hostHeader;
	size_t colon_pos = reqHost.find(':');
	if (colon_pos != std::string::npos)
		reqHost = reqHost.substr(0, colon_pos);
	normalize_case(reqHost);
	const ServerConfig* selectedServer = &servers[0];  // default

	for (const ServerConfig& server : servers)
	{
		for (const std::string& name : server.server_names)
		{
			std::string serverName = name;
			normalize_case(serverName);
			if (serverName == reqHost)
			{
				selectedServer = &server;
				break;
			}
		}
	}
	return selectedServer;
}

Response& ClientConnection::getResponse()
{
    return response;
}

ClientConnection::parseResult ClientConnection::parseData(const char *data, size_t len)
{
	try
	{	
		buffer.append(data, len);
		// auto respondWithError = [&](int code) -> parseResult
		// {
		// 	response = Response::buildErrorResponse(code, 1, fd);
		// 	//can  this send already????
		// 	return ERROR;
		// };
		//int currentChunkSize = -1;
		while (true)
		{
			if (state == REQUEST_LINE)
			{
				size_t line_end = buffer.find("\r\n");
				if (line_end == std::string::npos)
					return INCOMPLETE;
				std::string request_line = buffer.substr(0, line_end);
				buffer.erase(0, line_end);

				if (!is_ascii(request_line))
					throw ErrorResponseException(400);
				std::istringstream stream(request_line);
				std::string method, path, version;
				if (!(stream >> method >> path))
					throw ErrorResponseException(400);
				if (!(stream >> version))
					version = "HTTP/1.1";
				if (method.empty() || path.empty())
					throw ErrorResponseException(400);
				if (path[0] != '/' && path.find("http://") != 0 && path.find("https://") != 0)
					throw ErrorResponseException(400);
				if (method != "GET" && method != "POST" && method != "DELETE")
					throw ErrorResponseException(405);
				else if (!is_valid_http_version_syntax(version))
					throw ErrorResponseException(400);
				if (version != "HTTP/1.1")
					throw ErrorResponseException(505);
				request.setMethod(method);
				request.setPath(path);
				request.setHttpVersion(version);
				state = HEADERS;
			}
			else if (state == HEADERS)
			{
				size_t headers_end = buffer.find("\r\n\r\n");
				if (headers_end == std::string::npos)
					return INCOMPLETE;
				if (headers_end == 0)
					throw ErrorResponseException(400);
				std::string headers_str = buffer.substr(0, headers_end);
				buffer.erase(0, headers_end + 4);
				std::istringstream stream(headers_str);
				std::string line;
				while (std::getline(stream, line))
				{
					if (line.back() == '\r')
						line.pop_back();
					if (line.empty())
						continue; 
					size_t colon = line.find(':');
					if (colon == std::string::npos)
						throw ErrorResponseException(400);
					std::string key = line.substr(0, colon);
					normalize_case(key);
					std::string value = line.substr(colon + 1);
					value.erase(0, value.find_first_not_of(" "));
					request.addHeader(key, value);
				}
				std::string connType = request.getHeader("connection");
				if (connType == "close")
					isKeepAlive = false;
				std::string checkHost = request.getHeader("host");
				if (checkHost.empty())
					throw ErrorResponseException(400);
				std::string encoding = request.getHeader("transfer-encoding");
				std::string contentLengthVal = request.getHeader("content-length");
				if (!encoding.empty() && encoding != "chunked")
					throw ErrorResponseException(501);
				else if (!encoding.empty() && encoding == "chunked")
				{
					if (!contentLengthVal.empty())
						throw ErrorResponseException(400);
					state = CHUNKED_BODY;
				}
				else if (!contentLengthVal.empty())
				{
					expected_body_len = std::stoi(contentLengthVal);
					if (expected_body_len < 0)
						throw ErrorResponseException(400);
					state = BODY;
				}
				else
					state = COMPLETE;
			}
			else if (state == BODY)
			{
				if (buffer.size() < expected_body_len)
					return INCOMPLETE;
				request.setBody(buffer.substr(0, expected_body_len));
				buffer.erase(0, expected_body_len);
				state = COMPLETE;
			}
			else if (state == CHUNKED_BODY)
			{
				std::cout << "parsing chunked body\n";
				//size_t chunkedEnd = buffer.find("0\r\n");
			}
			else if (state == COMPLETE)
			{
				std::cout << "chunked body buffer: " << chunkedBodyBuffer << std::endl; 
				std::string connectionType = request.getHeader("connection");
				if (connectionType == "close")
					isKeepAlive = false;
				std::string chosenHost = request.getHeader("host");
				std::cout << "Bound servers count: " << bound_servers.size() << ", Host header: " << chosenHost << std::endl;
				selected_server = selectServerByHost(bound_servers, chosenHost);
				request.doRequest(*selected_server);
				return DONE;
			}
		}
	}
	catch (ErrorResponseException &e)
	{
		Response::buildErrorResponse(e.getResponseStatus(), 1, fd);
		return ERROR;
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << " WAS CATCHED IN DOREQUEST!!!" << std::endl;
		Response::buildErrorResponse(500, 1, fd);
		return ERROR;
	}
	return ERROR;
}

void ClientConnection::setLastActivity(void)
{
	// time_t timestamp;
	// struct tm datetime = {0};
  	// _lastactivity = mktime(&datetime);
	std::time_t result = std::time(nullptr);
    std::asctime(std::localtime(&result));
	this->_lastactivity = result;
	std::cout << "set last activity for " << this->fd << " " << this->_lastactivity << std::endl;
}

int ClientConnection::getLastActivity(void)
{
	return (_lastactivity);
}
