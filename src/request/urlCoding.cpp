/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   urlCoding.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 12:52:57 by aalbrech          #+#    #+#             */
/*   Updated: 2025/07/24 12:53:17 by aalbrech         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/HttpRequest.hpp"

static char hexToChar(char c)
{
	if ('0' <= c && c <= '9')
		return (c - '0');
	else if ('a' <= c && c <= 'f')
		return (c - 'a' + 10);
	else if ('A' <= c && c <= 'F')
		return (c - 'A' + 10);
	//throw???
	return (0);
}

void HttpRequest::decodeUrl(std::string& decodeThis)
{
	std::string decoded;

	for (size_t i = 0; i < decodeThis.length(); ++i)
	{
		if (decodeThis[i] == '%')
		{
			if (i + 2 >= decodeThis.length())
			{
				; //throw??
			}
			char first = decodeThis[i + 1];
			char second = decodeThis[i + 2];
			decoded += (hexToChar(first) << 4) | hexToChar(second);
			i += 2;
		}
		else if (decodeThis[i] == '+') //???
			decoded += " ";
		else 
			decoded += decodeThis[i];
		
	}
	decodeThis = decoded;
}