#include "../inc/ConfigParse.hpp"

//simple directives and block directives
//TO DO: nested blocks, directive parsing, errors????


int ConfigParse::parseServerBlock(std::ifstream &file)
{
	std::cout << "Parse server block call\n";
	std::string line;
	int braceCount = 1;
	while (std::getline(file, line))
	{
        if (line.find('{') != std::string::npos)
            ++braceCount;
        if (line.find('}') != std::string::npos)
            --braceCount;
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
