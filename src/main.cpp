#include "../inc/Server.hpp"

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	try {
		Server server;
		server.startServer();}
	catch(std::exception& e){
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
