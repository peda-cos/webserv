#ifndef WEBSERV_HTTP_HTTPREQUEST_HPP
#define WEBSERV_HTTP_HTTPREQUEST_HPP

#include <string>
#include <vector>
#include <utility>
#include <cstddef>

struct HttpRequest
{
	std::string method;
	std::string target;
	std::string version;
	std::vector<std::pair<std::string, std::string> > headers;
	std::string body;
	size_t content_length;
	bool chunked;

	HttpRequest()
		: content_length(0), chunked(false)
	{
	}
};

#endif // WEBSERV_HTTP_HTTPREQUEST_HPP
