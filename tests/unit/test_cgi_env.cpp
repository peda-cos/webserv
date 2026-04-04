#include "minitest.hpp"
#include "CgiEnvBuilder.hpp"
#include "HttpRequest.hpp"

#include <string>
#include <map>

static std::map<std::string, std::string> envp_to_map(char** envp) {
	std::map<std::string, std::string> env;
	if (!envp)
		return env;
	for (std::size_t i = 0; envp[i] != NULL; ++i) {
		std::string entry(envp[i]);
		std::size_t pos = entry.find('=');
		if (pos != std::string::npos)
			env[entry.substr(0, pos)] = entry.substr(pos + 1);
	}
	return env;
}

TEST(CgiEnv, RequestMethodGet)
{
	HttpRequest req;
	req.setMethod(GET)
	   .setUriPath("/cgi-bin/test.py")
	   .setVersion("HTTP/1.1");

	CgiEnvBuilder builder(req);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());
	ASSERT_EQ(std::string("GET"), env["REQUEST_METHOD"]);
}

TEST(CgiEnv, ContentTypeFromLowercaseHeaders)
{
	HttpRequest req;
	req.setMethod(POST)
	   .setUriPath("/cgi-bin/api.py")
	   .setVersion("HTTP/1.1")
	   .setBody("{}")
	   .addHeader("content-type", "application/json");

	CgiEnvBuilder builder(req);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());
	ASSERT_EQ(std::string("application/json"), env["CONTENT_TYPE"]);
}

TEST(CgiEnv, QueryStringUsesParsedField)
{
	HttpRequest req;
	req.setMethod(GET)
	   .setUriPath("/cgi-bin/search.py")
	   .setVersion("HTTP/1.1")
	   .addQueryParameter("key", "value");

	CgiEnvBuilder builder(req);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());
	ASSERT_EQ(std::string("key=value"), env["QUERY_STRING"]);
}

TEST(CgiEnv, HttpHeadersAreExported)
{
	HttpRequest req;
	req.setMethod(GET)
	   .setUriPath("/cgi-bin/test.py")
	   .setVersion("HTTP/1.1")
	   .addHeader("host", "example.com")
	   .addHeader("x-custom-header", "abc");

	CgiEnvBuilder builder(req);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());
	ASSERT_EQ(std::string("example.com"), env["HTTP_HOST"]);
	ASSERT_EQ(std::string("abc"), env["HTTP_X_CUSTOM_HEADER"]);
}

TEST(CgiEnv, ScriptNameUsesParsedPath)
{
	HttpRequest req;
	req.setMethod(GET)
	   .setUri("/cgi-bin/test.py?foo=bar")
	   .setPath("/cgi-bin/test.py")
	   .setQueryString("foo=bar")
	   .setVersion("HTTP/1.1");

	CgiEnvBuilder builder(req);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());
	ASSERT_EQ(std::string("/cgi-bin/test.py"), env["SCRIPT_NAME"]);
	ASSERT_EQ(std::string("foo=bar"), env["QUERY_STRING"]);
}

MINITEST_MAIN()
