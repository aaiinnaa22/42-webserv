/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   urlCoding.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 12:52:57 by aalbrech          #+#    #+#             */
/*   Updated: 2025/08/04 17:15:45 by aalbrech         ###   ########.fr       */
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
	else 
		throw ErrorResponseException(400);
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
				throw ErrorResponseException(400);
			char first = decodeThis[i + 1];
			char second = decodeThis[i + 2];
			decoded += (hexToChar(first) << 4) | hexToChar(second);
			i += 2;
		}
		else if (decodeThis[i] == '+')
			decoded += " ";
		else 
			decoded += decodeThis[i];
		
	}
	decodeThis = decoded;
}

static std::string charToPercentHex(unsigned char c) 
{
    const char hexDigits[] = "0123456789ABCDEF";
    std::string res;
    res += '%';
    res += hexDigits[c >> 4];
    res += hexDigits[c & 0x0F];
    return res;
}

void HttpRequest::encodeUrl(std::string& encodeThis)
{
    std::string encoded;

    for (size_t i = 0; i < encodeThis.length(); ++i)
    {
        unsigned char c = encodeThis[i];
        // unreserved characters according to RFC 3986
        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~' ||
            c == '/')  // allow '/' unencoded
        {
            encoded += c;
        }
        else if (c == ' ')
        {
            // encode space as %20, not '+'
            encoded += "%20";
        }
        else
        {
            encoded += charToPercentHex(c);
        }
    }
    encodeThis = encoded;
}

void HttpRequest::escapeHtml(std::string& encodeThis)
{
	std::string encoded;
    for (size_t i = 0; i < encodeThis.size(); ++i) {
        switch (encodeThis[i]) {
            case '&':  encoded.append("&amp;");  break;
            case '<':  encoded.append("&lt;");   break;
            case '>':  encoded.append("&gt;");   break;
            case '"':  encoded.append("&quot;"); break;
            case '\'': encoded.append("&#39;");  break;
            default:   encoded.push_back(encodeThis[i]); break;
        }
    }
	encodeThis = encoded;
}
