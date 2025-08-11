/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: llaakson <llaakson@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 19:02:01 by llaakson          #+#    #+#             */
/*   Updated: 2025/08/11 19:02:06 by llaakson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include "../inc/ConfigParse.hpp"
#include "webserv.hpp"

class Response
{
	private:
		int statusCode;
		std::string statusMessage;
		std::string httpVersion = "HTTP/1.1";
		std::map<std::string, std::string> headers;
		std::string body;
		std::map<int, std::string> errorPages;
		static const std::unordered_map<int, std::string> reasonPhrases;

	public:
		bool isSent = false;
		void setStatus(int code, const std::string& message = "");
		void setResponseHeader(const std::string &key, const std::string &value);
		void setResponseBody(std::string &bodyContent);

		int getStatusCode() const;
		std::string getStatusMessage() const;
		const std::string getHeader(const std::string &key) const;
		std::string getBody() const;
		std::string toString() const;

		void buildErrorResponse(int statusCode, int clientFd, std::map<int, std::string> errorPages = {});
		void sendResponse(int clientFd, bool isAlive); //public?
		Response() = default;
		Response(int code);
		Response(int code, const std::string& body);
		~Response() {};
};
