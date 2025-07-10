#pragma once
#include <exception>

class ErrorResponseException : public std::exception
{
	private:
		int responseStatus;
	public:
		const char *what() const noexcept override;
		ErrorResponseException(int status);
		int getResponseStatus(void);
};
