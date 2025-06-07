#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>

class ConfigParse
{
	public:
		int	confParse(std::string &filename);
		int	parseServerBlock(std::ifstream &file);
};

struct LocationConfig
{
	std::string path;
	std::string root;
	std::string index;
	std::vector<std::string> methods;
	std::string cgi_path_php;
	std::string cgi_path_python;
	bool dir_listing;
	int redirect_code;
	std::string redirect_target;
};

struct ServerConfig
{
	int	listen_port;
	int	max_client_body_size;
	int	max_client_header_size;
	std::string host;
	std::vector<std::string> server_names;
	std::vector<LocationConfig> locations;
};