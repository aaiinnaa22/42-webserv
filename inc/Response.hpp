#pragma
#include <string>
#include <map>

class Response
{
	private:
		int statusCode;
		std::string statusMessage;
		std::string httpVersion;
		std::map<std::string, std::string> headers;
		std::string body;
		//a map of reason phrases? with a switch?
	public:
		//set statusCode
		//set statusMessage
		//set body
		//set header

};
