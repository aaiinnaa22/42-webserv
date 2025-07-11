#include "../inc/Server.hpp"
#include "../inc/ConfigParse.hpp"

bool gSignalClose = false;

void signal_handler(int signal){ (void)signal; gSignalClose = true;}

int main(int argc, char **argv)
{
	std::string	confFile;
	if (argc == 2)
		confFile = argv[1];
	else
	{	
		confFile = "conf/basic.conf";
		std::cout << "No config file provided. Using default: basic.conf" << std::endl;
	}
	signal(SIGINT, signal_handler);
	ConfigParse parser;
	try 
	{
		parser.confParse(confFile);
	}
	catch (const std::exception &e) 
	{
		std::cerr << "Fatal config error: " << e.what() << std::endl;
		return 1;
	}
	const std::vector<ServerConfig> &servers = parser.getServers();
	if (servers.empty() || servers.size() > 5) 
	{
		if (servers.empty())
			std::cerr << "No server blocks parsed! servers vector is empty.\n";
		else
			std::cerr << "Too many server blocks in the config file.\n";
		return 1;
	}
	else
	{
		try
		{
			for (size_t i = 0; i < servers.size(); i++)
			{
				std::cout << servers[i].listen_port << " --> listen port\n";
				std::cout << servers[i].host << " --> host\n";
			}
			Server server;
			server.startServer(servers);
		}
		catch(std::exception& e){
			std::cerr << e.what() << std::endl;
		}
	}
	return 0;
}
