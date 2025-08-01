/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   paths.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aalbrech <aalbrech@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 12:45:45 by aalbrech          #+#    #+#             */
/*   Updated: 2025/08/01 13:22:07 by aalbrech         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/HttpRequest.hpp"

void HttpRequest::checkQueryString(void)
{
	size_t pos = originalPath.find('?');
	if (pos == std::string::npos)
		return ;
	queryString = originalPath.substr(0 + pos + 1);
	originalPath = originalPath.substr(0, pos);
}

void HttpRequest::checkPathIsSafe(void) //??
{
	std::filesystem::path canonicalPath;
	try 
	{
		canonicalPath = std::filesystem::weakly_canonical(completePath);
		//weakly_canonical allows us to make a path canonical, 
		//even tho it does not exist (a path does not exist when i try to POST)
	}
	catch (const std::filesystem::filesystem_error& e) 
	{
		if (errno == ENOENT || errno == ENOTDIR)
			throw ErrorResponseException(404);
		else if (errno == EACCES)
			throw ErrorResponseException(403);
		else if (errno == ENAMETOOLONG)
			throw ErrorResponseException(414);
		else 
			throw ErrorResponseException(500);
	}
	if (canonicalPath.string().find(currentLocation.root) != 0)
		throw ErrorResponseException(403);
}

int HttpRequest::checkPathIsDirectory(void)
{
	struct stat path_stat = safeStat(completePath);
	std::cout << "IS PATH DIR?!" << std::endl;
	return (S_ISDIR(path_stat.st_mode));
}

void HttpRequest::findCurrentLocation(ServerConfig config)
{
	int longest_match_len = 0;
	int match_len = 0;
	bool match_found = false;

	for (auto location : config.locations)
	{
		if (originalPath.find(location.path) == 0) //path starts with location path
		{
			match_len = location.path.length();
			if (match_len > longest_match_len)
			{
				currentLocation = location;
				longest_match_len = match_len;
				match_found = true;
			}
		}
		//in case of exact match, return that?
	}
	if (!match_found)
		throw ErrorResponseException(404);
}

void HttpRequest::makeRootAbsolute(std::string& myRoot)
{
	std::filesystem::path root(myRoot);
	try
	{
		if (root.is_relative())
			root = std::filesystem::current_path() / root;
		if (!std::filesystem::exists(root))
			throw ErrorResponseException(404);
		myRoot = std::filesystem::canonical(root);
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		//i try this on conf roots = server error
		throw ErrorResponseException(500);
	}
}

struct stat HttpRequest::safeStat(std::string statThis)
{
	struct stat st;
	if (stat(statThis.c_str(), &st) == -1)
	{
		if (errno == ENOENT || errno == ENOTDIR)
			throw ErrorResponseException(404);
		if (errno == EACCES)
			throw ErrorResponseException(403);
		if (errno == ENAMETOOLONG)
			throw ErrorResponseException(414);
		else
		{
			std::cout << "ERRROOOOORR: " << strerror(errno) << std::endl;
			throw ErrorResponseException(500);
		}
	}
	return (st);
}
