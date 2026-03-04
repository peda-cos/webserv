/* ************************************************************************** */
/*  test_http_response_builder.cpp – unit tests for HttpResponseBuilder      */
/*  All stubs return empty/default values so every test fails deterministically */
/* ************************************************************************** */

#include "minitest.hpp"

#include <string>
#include <map>
#include <sstream>

/* ======================================================================== */
/*  Stub: HttpResponse                                                      */
/* ======================================================================== */

struct HttpResponse {
    int                                 statusCode;
    std::string                         reasonPhrase;
    std::map<std::string, std::string>  headers;
    std::string                         body;

    HttpResponse() : statusCode(0) {}
};

/* ======================================================================== */
/*  Stub: HttpResponseBuilder                                               */
/* ======================================================================== */

class HttpResponseBuilder {
public:
    HttpResponseBuilder& setStatus(int code, const std::string& reason)
    {
        (void)code;
        (void)reason;
        return *this;
    }

    HttpResponseBuilder& setHeader(const std::string& key, const std::string& value)
    {
        (void)key;
        (void)value;
        return *this;
    }

    HttpResponseBuilder& setBody(const std::string& body)
    {
        (void)body;
        return *this;
    }

    std::string build() const
    {
        return "";
    }

    static std::string buildErrorPage(int code, const std::string& customPath)
    {
        (void)code;
        (void)customPath;
        return "";
    }

    static std::string buildRedirect(int code, const std::string& location)
    {
        (void)code;
        (void)location;
        return "";
    }
};

/* ======================================================================== */
/*  Tests                                                                   */
/* ======================================================================== */

/* 1. Status line must begin with "HTTP/1.1 200 OK\r\n" */
TEST(HttpResponseBuilder, StatusLineFormat)
{
    HttpResponseBuilder builder;
    std::string response = builder
        .setStatus(200, "OK")
        .build();

    std::string expected = "HTTP/1.1 200 OK\r\n";
    std::string actual   = response.substr(0, expected.size());
    ASSERT_EQ(expected, actual);
}

/* 2. Every built response must contain Content-Type, Content-Length, and
 *    Connection headers. */
TEST(HttpResponseBuilder, RequiredHeadersPresent)
{
    HttpResponseBuilder builder;
    std::string response = builder
        .setStatus(200, "OK")
        .setBody("test")
        .build();

    ASSERT_STR_CONTAINS(response, "Content-Type:");
    ASSERT_STR_CONTAINS(response, "Content-Length:");
    ASSERT_STR_CONTAINS(response, "Connection:");
}

/* 3. Content-Length must match the actual body size. */
TEST(HttpResponseBuilder, ContentLengthMatchesBody)
{
    HttpResponseBuilder builder;
    std::string response = builder
        .setStatus(200, "OK")
        .setBody("hello")
        .build();

    ASSERT_STR_CONTAINS(response, "Content-Length: 5");
}

/* 4. buildErrorPage with no custom path returns default HTML containing the
 *    status code. */
TEST(HttpResponseBuilder, DefaultErrorPage)
{
    std::string page = HttpResponseBuilder::buildErrorPage(404, "");

    ASSERT_STR_CONTAINS(page, "404");
}

/* 5. buildErrorPage with a custom path references that path in the output. */
TEST(HttpResponseBuilder, CustomErrorPage)
{
    std::string page = HttpResponseBuilder::buildErrorPage(404, "/errors/404.html");

    ASSERT_STR_CONTAINS(page, "/errors/404.html");
}

/* 6. buildRedirect must include a Location header pointing to the target. */
TEST(HttpResponseBuilder, RedirectHasLocationHeader)
{
    std::string response = HttpResponseBuilder::buildRedirect(301, "/new");

    ASSERT_STR_CONTAINS(response, "Location: /new");
}

MINITEST_MAIN()
