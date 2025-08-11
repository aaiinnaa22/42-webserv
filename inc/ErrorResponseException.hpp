/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorResponseException.hpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: llaakson <llaakson@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 19:03:01 by llaakson          #+#    #+#             */
/*   Updated: 2025/08/11 19:03:05 by llaakson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <exception>
#include <iostream>

class ErrorResponseException : public std::exception
{
	private:
		int responseStatus;
	public:
		const char *what() const noexcept override;
		ErrorResponseException(int status);
		int getResponseStatus(void);
};

class ChildError : public std::exception
{
	private:
		int responseStatus;
	public:
		const char *what() const noexcept override;
		ChildError(int status, std::string message = "");
};
