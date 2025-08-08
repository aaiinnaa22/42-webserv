/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 12:25:17 by aalbrech          #+#    #+#             */
/*   Updated: 2025/08/08 15:47:59 by aalbrech         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/HttpRequest.hpp"
#include "../../inc/ClientConnection.hpp"
#include "../../inc/Server.hpp"

std::vector<char *>HttpRequest::setupCgiEnv(ServerConfig config, std::string pathInfo)
{
	std::vector<char *> envp;
	std::string header;
	envVariables.push_back("REQUEST_METHOD=" + method);
	envVariables.push_back("SCRIPT_NAME=" + originalPath);
	envVariables.push_back("SCRIPT_FILENAME=" + completePath);
	envVariables.push_back("SERVER_PROTOCOL=HTTP/1.1");
	envVariables.push_back("SERVER_NAME=" + config.server_names.at(0));
	envVariables.push_back("SERVER_PORT=" + std::to_string(config.listen_port));
	envVariables.push_back("PATH_INFO=" + pathInfo);
	envVariables.push_back("REDIRECT_STATUS=200");
	envVariables.push_back("GATEWAY_INTERFACE=CGI/1.1");

	envVariables.push_back("QUERY_STRING=" + queryString);
	if (headers.find("content-length") != headers.end())
		header = headers.at("content-length");
	else
		header = "0"; 
	envVariables.push_back("CONTENT_LENGTH=" + header);
	if (headers.find("content-type") != headers.end())
		header = headers.at("content-type");
	else 
		header = "";
	envVariables.push_back("CONTENT_TYPE=" + header);
	for (size_t i = 0; i < envVariables.size(); ++i)
	{
		std::cout << "ENV VAR: " << envVariables[i] << std::endl;
		envp.push_back(const_cast<char *>(envVariables[i].c_str()));
	}
	envp.push_back(nullptr);
	return (envp);
}


std::string HttpRequest::getPathInfo(std::string cgiExtension)
{
	std::string pathInfo;
	int lenOfPos = 0;
	size_t posOfPathInfo = std::string::npos;
	if (cgiExtension == ".php")
	{
		posOfPathInfo = completePath.find(".php");
		lenOfPos = 4;
	}
	else if (cgiExtension == ".py")
	{
		posOfPathInfo = completePath.find(".py");
		lenOfPos = 3;
	}
	if (posOfPathInfo == std::string::npos)
		throw ErrorResponseException(500);
	if (posOfPathInfo + 1 < completePath.size())
	{
		pathInfo = completePath.substr(posOfPathInfo + lenOfPos);
		//remove "/" if its the first character?
		completePath = completePath.substr(0, posOfPathInfo + lenOfPos);
	}
	return (pathInfo);
}


void HttpRequest::checkCgiPath(std::string checkThisPath, bool tryIsExecutable)
{
	struct stat pathStat;
	pathStat = safeStat(checkThisPath);
	if (!S_ISREG(pathStat.st_mode))
		throw ErrorResponseException(404);
	if (tryIsExecutable)
	{
		if (access(checkThisPath.c_str(), X_OK) == -1)
		{
			if (errno == EACCES)
				throw ErrorResponseException(403);
			else 
				throw ErrorResponseException(500);
		}
	}
}


static bool isTabOrSpace(char c)
{
	if (c == '\t' || c == ' ')
		return (true);
	return (false);
}


static void parseCgiStatus(std::string status)
{
	int intStatus = -1;
	std::string finalStatus;
	int valueTime = 0;
	
	for (size_t i = 0; i < status.size(); ++i)
	{
		if (i == 0 && isdigit(status[i]))
			valueTime = 1;
		if (isTabOrSpace(status[i]))
		{
			if (i + 1 < status.size() && !isTabOrSpace(status[i + 1]))
				valueTime++;
			if (valueTime == 2)
				break ;
		}
		else if (isdigit(status[i]) && valueTime == 1)
			finalStatus += status[i];
		else
			throw ErrorResponseException(500);
	}
	intStatus = std::stoi(finalStatus);
	if (intStatus == -1)
		return ;
	if (intStatus > 599 || intStatus < 100)
		throw ErrorResponseException(500);
	if (intStatus != 200)
		throw ErrorResponseException(intStatus);
}

static std::string parseCgiContentType(std::string contentType)
{
	int valueTime = 0;
	std::string finalValue;
	
	for (size_t i = 0; i < contentType.size(); ++i)
	{
		if (i == 0 && isalpha(contentType[i]))
			valueTime = 1;
		if (isTabOrSpace(contentType[i]))
		{
			if (i + 1 < contentType.size() && !isTabOrSpace(contentType[i + 1]))
				valueTime++;
		}
		else if (valueTime == 1 && (isalpha(contentType[i]) || contentType[i] == '/'))
			finalValue += contentType[i];
		else if (contentType[i] == ';')
				break ;
		else
			throw ErrorResponseException(500);

	}
	if ((std::count(finalValue.begin(), finalValue.end(), '/')) != 1)
		throw ErrorResponseException(500);
	return (finalValue);
}



static int parseCgiContentLength(std::string contentLength, size_t max_client_body_size)
{
	int valueTime = 0;
	int finalValue = -1;
	std::string value;
	
	for (size_t i = 0; i < contentLength.size(); ++i)
	{
		if (i == 0 && isdigit(contentLength[i]))
			valueTime = 1;
		if (isTabOrSpace(contentLength[i]))
		{
			if (i + 1 < contentLength.size() && !isTabOrSpace(contentLength[i + 1]))
				valueTime++;
		}
		else if (valueTime == 1 && (isdigit(contentLength[i])))
			value += contentLength[i];
		else 
			throw ErrorResponseException(500);

	}
	finalValue = std::stoi(value);
	if (static_cast<size_t>(finalValue) > max_client_body_size)
		throw ErrorResponseException(413); 
	return (finalValue);
}

static std::pair<std::string, int> parseCgiHeaders(std::string cgiHeaders, size_t findLen, size_t max_client_body_size)
{
	std::vector<std::string> headerTypes {"status", "content-type", "content-length"};
	size_t pos;
	size_t endOfHeader;
	std::string header;
	std::string value;
	std::string contentType;
	int contentLength = -1;

	if (cgiHeaders.empty())
		throw ErrorResponseException(500);
	if (!isalpha(cgiHeaders.at(0)))
		throw ErrorResponseException(500);
	ClientConnection::normalize_case(cgiHeaders);
	for (auto headerType : headerTypes)
	{
		pos = cgiHeaders.find(headerType);
		if (pos != std::string::npos)
		{
			if (findLen == 2)
				endOfHeader = cgiHeaders.find("\n", pos);
			else if (findLen == 4)
				endOfHeader = cgiHeaders.find("\r\n", pos);
			if (endOfHeader != std::string::npos)
			{
				if (pos != 0 && cgiHeaders.at(pos - 1) != '\n')
					throw ErrorResponseException(500);
				size_t duplicatePos = cgiHeaders.find(headerType, endOfHeader);
				if (duplicatePos != std::string::npos)
				{
					if (duplicatePos == 0 || cgiHeaders.at(duplicatePos - 1) == '\n')
						throw ErrorResponseException(500);
				}
				std::string currentHeader = cgiHeaders.substr(pos, endOfHeader - pos - (findLen / 2) + 1);
				size_t delimitor = currentHeader.find(":");
				if (delimitor != std::string::npos)
				{
					cgiHeaders.erase(pos, endOfHeader - pos + (findLen / 2));
					header = currentHeader.substr(0, delimitor);
					if (header != headerType)
						throw ErrorResponseException(500);
					value = currentHeader.substr(delimitor + 1);
					if (headerType == "content-type")
						contentType = parseCgiContentType(value);
					else if (headerType == "status")
						parseCgiStatus(value);
					else if (headerType == "content-length")
						contentLength = parseCgiContentLength(value, max_client_body_size);
				}
				else 
					throw ErrorResponseException(500);
			}
			else 
				throw ErrorResponseException(500);
		}
	}
	if (contentType.empty())
		throw ErrorResponseException(500);
	return {contentType, contentLength};
}

void HttpRequest::parseCgiOutput(std::string cgiOutput)
{
	std::string cgiBody;
	std::string cgiHeaders;
	std::pair<std::string, int> headerResult;

	size_t pos = cgiOutput.find("\r\n\r\n");
	size_t findLen = 4;

	if (pos == std::string::npos) 
	{
		pos = cgiOutput.find("\n\n");
		findLen = 2;
	}
	if (pos != std::string::npos)
		cgiBody = cgiOutput.substr(pos + findLen);
	if (pos == std::string::npos)
		throw ErrorResponseException(500);
	cgiHeaders = cgiOutput.substr(0, pos + (findLen / 2));
	
	headerResult = parseCgiHeaders(cgiHeaders, findLen, max_client_body_size);
	if (headerResult.second != -1)
	{
		if (cgiBody.size() < static_cast<size_t>(headerResult.second)) //body is shorter that content length
			throw ErrorResponseException(500);
		cgiBody.resize(headerResult.second);
	}
	
	checkContentType(headerResult.first);
	httpResponse.setResponseHeader("content-type", headerResult.first);
	httpResponse.setResponseBody(cgiBody);
	httpResponse.setStatus(200);
}




void HttpRequest::doCgi(ServerConfig config, std::string cgiExtension, const Server& server)
{
	if (method != "GET" && method != "POST")
		throw ErrorResponseException(405);

	std::string pathInfo;
	ssize_t charsWritten;
	std::string interpreterPath;
	if (cgiExtension == ".py")
		interpreterPath = currentLocation.cgi_path_python;
	else if (cgiExtension == ".php")
		interpreterPath = currentLocation.cgi_path_php; 

	pathInfo = getPathInfo(cgiExtension);
	char *argv[] =
	{
		const_cast<char *>(interpreterPath.c_str()),
		const_cast<char *>(completePath.c_str()),
		nullptr
	};
	std::vector<char *> envp = setupCgiEnv(config, pathInfo);
	
	int stdinWriteFd = open("tempStdin", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
	if (stdinWriteFd == -1)
		throw ErrorResponseException(500);
	charsWritten = write(stdinWriteFd, body.c_str(), body.size());//check for 0, also the nonblocking file
	if (charsWritten == -1)
	{
		close(stdinWriteFd);
		throw ErrorResponseException(500);
	}
	close (stdinWriteFd);
	int stdinFd;
	int stdoutFd;
	stdinFd = open("tempStdin", O_RDONLY | O_CLOEXEC); //test? with post
	if (stdinFd == -1)
		throw ErrorResponseException(500);
	stdoutFd = open("tempStdout", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
	if (stdoutFd == -1)
	{
		close(stdinFd);
		throw ErrorResponseException(500);
	}
	pid_t pid = fork();
	if (pid == -1)
		throw ErrorResponseException(500);
	if (pid == 0)
	{
		(void)argv;
		(void)server;
		try
		{
			if (dup2(stdinFd, STDIN_FILENO) == -1)
			{
				close(stdinFd);
				close(stdoutFd);
				throw ChildError(500, "dup2");
			}
			if (dup2(stdoutFd, STDOUT_FILENO) == -1)
			{
				close(stdinFd);
				close(stdoutFd);
				throw ChildError(500, "dup2");
			}
			std::vector<int> fds = server.get_open_fds();
			for (size_t i = 0; i < fds.size(); ++i)
    		close(fds[i]);
			close(stdinFd);
			close(stdoutFd);
			if (chdir(currentLocation.root.c_str()) != 0) //test
				throw ChildError(500, "chdir");
			execve(interpreterPath.c_str(), argv, envp.data());
			std::cerr << "Execve call fail, cleaning fds...\n ! \n !\n !\n";
			throw ChildError(500, "execve");
			
			// std::vector<int> fds_to_close = server.get_open_fds();
			// for (size_t i = 0; i < fds_to_close.size(); ++i)
			// 	close(fds_to_close[i]);
			// server.~Server();
		}
		catch (ChildError& e)
		{
			throw ChildError(500);
		}
	}
	else
	{
		close(stdinFd);
		close(stdoutFd);
		int status = 0;
		const int TIMEOUT_SECONDS = 10;
		std::time_t start_time = std::time(nullptr);
		while (true)
		{
			pid_t result = waitpid(pid, &status, WNOHANG);
			if (result == -1)
				throw ErrorResponseException(500);
			else if (result > 0)
			{
				break;
			}
			std::time_t now = std::time(nullptr);
			if (now - start_time > TIMEOUT_SECONDS)
			{
				kill(pid, SIGKILL);
				waitpid(pid, NULL, 0);
				throw ErrorResponseException(500);
			}
			usleep(500000);
		}
		std::cout << "CHILD STATUS: " << status << std::endl;
		//test begin
		// int stdErrdebugOpen = open("tempStderrDEBUG", O_RDONLY);
		// std::cout << "DEBUG CGI ERROR OUTPUT" << std::endl;
		// int readDebug;
		// char buf[1000];
		// while ((readDebug = read(stdErrdebugOpen, buf, sizeof(buf))) > 0)
		// 	std::cout << buf << std::endl;
		// std::cout << "DEBUG DONE" << std::endl;
		//test done
		if (WIFSIGNALED(status)) 
		{
 			int sig = WTERMSIG(status);
    		std::cerr << "CGI script terminated by signal: " << sig << std::endl;
    		throw ErrorResponseException(500);
		}

		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		{
			std::cerr << "CGI script exited with code: " << WEXITSTATUS(status) << std::endl;
			throw ErrorResponseException(500);
		}

		int stdoutReadFd = open("tempStdout", O_RDONLY);
		if (stdoutReadFd == -1)
			throw ErrorResponseException(500);
		std::string cgiOutput;
		char buffer[1000];
		ssize_t charsRead;
		//read from stdout
		while ((charsRead = read(stdoutReadFd, buffer, sizeof(buffer))) > 0)//READ CHECKS FOR 0 and -1
			cgiOutput.append(buffer, charsRead);
		if (charsRead == -1)
		{
			close(stdoutReadFd);
			throw ErrorResponseException(500);
		}
		close(stdoutReadFd);
		std::cout << "time to parse cgi output" << std::endl;
		parseCgiOutput(cgiOutput);
	}
}


std::string HttpRequest::checkRequestIsCgi(void)
{	
	size_t pos = completePath.size();
	bool isCgi = false;
	std::string cgiExtension;
	std::string pathToTry;
	while (pos > 0)
	{
		pathToTry = completePath.substr(0, pos);
		try 
		{
			checkCgiPath(pathToTry);
		}
		catch (ErrorResponseException& e)
		{
			if (e.getResponseStatus() == 500)
				throw ErrorResponseException(500);
			pos = completePath.rfind('/', pos -1);
			continue ;
		}
		if (pathToTry.ends_with(".py"))
		{
			isCgi = true;
			cgiExtension = ".py";
			break ;
		}
		else if (pathToTry.ends_with(".php"))
		{
			isCgi = true;
			cgiExtension = ".php";
			break ;
		}
		pos = completePath.rfind('/', pos -1); //size_t can become huge in case of negative value!!!
	}
	if (isCgi == false)
		return ("");
	checkCgiPath(pathToTry, true);
	try 
	{
		if (cgiExtension == ".py")
			checkCgiPath(currentLocation.cgi_path_python, true);
		else if (cgiExtension == ".php")
			checkCgiPath(currentLocation.cgi_path_php, true);
	}
	catch (ErrorResponseException& e)
	{
		throw ErrorResponseException(500);
	}
	return (cgiExtension);
}