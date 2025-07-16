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