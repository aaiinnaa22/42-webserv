/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: llaakson <llaakson@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 18:58:43 by llaakson          #+#    #+#             */
/*   Updated: 2025/08/11 18:58:48 by llaakson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include "../inc/ConfigParse.hpp"
#include "../inc/ErrorResponseException.hpp"

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
	signal(SIGQUIT, signal_handler);
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
				if (servers[i].root.empty() || servers[i].getHost().empty() || servers[i].getPort() == 0)
					throw std::runtime_error("Info missing in the server block");
				std::cout << "\033[1;34mServer " << i + 1 
					<< "\033[0m | Host: \033[1;32m" << servers[i].host 
          			<< "\033[0m | Port: \033[1;36m" << servers[i].listen_port 
          			<< "\033[0m" << std::endl;
			}
			Server server;
			server.startServer(servers);
			unlink("tempStdin");
			unlink("tempStdout");
		}
		catch (ChildError& e)
		{
			return 1;
		}
		catch(std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			unlink("tempStdin");
			unlink("tempStdout");
		}
	}
	return 0;
}
