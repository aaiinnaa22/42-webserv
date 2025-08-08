/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   headers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 12:49:33 by aalbrech          #+#    #+#             */
/*   Updated: 2025/08/08 14:29:43 by aalbrech         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/HttpRequest.hpp"

void HttpRequest::checkContentType(std::string responseContentType)
{
	std::vector<std::string> allowedTypes = 
	{
		"text/html",
		"text/css",
		"image/png",
		"image/gif",
		"image/jpeg",
		"text/plain",
		"image/x-icon",
		"application/pdf"
	};
	bool isAllowed = false;
	for (auto type : allowedTypes)
	{
		if (responseContentType == type)
			isAllowed = true;
	}
	if (!isAllowed)
		throw ErrorResponseException(415);
	if (headers.find("accept") != headers.end())
	{
		std::string acceptTheseContentTypes = headers.at("accept");
		if (acceptTheseContentTypes.find("*/*") != std::string::npos)
			return ;
		size_t pos = responseContentType.find('/');
		if (pos != std::string::npos)
		{
			std::string wildCard;
			wildCard = responseContentType.substr(0, pos + 1);
			wildCard += '*';
			if (acceptTheseContentTypes.find(wildCard) != std::string::npos)
				return ;
		}
		if (acceptTheseContentTypes.find(responseContentType) != std::string::npos)
			return ;
		throw ErrorResponseException(406);
	}
}

void HttpRequest::setContentType(int postCheck)
{
	size_t dot;
	std::string fileExtension;
	std::string responseContentType;

	dot = completePath.rfind(".");
	if (dot == std::string::npos)
		throw ErrorResponseException(415);
	fileExtension = completePath.substr(dot + 1, completePath.length());
	if (postCheck == 1)
	{
		if (fileExtension != "jpg" && fileExtension != "jpeg" && fileExtension != "png"
				&& fileExtension != "gif" && fileExtension != "pdf" && fileExtension != "txt")
			throw ErrorResponseException(415);
	}
	if (fileExtension == "html" || fileExtension == "css")
		responseContentType = "text/" + fileExtension;
	else if (fileExtension == "png" || fileExtension == "gif")
		responseContentType = "image/" + fileExtension;
	else if (fileExtension == "jpg" || fileExtension == "jpeg")
		responseContentType = "image/jpeg";
	else if (fileExtension == "txt")
		responseContentType = "text/plain";
	else if (fileExtension == "ico")
		responseContentType = "image/x-icon";
	else if (fileExtension == "pdf")
		responseContentType = "application/pdf";
	else
		throw ErrorResponseException(415);
	checkContentType(responseContentType);
	httpResponse.setResponseHeader("content-type", responseContentType);
}
