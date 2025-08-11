#include "../inc/ClientConnection.hpp"

void	ClientConnection::normalize_case(std::string &key)
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

bool is_valid_header_key(const std::string& key)
{
	for (size_t i = 0; i < key.length(); ++i)
	{
		char c = key[i];
		if (!(std::isalnum(c) ||
			  c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
			  c == '\''|| c == '*' || c == '+' || c == '-' || c == '.' ||
			  c == '^' || c == '_' || c == '`' || c == '|' || c == '~'))
		{
			return false;
		}
	}
	return true;
}


std::string trimKV(const std::string& s) 
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";

    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

void ClientConnection::setIsAlive(bool isAlive)
{
	isKeepAlive = isAlive;
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
	ClientConnection::normalize_case(reqHost);
	const ServerConfig* selectedServer = &servers[0];

	for (const ServerConfig& server : servers)
	{
		for (const std::string& name : server.server_names)
		{
			std::string serverName = name;
			ClientConnection::normalize_case(serverName);
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

int	ClientConnection::parseRequestLine(size_t len)
{
	(void)len;
	size_t line_end = buffer.find("\r\n");
	std::string request_line = buffer.substr(0, line_end);
	buffer = buffer.substr(line_end);

	if (!is_ascii(request_line))
	{
		throw ErrorResponseException(400);
	}
	std::istringstream stream(request_line);
	std::string method, path, version;
	if (!(stream >> method >> path))
	{
		throw ErrorResponseException(400);
	}
	if (!(stream >> version))
		version = "HTTP/1.1";
	if (method.empty() || path.empty())
	{
		throw ErrorResponseException(400);
	}
	if (method != "GET" && method != "POST" && method != "DELETE")
	{
		throw ErrorResponseException(405);
	}
	if (path[0] != '/' && path.find("http://") != 0 && path.find("http:://") != 0)
	{
		throw ErrorResponseException(400);
	}
	if (path.size() > MAX_URI_LENGTH)
	{
		throw ErrorResponseException(414);
	}
 	if (!is_valid_http_version_syntax(version))
	{
		throw ErrorResponseException(400);
	}
	if (version != "HTTP/1.1")
	{
		throw ErrorResponseException(505);
	}
	request.setMethod(method);
	request.setPath(path);
	request.setHttpVersion(version);
	return 0;
}

int ClientConnection::parseHeaders(std::string buffer)
{
	std::istringstream stream(buffer);
    std::string line;
	std::set<std::string> seenHeaders;
	const std::set<std::string> disallowedDuplicates = {"host", "content-length", "content-type", "user-agent"};
	while (std::getline(stream, line))
	{
		if (line.back() == '\r')
			line.pop_back();
		if (line.empty())
			continue; 
		size_t colon = line.find(':');
		if (colon == std::string::npos)
		{
			throw ErrorResponseException(400);
		}
		std::string key = line.substr(0, colon);
		if (!is_valid_header_key(key))
		{
			throw ErrorResponseException(400);
		}
		normalize_case(key);
		std::string value = line.substr(colon + 1);
		value = trimKV(value);
		if (disallowedDuplicates.count(key) && seenHeaders.count(key))
		{
			std::cout << "duplicated header: " << key << std::endl;
			throw ErrorResponseException(400);
		}
		seenHeaders.insert(key);
		request.addHeader(key, value);
	}
	return 0;
}

void ClientConnection::parseMultipartBody(const std::string& body, const std::string& boundary) 
{
    std::string boundary_marker = "--" + boundary;
    std::string closing_boundary = boundary_marker + "--";

	size_t pos = 0;
    int part_count = 0;
	//counting the parts (allow 1, more is 501 not implemented)
    while ((pos = body.find(boundary_marker, pos)) != std::string::npos)
    {
        if (body.compare(pos, closing_boundary.length(), closing_boundary) == 0)
            break;
        ++part_count;
        pos += boundary_marker.length();
    }
    if (part_count > 1)
        throw ErrorResponseException(501);

	//stripping the boundary strings at the start and end
    size_t part_start = body.find(boundary_marker + "\r\n");
    if (part_start == std::string::npos)
        throw ErrorResponseException(400);
    part_start += boundary_marker.length() + 2;
    size_t part_end = body.find(closing_boundary, part_start);
    if (part_end == std::string::npos)
        throw ErrorResponseException(400);
    std::string part = body.substr(part_start, part_end - part_start);
	
	//separating multipart headers from the actual body
    size_t header_end = part.find("\r\n\r\n");
    if (header_end == std::string::npos)
        throw ErrorResponseException(400);

    std::string header_section = part.substr(0, header_end);
    std::string content = part.substr(header_end + 4);
	if (content.size() >= 2 && content.substr(content.size() - 2) == "\r\n")
    	content.erase(content.size() - 2);
	request.setBody(content);
	
	//parsing headers (special treatment for content disposition and related params)
    std::istringstream headers_stream(header_section);
    std::string line;
	while (std::getline(headers_stream, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
		
		size_t colonPos = line.find(':');
	    if (colonPos == std::string::npos)
    	    continue;
		std::string key = line.substr(0, colonPos);
		key = trimKV(key);
    	std::string val = line.substr(colonPos + 1);
		val = trimKV(val);
		normalize_case(key);
        if (key == "content-disposition")
        {
            size_t semiPos = val.find(";");
            std::string dispositionType;
			std::string paramsPart;
			if (semiPos == std::string::npos)
			{
            	dispositionType = val;
            	paramsPart.clear();
        	}
			else 
			{
            	dispositionType = val.substr(0, semiPos);
            	paramsPart = val.substr(semiPos + 1);
        	}

            request.bodyHeaders["content-disposition"] = dispositionType;

            std::istringstream paramStream(paramsPart);
            std::string param;
            while (std::getline(paramStream, param, ';'))
            {
                size_t eq = param.find('=');
                if (eq != std::string::npos)
                {
                    std::string paramKey = param.substr(0, eq);
                    std::string paramVal = param.substr(eq + 1);
                    paramKey = trimKV(paramKey);
                    paramVal = trimKV(paramVal);
                    if (!paramVal.empty() && paramVal.front() == '"' && paramVal.back() == '"')
                        paramVal = paramVal.substr(1, paramVal.size() - 2);
                    request.formFields[paramKey] = paramVal;
                }
            }
        }
        else
        {
                request.bodyHeaders[key] = val;
        }
    }
}

ClientConnection::parseResult ClientConnection::parseData(const char *data, size_t len, const Server& server)
{
	try
	{
		buffer.append(data, len);
		while (true)
		{
			if (state == REQUEST_LINE)
			{
				if (buffer.find("\r\n") == std::string::npos)
					return INCOMPLETE;
				parseRequestLine(len);
				state = HEADERS;
			}
			else if (state == HEADERS)
			{
				std::string header_buffer;
				size_t headers_end;
				headers_end = buffer.find("\r\n\r\n");
				if (headers_end == std::string::npos)
				{
					return INCOMPLETE;
				}
				header_buffer = buffer.substr(0, headers_end);

				parseHeaders(header_buffer);
				buffer.erase(0, headers_end + 4);
				std::string connType = request.getHeader("connection");
				if (connType == "close")
				{
					request.setKeepAlive(false);
					isKeepAlive = false;
				}
				std::string checkHost = request.getHeader("host");
				if (checkHost.empty())
				{
					throw ErrorResponseException(400);
				}
				//finding matching server block, moved here from do request
				selected_server = selectServerByHost(bound_servers, checkHost);
				request.setErrorPages(selected_server->error_pages_2);
				if (header_buffer.size() > selected_server->max_client_header_size)
				{
					throw ErrorResponseException(431);
				}
				std::string encoding = request.getHeader("transfer-encoding");
				std::string contentLengthVal = request.getHeader("content-length");
				if (request.getMethod() != "POST")
				{
					state = COMPLETE;
					continue;
				}
				if (request.getMethod() == "POST" && contentLengthVal.empty() && encoding.empty())
				{
					throw(ErrorResponseException(411));
				}
				if (!encoding.empty() && encoding != "chunked")
					throw ErrorResponseException(501);
				else if (!encoding.empty() && encoding == "chunked")
				{
					if (!contentLengthVal.empty())
					{
						throw ErrorResponseException(400);
					}
					else
					{
						state = CHUNKED_BODY;
						reading_chunk_size = 1;
					}
				}
				else if (!contentLengthVal.empty())
				{
					expected_body_len = std::stoi(contentLengthVal);
					if (expected_body_len < 0) 
					{
						throw ErrorResponseException(400);
					}
					if (static_cast<size_t>(expected_body_len) > selected_server->max_client_body_size)
						throw ErrorResponseException(413);
					state = BODY;
					std::string contentType = request.getHeader("content-type");
					if (contentType.find("multipart/form-data") != std::string::npos) 
					{
						size_t boundary_pos = contentType.find("boundary=");
						if (boundary_pos == std::string::npos)
							throw ErrorResponseException(400);
						boundary = contentType.substr(boundary_pos + 9);
    					isMultipart = true;
					}
				}
				else
					state = COMPLETE;
			}
			if (state == BODY)
			{
				if (buffer.size() < static_cast<size_t>(expected_body_len))
					return INCOMPLETE;
				if (isMultipart)
				{
    				parseMultipartBody(buffer.substr(0, expected_body_len), boundary);
					isMultipart = false;
				}
				else
					request.setBody(buffer.substr(0, expected_body_len));
			
				buffer.erase(0, expected_body_len);

				state = COMPLETE;
			}
			if (state == CHUNKED_BODY)
			{
				while (true)
				{
					if (reading_chunk_size)
					{
						size_t line_end = buffer.find("\r\n");
						if (line_end == std::string::npos)
							return INCOMPLETE;

						std::string size_str = buffer.substr(0, line_end);
						buffer.erase(0, line_end + 2);
						chunk_size = std::stoi(size_str, nullptr, 16);

						if (chunk_size == 0)
						{
							state = COMPLETE;
							break;
						}

						reading_chunk_size = false;
					}
					else
					{
						if (buffer.size() < static_cast<size_t>(chunk_size + 2))
							return INCOMPLETE;

						std::string chunk_data = buffer.substr(0, chunk_size);
						request.appendBody(chunk_data);
						buffer.erase(0, chunk_size);

						if (buffer.substr(0, 2) != "\r\n")
						{
							throw ErrorResponseException(400);
						}
						buffer.erase(0, 2);
						reading_chunk_size = true;
					}
				}
				state = COMPLETE;
			}
			if (state == COMPLETE)
			{
				response = request.doRequest(*selected_server, server);
				buffer.erase();
				return DONE;
			}
		}
	}
	catch (ErrorResponseException &e)
	{
		int shouldClose = e.getResponseStatus();
		//std::cout << "response status from catch: " << shouldClose << std::endl;
		if (shouldClose == 400 || shouldClose == 408 || shouldClose == 413
			|| shouldClose == 414 || shouldClose == 431 || shouldClose == 505)
		{
			request.setKeepAlive(false);
			isKeepAlive = false;
		}
		response.buildErrorResponse(e.getResponseStatus(), fd, request.getErrorPages());
		return ERROR;
	}
	catch (ChildError& e)
	{
		throw ChildError(500);
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << " caught in DoRequest" << std::endl;
		response.buildErrorResponse(500, fd, request.getErrorPages());
		return ERROR;
	}
	return ERROR;
}

void ClientConnection::setLastActivity(void)
{
	std::time_t result = std::time(nullptr);
    std::asctime(std::localtime(&result));
	this->_lastactivity = result;
	//std::cout << "set last activity for " << this->fd << " " << this->_lastactivity << std::endl;
}

int ClientConnection::getLastActivity(void)
{
	return (_lastactivity);
}
