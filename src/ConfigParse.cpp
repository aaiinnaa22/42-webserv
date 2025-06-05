#include "../inc/ConfigParse.hpp"

//simple directives and block directives
//TO DO: nested blocks, directive parsing, errors????

std::string trim(const std::string &toTrim)
{
	size_t pre = toTrim.find_first_not_of(" \t\n\r");
	if (pre == std::string::npos)
		throw std::runtime_error("missing info");
	size_t post = toTrim.find_last_not_of(" \t\n\r;");
	return toTrim.substr(pre, (post - pre + 1));
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
			size_t pos = line.find("listen");
			if (pos != std::string::npos)
			{
				std::string port = line.substr(pos + 6);
				port = trim(port);
				s1.listen_port = std::stoi(port);
				std::cout << s1.listen_port << std::endl;
			}
			pos = line.find("host");
			if (pos != std::string::npos)
			{
				std::string host = line.substr(pos + 4);
				host = trim(host);
				s1.host = host;
				std::cout << s1.host << std::endl;
			}
			pos = line.find("server_name");
			if (pos != std::string::npos)
			{
				std::string names = line.substr(pos + 11);
				names = trim(names);
				size_t start = 0;
				while (start < names.length())
				{
					size_t end = names.find(' ', start);
					if (end == std::string::npos)
						end = names.length();
					std::string oneName = names.substr(start, end - start);
					if (!oneName.empty())
						s1.server_names.push_back(oneName);
					start = end + 1;
				}
			}
			// for (int i = 0; i < s1.server_names.size(); i++)
			// {
			// 	std::cout << "from vector index " << i << " " << s1.server_names[i] << std::endl;
			// }
			std::cout << "Inside server block: " << line << std::endl;
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
	else
		std::cout << "Sesame opened" << std::endl;
	std::string line;
	while (std::getline(file, line))
	{
		std::size_t commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);
		line.erase(0,line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);
		if (line.empty())
			continue;
		if (line.find("server") != std::string::npos && line.find("{") != std::string::npos)
			parseServerBlock(file);
	}
	file.close();
	return 0;
}
