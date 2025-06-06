#include "../inc/ConfigParse.hpp"

//simple directives and block directives
//TO DO: nested blocks, directive parsing, errors????

//TO DO SATURDAY: cgi_path_php, cgi_path_python, dir_listing, and... return 307 /newDir/; (no idea what this is)
//TO DO SATURDAY: comments are not stripped from the latter part of basic config

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

	size_t pos = line.find("location");
	size_t brace = line.find('{', pos);//check for braces and position??
	std::string path = line.substr(pos + 8, brace - (pos + 8));
	locBlock.path = trim(path);
	int braceCount = 1;
	std::string inLine;
	// std::vector<std::string>nested;
	while (std::getline(file, inLine))
	{
		if (inLine.find('{') != std::string::npos)
           	++braceCount;
        if (inLine.find('}') != std::string::npos)
        	--braceCount;
		if (inLine.empty())
			continue;
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
		if (braceCount == 0)
			break;
	}
	locations.push_back(locBlock);
	std::cout << "Parsed location block:\n";
	std::cout << "  path: " << locBlock.path << "\n";
	std::cout << "  root: " << locBlock.root << "\n";
	std::cout << "  index: " << locBlock.index << "\n";
	std::cout << "  methods:";
	for (size_t i = 0; i < locBlock.methods.size(); ++i)
		std::cout << " " << locBlock.methods[i];
	std::cout << "\n";

	return locBlock;
}

int ConfigParse::parseServerBlock(std::ifstream &file)
{
	std::cout << "Parse server block call\n";
	std::string line;
	int braceCount = 1;
	ServerConfig s1;
	while (std::getline(file, line))
	{
        	if (line.find('{') != std::string::npos)
           		 ++braceCount;
        	if (line.find('}') != std::string::npos)
            		--braceCount;
			if (line.find("location") != std::string::npos)
			{
				LocationConfig l1 = parseLocationBlock(file, line, s1.locations);
				//s1.locations.push_back(l1);
				continue;
			}
			std::string value = extractConfig(line, "listen");
			if (!value.empty())
			{
				s1.listen_port = stoi(value);//TODO: check
				std::cout << s1.listen_port << "-->listen port from struct\n";
			}
			value = extractConfig(line, "host");
			if (!value.empty())
			{
				s1.host = value;
				std::cout << s1.host << "-->host from struct\n";
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
				std::cout << s1.max_client_body_size << "-->body size from struct\n";
			}
			value = extractConfig(line, "max_client_header_size");
			if (!value.empty())
			{
				s1.max_client_header_size = std::stoi(value);//TODO: check
				std::cout << s1.max_client_header_size << "-->header size from struct\n";
			}
			// for (int i = 0; i < s1.server_names.size(); i++)
			// {
			// 	std::cout << "from vector index " << i << " " << s1.server_names[i] << std::endl;
			// }
        	if (braceCount == 0)
       		{
            		std::cout << "End of server block\n";
            		break;
        	}
	}
	return 0;
}

int ConfigParse::confParse(std::string &filename)
{
	std::cout << "messege from confParse" << std::endl;
	if (std::filesystem::path(filename).extension() != ".conf")
	{
		std::cerr << "Invalid file extension" << std::endl;
		return 1;
	}
	std::ifstream file;
	file.open(filename);
	if (file.fail())
	{
		std::cout << "Error opening config file" << std::endl;
		return 1;
	}
	// else
	// 	std::cout << "Sesame opened" << std::endl;
	std::string line;
	bool insideBlock = false;
	while (std::getline(file, line))
	{
		std::size_t commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);
		line.erase(0,line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);
		if (line.empty())
			continue;
		if (!insideBlock)
		{
			if (line.find("server") != std::string::npos) 
			{
				if (line.find('{') != std::string::npos)
					parseServerBlock(file);
				else
					insideBlock = true;
				continue;
			}
		}
		else
		{
			if (line.find('{') != std::string::npos)
			{
				parseServerBlock(file);
				insideBlock = false;
			}
			else
				std::cerr << "Error: expected '{' after server directive\n";
		}
	}
	file.close();
	return 0;
}
