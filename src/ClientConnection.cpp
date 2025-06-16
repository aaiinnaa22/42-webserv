#include "../inc/ClientConnection.hpp"
#include "../inc/ConfigParse.hpp"

bool ClientConnection::parseData(const char *data, size_t len, ServerConfig config)
{
	buffer.append(data, len);

	while (true)
	{
		if (state == REQUEST_LINE || state == HEADERS)
		{
			size_t header_end = buffer.find("\r\n\r\n");
			if (header_end == std::string::npos)
				return false;

			std::string headers_part = buffer.substr(0, header_end);
			buffer.erase(0, header_end + 4);

			std::istringstream stream(headers_part);
			std::string line;
	
			if (state == REQUEST_LINE)
			{
				if (!std::getline(stream, line) || line.empty())
					throw std::runtime_error("400 Bad Request");
				
				if (line.back() == '\r') line.pop_back();
				std::istringstream lineStr(line);
				std::string method, path, version;
				lineStr >> method >> path >> version;
				if (method.empty() || path.empty() || version.empty())
					throw std::runtime_error("400 Bad Request");
				request.setMethod(method);
				request.setPath(path);
				request.setHttpVersion(version);
				if (method != "GET" && method != "POST" && method != "DELETE")
					throw std::runtime_error("405 Method Not Allowed");// to be replaced with send response? //return true?
				if (version != "HTTP/1.1")
					throw std::runtime_error("505 HTTP Version Not Supported");
				state = HEADERS;
			}
			while (std::getline(stream, line))
			{
				if (line.back() == '\r') line.pop_back();
				if (line.empty())
					continue; 

				size_t colon = line.find(':');
				if (colon == std::string::npos)
					throw std::runtime_error("400 Bad Request");

				std::string key = line.substr(0, colon);
				std::string value = line.substr(colon + 1);
				value.erase(0, value.find_first_not_of(" "));

				request.addHeader(key, value);
			}

			std::string contentLengthVal = request.getHeader("Content-Length");
			if (!contentLengthVal.empty())
			{
				expected_body_len = std::stoi(contentLengthVal);
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
			request.doRequest(config);
			return true;
		}
	}
}

