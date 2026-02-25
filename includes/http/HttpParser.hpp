#ifndef WEBSERV_HTTP_HTTPPARSER_HPP
#define WEBSERV_HTTP_HTTPPARSER_HPP

#include <string>
#include <cstddef>
#include "http/HttpRequest.hpp"

enum ParseState
{
	REQUEST_LINE,
	HEADERS,
	BODY,
	CHUNK_SIZE,
	CHUNK_DATA,
	TRAILERS,
	DONE
};

enum ParseResult
{
	PARSE_INCOMPLETE,
	PARSE_COMPLETE,
	PARSE_ERROR
};

class HttpParser
{
public:
	explicit HttpParser(size_t max_body_size = 1048576);
	ParseResult feed(std::string& rbuf);
	const HttpRequest& request() const;
	int error_code() const;
	void reset();

private:
	ParseState _state;
	HttpRequest _request;
	size_t _body_remaining;
	size_t _header_bytes;
	size_t _max_body_size;
	int _error_code;
	bool _seen_content_length;

ParseResult parse_request_line(std::string& rbuf);
	ParseResult parse_headers(std::string& rbuf);
	ParseResult parse_body_cl(std::string& rbuf);
	ParseResult parse_chunk_size(std::string& rbuf);
	ParseResult parse_chunk_data(std::string& rbuf);
	ParseResult parse_trailers(std::string& rbuf);
};

#endif // WEBSERV_HTTP_HTTPPARSER_HPP
