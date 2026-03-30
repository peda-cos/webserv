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
#include <sstream>

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"

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

/* 12. Chunked body exceeding max size in single buffer returns 413 --------- */
TEST(HttpRequestParser, ChunkedBodyExceedingMaxInSingleBufferReturns413)
{
	HttpRequestParser parser;
	parser.setMaxBodySize(10);

	// Chunked-encoded body: "Hello World!" (12 bytes) encoded as:
	// "c\r\nHello World!\r\n0\r\n\r\n"
	std::string chunkedBody = "c\r\nHello World!\r\n0\r\n\r\n";
	std::ostringstream oss;
	oss << "POST /upload HTTP/1.1\r\n"
	    << "Host: localhost\r\n"
	    << "Transfer-Encoding: chunked\r\n"
	    << "\r\n"
	    << chunkedBody;

	parser.feed(oss.str());

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(413, req.errorCode);
}

/* 13. Chunked body within max size in single buffer accepted --------------- */
TEST(HttpRequestParser, ChunkedBodyWithinMaxInSingleBufferAccepted)
{
	HttpRequestParser parser;
	parser.setMaxBodySize(20);

	// Chunked-encoded body: "Hello World!" (12 bytes) - within limit
	std::string chunkedBody = "c\r\nHello World!\r\n0\r\n\r\n";
	std::ostringstream oss;
	oss << "POST /upload HTTP/1.1\r\n"
	    << "Host: localhost\r\n"
	    << "Transfer-Encoding: chunked\r\n"
	    << "\r\n"
	    << chunkedBody;

	parser.feed(oss.str());

	ASSERT_TRUE(parser.isComplete());

	HttpRequest req = parser.getRequest();
	ASSERT_EQ(0, req.errorCode);
	ASSERT_EQ(S("Hello World!"), req.body);
}

/* 14. Headers split across two feed() calls -------------------------------- */
TEST(HttpRequestParser, HeadersSplitAcrossTwoFeeds)
{
	HttpRequestParser parser;

	// First feed: partial headers (no \r\n\r\n yet)
	parser.feed(S("GET / HTTP/1.1\r\n"
	              "Host: local"));

	// Should be incomplete after first feed
	ASSERT_FALSE(parser.isComplete());
	HttpRequest req1 = parser.getRequest();
	ASSERT_EQ(0, req1.errorCode);

	// Second feed: complete the headers
	parser.feed(S("host\r\n"
	              "\r\n"));

	// Should be complete now
	ASSERT_TRUE(parser.isComplete());
	HttpRequest req2 = parser.getRequest();
	ASSERT_EQ(0, req2.errorCode);
	ASSERT_EQ(S("GET"), req2.method);
	ASSERT_EQ(S("/"), req2.uri);
	ASSERT_EQ(S("1.1"), req2.httpVersion);
}

/* 15. Content-Length body arriving in two separate feed() calls ------------ */
TEST(HttpRequestParser, ContentLengthBodySplitAcrossTwoFeeds)
{
	HttpRequestParser parser;

	// First feed: headers + partial body
	parser.feed(S("POST /submit HTTP/1.1\r\n"
	              "Host: localhost\r\n"
	              "Content-Length: 10\r\n"
	              "\r\n"
	              "Hello"));

	// Should be incomplete - only 5 of 10 body bytes received
	ASSERT_FALSE(parser.isComplete());
	HttpRequest req1 = parser.getRequest();
	ASSERT_EQ(0, req1.errorCode);

	// Second feed: remaining body bytes
	parser.feed(S("World"));

	// Should be complete now
	ASSERT_TRUE(parser.isComplete());
	HttpRequest req2 = parser.getRequest();
	ASSERT_EQ(0, req2.errorCode);
	ASSERT_EQ(S("POST"), req2.method);
	ASSERT_EQ(S("/submit"), req2.uri);
	// Body should NOT be duplicated - should be exactly "HelloWorld"
	ASSERT_EQ(S("HelloWorld"), req2.body);
}

/* 16. Chunked body split across multiple feed() calls ---------------------- */
TEST(HttpRequestParser, ChunkedBodySplitAcrossMultipleFeeds)
{
	HttpRequestParser parser;

	// First feed: headers + first chunk size + partial data
	parser.feed(S("POST /data HTTP/1.1\r\n"
	              "Host: localhost\r\n"
	              "Transfer-Encoding: chunked\r\n"
	              "\r\n"
	              "7\r\n"
	              "Moz"));

	// Should be incomplete - chunked body not complete
	ASSERT_FALSE(parser.isComplete());

	// Second feed: rest of first chunk + second chunk + terminator
	parser.feed(S("illa\r\n"
	              "9\r\n"
	              "Developer\r\n"
	              "0\r\n"
	              "\r\n"));

	// Should be complete now
	ASSERT_TRUE(parser.isComplete());
	HttpRequest req = parser.getRequest();
	ASSERT_EQ(0, req.errorCode);
	ASSERT_EQ(S("POST"), req.method);
	ASSERT_EQ(S("/data"), req.uri);
	ASSERT_EQ(S("MozillaDeveloper"), req.body);
}

/* ======================================================================== */
/*  Runner                                                                   */
/* ======================================================================== */

MINITEST_MAIN()
