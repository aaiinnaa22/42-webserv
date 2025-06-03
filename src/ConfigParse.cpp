#include "../inc/ConfigParse.hpp"

//simple directives and block directives

int ConfigParse::parseServerBlock(std::ifstream &file)
{
	std::cout << "Parse server block call\n";
	return 0;
}

int ConfigParse::confParse(std::string &filename)
{
	std::cout << "messege from confParse" << std::endl;
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
		if (line.find("server") != std::string::npos && line.find("{") != std::string::npos)
			parseServerBlock(file);
	}
	file.close();
	return 0;
}
