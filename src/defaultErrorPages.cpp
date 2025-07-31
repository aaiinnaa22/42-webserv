/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   defaultErrorPages.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hskrzypi <hskrzypi@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:00:50 by hskrzypi          #+#    #+#             */
/*   Updated: 2025/07/31 17:00:52 by hskrzypi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/webserv.hpp"

std::map<int, std::string> getSetDefaultErrorPages(std::map<int, std::string> setThesePages)
{
	static std::map<int, std::string> webservDefaultErrorPages;
	if (!setThesePages.empty())
		webservDefaultErrorPages = setThesePages;
	return (webservDefaultErrorPages);
}
