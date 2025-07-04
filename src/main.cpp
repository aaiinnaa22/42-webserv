#include "../inc/Server.hpp"
#include "../inc/ConfigParse.hpp"

bool gSignalClose = false;

void signal_handler(int signal){gSignalClose = true;}

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
	if (servers.empty()) 
	{
		std::cerr << "No server blocks parsed! servers vector is empty.\n";
		return 1;
	}
	else
	{
		try
		{
			for (int i = 0; i < servers.size(); i++)
			{
				std::cout << servers[i].listen_port << " --> listen port\n";
				std::cout << servers[i].host << " --> host\n";
			}
			Server server;
			server.startServer(servers);//(servers[0].listen_port, servers[0].host);
		}
		catch(std::exception& e){
			std::cerr << e.what() << std::endl;
		}
	}
	return 0;
}
