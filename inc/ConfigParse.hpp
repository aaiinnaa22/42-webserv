#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <set>

struct LocationConfig
{
	std::string path = "/";
	std::string root = "";
	std::string index = "";
	std::vector<std::string> methods;
	std::string cgi_path_php = "";
	std::string cgi_path_python = "";
	std::string upload_dir = "";
	bool dir_listing = false;
	int redirect_code = -1;
	std::string redirect_target = "";
};

struct ServerConfig
{
	int	listen_port = 0;
	size_t	max_client_body_size = 1048576; //1MB
	size_t	max_client_header_size = 8192;
	std::string host = "";
	std::string root = "";
	std::vector<std::string> server_names;
	std::map<int, std::string> error_pages;
	std::vector<LocationConfig> locations;
	int getPort() const {return listen_port;}
	const std::string& getHost() const { return host; }
	bool matches(const std::string& header_host) const {
		for (size_t i = 0; i < server_names.size(); ++i) {
			if (server_names[i] == header_host)
				return true;
		}
		return false;
	}
};

class ConfigParse
{
	private:
		std::vector<ServerConfig> servers;
	public:
		int	confParse(std::string &filename);
		ServerConfig	parseServerBlock(std::ifstream &file);
		const std::vector<ServerConfig> &getServers() const;
};

