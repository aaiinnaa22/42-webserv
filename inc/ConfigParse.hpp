#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <fstream>

class ConfigParse
{
	public:
		int	confParse(std::string &filename);
		int	parseServerBlock(std::ifstream &file);
};

struct ServerConfig
{
	int			listen_port;
	std::string host;
	std::vector<std::string> server_names;
	std::string root;
};