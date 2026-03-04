/* ************************************************************************** */
/*  test_http_request_parser.cpp - TDD tests for HttpRequestParser           */
/*                                                                            */
/*  All stubs return defaults so every test is expected to FAIL until the     */
/*  production implementation replaces them.                                  */
/*  Compiles with: -std=c++98 -Wall -Wextra -Werror                          */
/* ************************************************************************** */

#include "minitest.hpp"

#include <string>
#include <map>
#include <cstddef>

/* ======================================================================== */
/*  Stub structures — replace with real includes once production code exists */
/* ======================================================================== */

struct HttpRequest
{
	std::string                        method;
	std::string                        uri;
	std::string                        path;
	std::string                        queryString;
	std::string                        httpVersion;
	std::map<std::string, std::string> headers;
	std::string                        body;
	int                                errorCode;

	HttpRequest()
		: method()
		, uri()
		, path()
		, queryString()
		, httpVersion()
		, headers()
		, body()
		, errorCode(0)
	{}
};

class HttpRequestParser
{
public:
	HttpRequestParser()
		: _maxBodySize(0)
	{}

	void feed(const std::string& /* data */)
	{
		/* stub: no-op */
	}

	bool isComplete() const
	{
		/* stub: always incomplete */
		return false;
	}

	HttpRequest getRequest() const
	{
		/* stub: return default (empty) request */
		return HttpRequest();
	}

	void setMaxBodySize(std::size_t size)
	{
		(void)size;
		/* stub: no-op */
	}

private:
	std::size_t _maxBodySize;
};

/* ======================================================================== */
/*  Helper: build a std::string that may contain embedded \r\n              */
/* ======================================================================== */

static std::string S(const char* s)
{
	return std::string(s);
}

/* ======================================================================== */
/*  Tests                                                                    */
/* ======================================================================== */

/* 1. Simple GET request -------------------------------------------------- */
TEST(HttpRequestParser, GetRequestParsed)
{
	HttpRequestParser parser;
	parser.feed(S("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"));

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(S("GET"),  req.method);
	ASSERT_EQ(S("/"),    req.uri);
	ASSERT_EQ(S("1.1"),  req.httpVersion);
	ASSERT_EQ(0,         req.errorCode);
}

/* 2. POST with Content-Length -------------------------------------------- */
TEST(HttpRequestParser, PostWithContentLength)
{
	HttpRequestParser parser;
	parser.feed(S("POST /submit HTTP/1.1\r\n"
	              "Host: localhost\r\n"
	              "Content-Length: 13\r\n"
	              "\r\n"
	              "Hello, World!"));

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(S("POST"),          req.method);
	ASSERT_EQ(S("/submit"),       req.uri);
	ASSERT_EQ(S("Hello, World!"), req.body);
	ASSERT_EQ(0,                  req.errorCode);
}

/* 3. POST with chunked transfer encoding --------------------------------- */
TEST(HttpRequestParser, PostChunkedEncoding)
{
	HttpRequestParser parser;
	parser.feed(S("POST /data HTTP/1.1\r\n"
	              "Host: localhost\r\n"
	              "Transfer-Encoding: chunked\r\n"
	              "\r\n"
	              "7\r\n"
	              "Mozilla\r\n"
	              "9\r\n"
	              "Developer\r\n"
	              "7\r\n"
	              "Network\r\n"
	              "0\r\n"
	              "\r\n"));

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(S("POST"),                    req.method);
	ASSERT_EQ(S("MozillaDeveloperNetwork"), req.body);
	ASSERT_EQ(0,                            req.errorCode);
}

/* 4. Missing Host header returns 400 ------------------------------------- */
TEST(HttpRequestParser, MissingHostReturns400)
{
	HttpRequestParser parser;
	parser.feed(S("GET / HTTP/1.1\r\n"
	              "\r\n"));

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(400, req.errorCode);
}

/* 5. URI query-string split ---------------------------------------------- */
TEST(HttpRequestParser, UriQueryStringSplit)
{
	HttpRequestParser parser;
	parser.feed(S("GET /search?q=test&page=1 HTTP/1.1\r\n"
	              "Host: localhost\r\n"
	              "\r\n"));

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(S("/search"),        req.path);
	ASSERT_EQ(S("q=test&page=1"), req.queryString);
	ASSERT_EQ(0,                  req.errorCode);
}

/* 6. Headers are stored / accessible case-insensitively ------------------- */
TEST(HttpRequestParser, HeadersCaseInsensitive)
{
	HttpRequestParser parser;
	parser.feed(S("GET / HTTP/1.1\r\n"
	              "Host: localhost\r\n"
	              "Content-Type: text/plain\r\n"
	              "\r\n"));

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();

	/*
	 * The parser should normalise header names to a canonical form
	 * (e.g. all-lowercase) so lookups are case-insensitive.
	 */
	ASSERT_TRUE(req.headers.find("content-type") != req.headers.end());
	ASSERT_EQ(S("text/plain"), req.headers["content-type"]);
}

/* 7. Extra whitespace in request-line returns 400 ------------------------- */
TEST(HttpRequestParser, ExtraWhitespaceReturns400)
{
	HttpRequestParser parser;
	parser.feed(S("GET  /  HTTP/1.1\r\n"
	              "Host: localhost\r\n"
	              "\r\n"));

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(400, req.errorCode);
}

/* 8. Missing HTTP version returns 400 ------------------------------------- */
TEST(HttpRequestParser, MissingVersionReturns400)
{
	HttpRequestParser parser;
	parser.feed(S("GET /\r\n"
	              "Host: localhost\r\n"
	              "\r\n"));

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(400, req.errorCode);
}

/* 9. Body exceeding max body size returns 413 ----------------------------- */
TEST(HttpRequestParser, BodyExceedingMaxReturns413)
{
	HttpRequestParser parser;
	parser.setMaxBodySize(10);

	std::string largeBody(100, 'A');
	std::ostringstream oss;
	oss << "POST /upload HTTP/1.1\r\n"
	    << "Host: localhost\r\n"
	    << "Content-Length: 100\r\n"
	    << "\r\n"
	    << largeBody;

	parser.feed(oss.str());

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(413, req.errorCode);
}

/* 10. Partial (truncated) request awaits more data ------------------------ */
TEST(HttpRequestParser, PartialRequestAwaitsMore)
{
	HttpRequestParser parser;
	parser.feed(S("GET / HTTP/1.1\r\n"
	              "Host: local"));

	/* Not yet terminated with \r\n\r\n — should be incomplete */
	ASSERT_FALSE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(0, req.errorCode);
}

/* 11. Pipelined requests — first parsed independently --------------------- */
TEST(HttpRequestParser, PipelinedRequests)
{
	HttpRequestParser parser;
	parser.feed(S("GET /first HTTP/1.1\r\n"
	              "Host: localhost\r\n"
	              "\r\n"
	              "GET /second HTTP/1.1\r\n"
	              "Host: localhost\r\n"
	              "\r\n"));

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(S("GET"),    req.method);
	ASSERT_EQ(S("/first"), req.uri);
	ASSERT_EQ(0,           req.errorCode);
}

/* ======================================================================== */
/*  Runner                                                                   */
/* ======================================================================== */

MINITEST_MAIN()
