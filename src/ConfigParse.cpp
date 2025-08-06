#include "../inc/ConfigParse.hpp"
#include "../inc/Response.hpp"
#include "../inc/HttpRequest.hpp"

void set_default_errors(std::map<int, std::string>& defaultMap)
{
	int defaultCodes[] = {400, 403, 404, 405, 406, 408, 409, 411, 413, 414, 415, 418, 431, 500, 501, 503, 505};
	for (size_t i = 0; i < std::size(defaultCodes); ++i)
	{
		int code = defaultCodes[i];
		std::string path = "./default/" + std::to_string(code) + ".html";

		std::ifstream file(path);
		if (!file)
		{
			throw std::runtime_error("Default error pages missing");
		}
		std::ostringstream buffer;
		buffer << file.rdbuf();
		defaultMap[code] = buffer.str();
		file.close();
	}
	getSetDefaultErrorPages(defaultMap);
}

std::string cleanLine(const std::string &orgLine)
{
	std::string line = orgLine;
	std::size_t commentPos = line.find('#');
	if (commentPos != std::string::npos)
		line = line.substr(0, commentPos);
	line.erase(0,line.find_first_not_of(" \t"));
	line.erase(line.find_last_not_of(" \t") + 1);
	return line;
}

bool isSingleBraceLine(const std::string& line)
{
	std::string cleaned = cleanLine(line);
	if (cleaned == "{")
		return true;
	if (cleaned == "}")
		return true;
	return false;
}

std::string trim(const std::string &toTrim)
{
	size_t pre = toTrim.find_first_not_of(" \t\n\r");
	if (pre == std::string::npos)
		throw std::runtime_error("missing info");
	size_t post = toTrim.find_last_not_of(" \t\n\r;");
	return toTrim.substr(pre, (post - pre + 1));
}

std::string extractConfig(const std::string &line, const std::string &keyword)
{
	size_t pos = line.find(keyword);
	if (pos != std::string::npos)
	{
		std::string value = line.substr(pos + keyword.length());

		size_t semicolonPos = value.find(';');
		if (semicolonPos != std::string::npos)
			value = value.substr(0, semicolonPos);

		value = trim(value);
		return value;
	}
	return "";
}

LocationConfig parseLocationBlock(std::ifstream &file, const std::string &line, std::vector<LocationConfig> &locations)
{
	(void)locations;
	LocationConfig locBlock;
	locBlock.dir_listing = false;
	locBlock.redirect_code = -1;
	std::string path;
	size_t pos = line.find("location");
	std::set<std::string> seenDirectives;

	path = line.substr(pos + 8);
	locBlock.path = trim(path);
	if (locBlock.path[0] != '/' || (locBlock.path.size() > 1 && locBlock.path[1] == '/'))
		throw std::runtime_error("path in location block should start with one /");
	std::string inLine;
	while (std::getline(file, inLine))
	{
		inLine = cleanLine(inLine);
		if (inLine.empty())
			continue;
		std::string value = extractConfig(inLine, "root");
		if (!value.empty() && !seenDirectives.count("root")) 
		{
			locBlock.root = value;
			seenDirectives.insert("root");
		}
		value = extractConfig(inLine, "index");
		if (!value.empty() && !seenDirectives.count("index"))
		{
			locBlock.index = value;
			seenDirectives.insert("index");
		}
		value = extractConfig(inLine, "methods");
		if (!value.empty() && !seenDirectives.count("methods"))
		{
			size_t start = 0;
			while (start < value.length())
			{
				size_t end = value.find(' ', start);
				if (end == std::string::npos)
					end = value.length();
				std::string oneMethod = value.substr(start, end - start);
				if (!oneMethod.empty())
					locBlock.methods.push_back(oneMethod);
				start = end + 1;
			}
			seenDirectives.insert("methods");
		}
		value = extractConfig(inLine, "cgi_path_php");
		if (!value.empty() && !seenDirectives.count("cgi_path_php"))
		{ 
			locBlock.cgi_path_php = value;
			seenDirectives.insert("cgi_path_php");
		}
		value = extractConfig(inLine, "cgi_path_python");
		if (!value.empty() && !seenDirectives.count("cgi_path_python")) 
		{
			locBlock.cgi_path_python = value;
			seenDirectives.insert("cgi_path_python");
		}
		value = extractConfig(inLine, "upload");
		if (!value.empty() && !seenDirectives.count("upload_dir"))
		{
			locBlock.upload_dir = value;
			seenDirectives.insert("upload_dir");
		}
		value = extractConfig(inLine, "dir_listing");
		if (!value.empty() && !seenDirectives.count("dir_listing"))
		{ 
			locBlock.dir_listing = (value == "on") ? true : false;
			seenDirectives.insert("dir_listing");
		}
		value = extractConfig(inLine, "return");
		if (!value.empty() && !seenDirectives.count("return"))
		{
			size_t spPos = value.find(' ');
			if (spPos != std::string::npos)
			{
				std::string redirCode = value.substr(0, spPos);
				locBlock.redirect_code = std::stoi(redirCode);
				locBlock.redirect_target = trim(value.substr(spPos + 1));
			}
			else
    		{
        		locBlock.redirect_code = -1;
        		locBlock.redirect_target.clear();
    		}
			seenDirectives.insert("return");
		}
		if (inLine.find('}') != std::string::npos)
			break;
		if (inLine.find("location") != std::string::npos)
			throw std::runtime_error("Location block misconfigured");
	}
	if (locBlock.methods.empty()) 
		throw std::runtime_error("Method info missing from a location block");
	if (locBlock.path.empty())
		throw std::runtime_error("Path info missing from a location block");
	return locBlock;
}

ServerConfig ConfigParse::parseServerBlock(std::ifstream &file)
{
	std::string line;
	int braceCount = 1;
	ServerConfig s1;
	std::set<std::string> seenDirectives;
	set_default_errors(s1.default_error_pages);
	while (std::getline(file, line))
	{
		line = cleanLine(line);
		if (line.empty())
			continue;
        braceCount += std::count(line.begin(), line.end(), '{');
     	braceCount -= std::count(line.begin(), line.end(), '}');
		if (line.find("location") != std::string::npos)
		{
			LocationConfig l1 = parseLocationBlock(file, line, s1.locations);
			s1.locations.push_back(l1);
			continue;
		}
		std::string value = extractConfig(line, "listen");
		if (!value.empty() && !seenDirectives.count("listen"))
		{
			std::regex listenRegex(R"(^(\d{1,3}\.){3}\d{1,3}:\d{1,5}$)");
			if (std::regex_match(value, listenRegex))
			{
				size_t colonPos = value.find(':');
				if (colonPos != std::string::npos)
				{
					s1.host = value.substr(0, colonPos);
					s1.listen_port = std::stoi(value.substr(colonPos + 1));
					if (s1.listen_port > 65535)
						throw std::runtime_error("Port out of valid range");
				}
			}
			else
				throw std::runtime_error("Please add ip and port in format ddd.d.d.d:dddd");
			seenDirectives.insert("listen");
		}
		value = extractConfig(line, "server_name");
		if (!value.empty() && !seenDirectives.count("server_name"))
		{
			size_t start = 0;
			while (start < value.length())
			{
				size_t end = value.find(' ', start);
				if (end == std::string::npos)
					end = value.length();
				std::string oneName = value.substr(start, end - start);
				if (!oneName.empty())
					s1.server_names.push_back(oneName);
				start = end + 1;
			}
			seenDirectives.insert("server_name");
		}
		value = extractConfig(line, "max_client_body_size");
		if (!value.empty() && !seenDirectives.count("max_client_body_size"))
		{
			int body_size = std::stoi(value);
			if (body_size < 1 || body_size > static_cast<int>(s1.max_client_body_size))
				throw std::runtime_error("Invalid body size in config file");
			s1.max_client_body_size = body_size;
			seenDirectives.insert("max_client_body_size");
		}
		value = extractConfig(line, "max_client_header_size");
		if (!value.empty() && !seenDirectives.count("max_client_header_size"))
		{
			int header_size = std::stoi(value);
			if (header_size < 1 || header_size > static_cast<int>(s1.max_client_header_size))
				throw std::runtime_error("Invalid header size in config file");
			s1.max_client_header_size = header_size;
			seenDirectives.insert("max_client_header_size");
		}
		value = extractConfig(line, "root");
		if (!value.empty() && !seenDirectives.count("root"))
		{
			HttpRequest::makeRootAbsolute(value);
			s1.root = value;
			seenDirectives.insert("root");

		}
		value = extractConfig(line, "error_page");
		{
			if (!value.empty())
			{
				std::istringstream iss(value);
				std::string codeString, path;
				iss >> codeString >> path;
				if (!codeString.empty() && !path.empty())
				{
					if (!std::regex_match(codeString, std::regex(R"(^\d{3}$)")))
						throw std::runtime_error ("Error code out of range " + codeString);
					int code = std::stoi(codeString);
					path = s1.root + "/" + path;
					std::filesystem::path canonicalErrorPath;
					canonicalErrorPath = std::filesystem::canonical(path);
					if (canonicalErrorPath.string().find(s1.root) != 0)
						throw std::runtime_error("Error page escapes server root");
					s1.error_pages_2[code] = path;
					std::ifstream file(path);
					if (!file)
					{
						throw std::runtime_error("Failed to open error page");
					}
					else
					{
						std::ostringstream buffer;
						buffer << file.rdbuf();
						s1.error_pages_2[code] = buffer.str();
					}
				}
			}
		}
       	if (braceCount == 0)
      	{
           	break;
        }
	}
	return s1;
}

void confSyntaxCheck(std::ifstream &file)
{
	static const std::set<std::string> validDirectives = {"server", "listen", "server_name", "max_client_body_size",
	"max_client_header_size", "index", "root", "error_page", "location", "methods", "dir_listing", 
	"cgi_path_python", "cgi_path_php", "upload", "return"};

	std::string line;
	int lineNumber = 0;
	while (std::getline(file, line)) 
	{
		lineNumber++;

		std::string cleaned = cleanLine(line);

		if (cleaned.empty())
			continue;

		if (cleaned == "{" || cleaned == "}")
			continue;

		std::istringstream iss(cleaned);
		std::string firstWord;
		iss >> firstWord;

		if (validDirectives.find(firstWord) != validDirectives.end())
			continue;
		throw std::runtime_error("Syntax error in config file at line " + std::to_string(lineNumber));
	}

	file.clear();
	file.seekg(0, std::ios::beg);
}	

int ConfigParse::confParse(std::string &filename)
{
	if (std::filesystem::path(filename).extension() != ".conf")
		throw(std::runtime_error("Invalid file extension"));
	std::ifstream file;
	file.open(filename);
	if (file.fail())
	{
		throw(std::runtime_error("Error opening config file"));
	}
	std::string line;
	int openBraces = 0;
	int closeBraces = 0;
	while (getline(file, line))
	{
		if (line.find('{') != std::string::npos || line.find('}') != std::string::npos)
		{
		if (!isSingleBraceLine(line))
			throw std::runtime_error("Braces must appear alone on their own line: " + line);
		}
		openBraces += std::count(line.begin(), line.end(), '{');
		closeBraces += std::count(line.begin(), line.end(), '}');
	}
	if (openBraces != closeBraces || openBraces == 0)
		throw std::runtime_error("Braces mismatch");
	file.clear();
	file.seekg(0, std::ios::beg);
	confSyntaxCheck(file);
	bool insideBlock = false;
	while (std::getline(file, line))
	{
		line = cleanLine(line);
		if (line.empty())
			continue;
		if (!insideBlock)
		{
			if (line.find("server") != std::string::npos) 
			{
				insideBlock = true;
				continue;
			}
		}
		else
		{
			if (line.find('{') != std::string::npos)
			{
				ServerConfig s = parseServerBlock(file);
				servers.push_back(s);
				insideBlock = false;
			}
			else
				throw(std::runtime_error("Error in server block structure\n"));
		}
	}
	file.close();
	return 0;
}

const std::vector<ServerConfig> &ConfigParse::getServers() const{
	return servers;
}