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
