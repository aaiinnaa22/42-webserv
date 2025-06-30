#include "../inc/ClientConnection.hpp"
#include "../inc/ConfigParse.hpp"
//#include "../inc/HttpRequest.hpp"

void	normalize_case(std::string &key)
{
	transform(key.begin(), key.end(), key.begin(), ::tolower);
}

bool is_valid_http_version_syntax(const std::string &version)
{
	if (version.size() < 8 | version.size() > 10)
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
	state == REQUEST_LINE;
	buffer.clear();
	expected_body_len = 0;
	request = HttpRequest(fd);
}

//TO DO: normalization of characters for key-value pairs (nginx is not case sensitive)
bool ClientConnection::parseData(const char *data, size_t len, ServerConfig config)
{
	buffer.append(data, len);

	while (true)
	{
		if (state == REQUEST_LINE)
		{
			size_t line_end = buffer.find("\r\n");
			if (line_end == std::string::npos)
				return false;

			std::string request_line = buffer.substr(0, line_end);
			buffer.erase(0, line_end);

			if (!is_ascii(request_line))
                throw std::runtime_error("400 Bad Request: non-ASCII in request line");
			std::istringstream stream(request_line);
			std::string method, path, version;
			if (!(stream >> method >> path))
				throw std::runtime_error("400 Bad Request");
			if (!(stream >> version))
    			version = "HTTP/1.1";
			if (method.empty() || path.empty())
				throw std::runtime_error("400 Bad Request");
			if (path[0] != '/' && path.find("http://") != 0 && path.find("https://") != 0)
				throw std::runtime_error("400 Bad Request");
			if (method != "GET" && method != "POST" && method != "DELETE")
				throw std::runtime_error("405 Method Not Allowed");
			else if (!is_valid_http_version_syntax(version))
        		throw std::runtime_error("400 Bad Request");
			if (version != "HTTP/1.1")
				throw std::runtime_error("505 HTTP Version Not Supported");
			request.setMethod(method);
			request.setPath(path);
			request.setHttpVersion(version);
			state = HEADERS;
		}
		else if (state == HEADERS)
		{
    		size_t headers_end = buffer.find("\r\n\r\n");
			if (headers_end == std::string::npos)
                return false;
			if (headers_end == 0)
    			throw std::runtime_error("400 Bad Request: no headers");
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
					throw std::runtime_error("400 Bad Request");

				std::string key = line.substr(0, colon);
				normalize_case(key);
				std::string value = line.substr(colon + 1);
				value.erase(0, value.find_first_not_of(" "));

				request.addHeader(key, value);
			}
			std::string checkHost = request.getHeader("host");
			if (checkHost.empty())
				throw std::runtime_error("400 Bad Request");
			std::string contentLengthVal = request.getHeader("content-length");
			if (!contentLengthVal.empty())
			{
				expected_body_len = std::stoi(contentLengthVal);//stoi check!
				if (expected_body_len < 0)
					throw std::runtime_error("400 Bad Request");
				state = BODY;
			}
			else
				state = COMPLETE;
		}
		else if (state == BODY)
		{
			if (buffer.size() < expected_body_len)
				return false;
			request.setBody(buffer.substr(0, expected_body_len));
			buffer.erase(0, expected_body_len);
			state = COMPLETE;
		}
		else if (state == COMPLETE)
		{
			std::string connectionType = request.getHeader("connection");
			if (connectionType == "close")
			{
				isKeepAlive = false;
				std::cout << "connection header result " << isKeepAlive << std::endl;
			}
			request.doRequest(config);
			return true;
		}
	}
}

