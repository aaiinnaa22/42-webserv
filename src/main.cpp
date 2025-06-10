#include "../inc/Server.hpp"
#include "../inc/ConfigParse.hpp"

int main(int argc, char **argv)
{
	std::string	confFile;
	if (argc == 2)
		confFile = argv[1];
	else
	{	
		confFile = "basic.conf";
		std::cout << "No config file provided. Using default: basic.conf" << std::endl;
	}
	try {
		ConfigParse parser;
		Server server;

		parser.confParse(confFile);
		const std::vector<ServerConfig> &servers = parser.getServers();
		if (servers.empty()) 
		{
			std::cerr << "No server blocks parsed! servers vector is empty.\n";
			return 1;
		}
		// else
		// {
		// 	std::cout << servers[0].listen_port << " --> listen port\n";
		// 	std::cout << servers[0].host << " --> host\n";
		// 	std::cout << servers[0].root << " --> root\n";
		// 	std::cout << servers[0].max_client_body_size << " --> body size\n";
		// 	std::cout << servers[0].max_client_header_size << " --> header size\n";
		// 	for (int i = 0; i < servers[0].server_names.size(); i++)
		// 		std::cout << servers[0].server_names[i] << " --> server name from vector index " << i << std::endl;
		// 	for (const auto &entry : servers[0].error_pages)
		// 		std::cout << entry.first << " => " << entry.second << " --> error page\n";
		// 	std::cout << "locations vector: \n";
		// 	for (int j = 0; j < servers[0].locations.size(); j++)
		// 	{
		// 		std::cout << "Location from vector index: " << j << std::endl;
		// 		std::cout << "  path: " << servers[0].locations[j].path << std::endl;
		// 		std::cout << "  root: " << servers[0].locations[j].root << std::endl;
		// 		std::cout << "  index: " << servers[0].locations[j].index << std::endl;
		// 		std::cout << "  CGI PHP: " << servers[0].locations[j].cgi_path_php << std::endl;
		// 		std::cout << "  CGI Python: " << servers[0].locations[j].cgi_path_python << std::endl;
		// 		std::cout << "  upload directory: " << servers[0].locations[j].upload_dir << std::endl;
		// 		std::cout << "  directory listing: " << servers[0].locations[j].dir_listing << std::endl;
		// 		std::cout << "  redir code: " << servers[0].locations[j].redirect_code << std::endl;
		// 		std::cout << "  redir target: " << servers[0].locations[j].redirect_target << std::endl;
		// 		std::cout << "  methods: ";
		// 		for (size_t k = 0; k < servers[0].locations[j].methods.size(); k++)
		// 			std::cout << servers[0].locations[j].methods[k] << " ";
		// 		std::cout << std::endl << "------------------" << std::endl;
		// 	}
		// }
		server.startServer(servers[0].listen_port, servers[0].host);
	}
	catch(std::exception& e){
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
