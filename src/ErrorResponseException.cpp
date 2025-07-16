#include "../inc/ErrorResponseException.hpp"

ErrorResponseException::ErrorResponseException(int status)
{
	responseStatus = status;
}

const char *ErrorResponseException::what() const noexcept
{
	return ("Http response is an error");
}

int ErrorResponseException::getResponseStatus(void)
{
	return (responseStatus);
}

ChildError::ChildError(int status, std::string message)
{
	responseStatus = status;
	if (!message.empty())
        std::cout << "Error: " << message << std::endl;
}

const char *ChildError::what() const noexcept
{
	return ("Error occurred in child process\n");
}