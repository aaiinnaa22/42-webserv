#include "../inc/ConfigParse.hpp"

//simple directives and block directives
//TO DO: nested blocks, directive parsing, errors????

//DONE: cgi_path_php, cgi_path_python, dir_listing, and... return 307 /newDir/; (no idea what this is)
//TO DO SUNDAY: comments are not stripped from the latter part of basic config

//TO DO SUNDAY: struct has to be accessible from all over the program - should it be an object? a static struct? 
// WE SHALL SEE

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
		value = trim(value);
		return value;
	}
	return "";
	//throw std::runtime_error("issues in config parsing");
}

LocationConfig parseLocationBlock(std::ifstream &file, const std::string &line, std::vector<LocationConfig> &locations)
{
	LocationConfig locBlock;
	locBlock.dir_listing = false;
	locBlock.redirect_code = -1;

	size_t pos = line.find("location");
	size_t brace = line.find('{', pos);//check for braces and position??
	std::string path = line.substr(pos + 8, brace - (pos + 8));
	locBlock.path = trim(path);
	int braceCount = 1;
	std::string inLine;
	while (std::getline(file, inLine))
	{
		inLine = cleanLine(inLine);
		if (inLine.empty())
			continue;
		braceCount += std::count(inLine.begin(), inLine.end(), '{');
     	braceCount -= std::count(inLine.begin(), inLine.end(), '}');
		if (braceCount == 0)
			break;
		std::string value = extractConfig(inLine, "root");
		if (!value.empty()) 
			locBlock.root = value;
		value = extractConfig(inLine, "index");
		if (!value.empty()) 
			locBlock.index = value;
		value = extractConfig(inLine, "methods");
		if (!value.empty())
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
		}
		value = extractConfig(inLine, "cgi_path_php");
		if (!value.empty()) 
			locBlock.cgi_path_php = value;
		value = extractConfig(inLine, "cgi_path_python");
		if (!value.empty()) 
			locBlock.cgi_path_python = value;
		value = extractConfig(inLine, "upload_dir");
		if (!value.empty())
			locBlock.upload_dir = value;
		value = extractConfig(inLine, "dir_listing");
		if (!value.empty()) 
			locBlock.dir_listing = (value == "on") ? true : false;
		value = extractConfig(inLine, "return");
		if (!value.empty())
		{
			size_t spPos = value.find(' ');
			if (spPos != std::string::npos)
			{
				std::string redirCode = value.substr(0, spPos);
				locBlock.redirect_code = std::stoi(redirCode);//to check
				locBlock.redirect_target = trim(value.substr(spPos + 1));
			}
			else
    		{
        		locBlock.redirect_code = -1;
        		locBlock.redirect_target.clear();
    		}
		}
		if (braceCount == 1)
			break;
	}
	// std::cout << "Parsed location block:\n";
	// std::cout << "  path: " << locBlock.path << "\n";
	// std::cout << "  root: " << locBlock.root << "\n";
	// std::cout << "  index: " << locBlock.index << "\n";
	// std::cout << "  methods:";
	// for (size_t i = 0; i < locBlock.methods.size(); ++i)
	// 	std::cout << " " << locBlock.methods[i];
	// std::cout << "\n";
	// std::cout << "  cgi path php: " << locBlock.cgi_path_php << std::endl;
	// std::cout << "  cgi path python: " << locBlock.cgi_path_python << std::endl;
	// std::cout << "  upload dir: " << locBlock.upload_dir << std::endl;
	// std::cout << "  dir listing: " << locBlock.dir_listing << std::endl;
	// std::cout << "  redir code: " << locBlock.redirect_code << std::endl;
	// std::cout << "  redir target: " << locBlock.redirect_target << std::endl;
	return locBlock;
}

ServerConfig ConfigParse::parseServerBlock(std::ifstream &file)
{
	//std::cout << "Parse server block call\n";
	std::string line;
	int braceCount = 1;
	ServerConfig s1;
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
		if (!value.empty())
		{
			size_t colonPos = value.find(':');
			if (colonPos != std::string::npos)
			{
				s1.host = value.substr(0, colonPos);
				s1.listen_port = std::stoi(value.substr(colonPos + 1));
			}
			else
			{
				s1.listen_port = std::stoi(value);
				if (s1.host.empty())
					s1.host = "0.0.0.0";
			}
			//std::cout << s1.host << "-->host from struct\n";
			//std::cout << s1.listen_port << "-->listen port from struct\n";
		}
		value = extractConfig(line, "server_name");
		if (!value.empty())
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
		}
		value = extractConfig(line, "max_client_body_size");
		if (!value.empty())
		{
			s1.max_client_body_size = std::stoi(value);//TODO: check
			//std::cout << s1.max_client_body_size << "-->body size from struct\n";
		}
		value = extractConfig(line, "max_client_header_size");
		if (!value.empty())
		{
			s1.max_client_header_size = std::stoi(value);//TODO: check
			//std::cout << s1.max_client_header_size << "-->header size from struct\n";
		}
		value = extractConfig(line, "root");
		if (!value.empty())
		{
			s1.root = value;
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
					int code = std::stoi(codeString);//check
					s1.error_pages[code] = path;
				}
				//else error message? throwing?
			}
		}
       	if (braceCount == 0)
      	{
    		//std::cout << "End of server block\n";
           	break;
        }
	}
	// std::cout << "Total locations parsed: " << s1.locations.size() << std::endl;
	// for (const auto &entry : s1.error_pages)
	// {
	// 	std::cout << entry.first << " => " << entry.second << " --> error page\n";
	// }
	return s1;
}

int ConfigParse::confParse(std::string &filename)
{
	if (std::filesystem::path(filename).extension() != ".conf")
		throw(std::runtime_error("Invalid file extension"));
	std::ifstream file;
	file.open(filename);
	if (file.fail())
	{
		std::cout << "Error opening config file" << std::endl;
		return 1;
	}
	std::string line;
	bool insideBlock = false;
	// std::set<std::pair<std::string, int>> seenPairs; 
	while (std::getline(file, line))
	{
		line = cleanLine(line);
		if (line.empty())
			continue;
		if (!insideBlock)
		{
			if (line.find("server") != std::string::npos) 
			{
				if (line.find('{') != std::string::npos)
				{
					ServerConfig s = parseServerBlock(file);
					// std::pair<std::string, int> hpPair = {s.host, s.listen_port};
                    // if (seenPairs.find(hpPair) != seenPairs.end())
					// {
                    //     throw(std::runtime_error("Parsing error: duplicate listen host and port found"));
					// }
					// seenPairs.insert(hpPair);
					servers.push_back(s);
				}
				else
					insideBlock = true;
				continue;
			}
		}
		else
		{
			if (line.find('{') != std::string::npos)
			{
				ServerConfig s = parseServerBlock(file);
				// std::pair<std::string, int> hpPair = {s.host, s.listen_port};
				// if (seenPairs.find(hpPair) != seenPairs.end())
				// {
				// 	throw std::runtime_error("Parsing error: duplicate listen host and port found");
				// }
				// seenPairs.insert(hpPair);
				servers.push_back(s);
				insideBlock = false;
			}
			else
				throw(std::runtime_error("Error: expected '{' after server directive\n"));
		}
	}
	file.close();
	return 0;
}

const std::vector<ServerConfig> &ConfigParse::getServers() const{
	return servers;
}