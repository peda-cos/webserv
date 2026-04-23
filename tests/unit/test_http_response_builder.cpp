/* ************************************************************************** */
/*  test_http_response_builder.cpp – unit tests for HttpResponseBuilder      */
/* ************************************************************************** */

#include "minitest.hpp"
#include <HttpResponseBuilder.hpp>

#include <string>

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

/* 4. buildErrorPage with no custom body returns default HTML containing the
 *    status code. */
TEST(HttpResponseBuilder, DefaultErrorPage)
{
    std::string page = HttpResponseBuilder::buildErrorPage(404, "");

    ASSERT_STR_CONTAINS(page, "404");
}

/* 5. buildErrorPage with a custom body uses that body. */
TEST(HttpResponseBuilder, CustomErrorPage)
{
    std::string customBody = "<html><body>Custom 404 from /errors/404.html</body></html>";
    std::string page = HttpResponseBuilder::buildErrorPage(404, customBody);

    ASSERT_STR_CONTAINS(page, "/errors/404.html");
}

/* 6. buildRedirect must include a Location header pointing to the target. */
TEST(HttpResponseBuilder, RedirectHasLocationHeader)
{
    std::string response = HttpResponseBuilder::buildRedirect(301, "/new");

    ASSERT_STR_CONTAINS(response, "Location: /new");
}

MINITEST_MAIN()
