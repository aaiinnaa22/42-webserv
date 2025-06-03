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
	ConfigParse parser;
	parser.confParse(confFile);
	//startServer();	
	return 0;
}
