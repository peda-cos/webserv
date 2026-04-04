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

/* Helper to create minimal LocationConfig for testing */
LocationConfig create_test_location() {
	LocationConfig loc;
	loc.cgi_handlers[".py"] = "/usr/bin/python3";
	loc.cgi_handlers[".php"] = "/usr/bin/php-cgi";
	loc.root = "/tmp";
	return loc;
}

/* ------------------------------------------------------------------------ */
/* 1. A GET request must set REQUEST_METHOD to "GET".                        */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, RequestMethodGet)
{
	HttpRequest req;
	req.setMethod(GET)
	   .setUriPath("/cgi-bin/test.py")
	   .setVersion("HTTP/1.1");

	LocationConfig loc = create_test_location();
	CgiEnvBuilder builder(req, loc);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());
	ASSERT_EQ(std::string("GET"), env["REQUEST_METHOD"]);
}

/* ------------------------------------------------------------------------ */
/* 2. A POST request must set REQUEST_METHOD to "POST".                     */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, RequestMethodPost)
{
	HttpRequest req;
	req.setMethod(POST)
	   .setUriPath("/cgi-bin/submit.py")
	   .setVersion("HTTP/1.1")
	   .setBody("data=hello");

	LocationConfig loc = create_test_location();
	CgiEnvBuilder builder(req, loc);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());

	ASSERT_EQ(std::string("POST"), env["REQUEST_METHOD"]);
}

/* ------------------------------------------------------------------------ */
/* 3. Content-Type header must be forwarded to CONTENT_TYPE env var.         */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, ContentTypeFromHeaders)
{
	HttpRequest req;
	req.setMethod(POST)
	   .setUriPath("/cgi-bin/api.py")
	   .setVersion("HTTP/1.1")
	   .setBody("{}")
	   .addHeader("content-type", "application/json");

	LocationConfig loc = create_test_location();
	CgiEnvBuilder builder(req, loc);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());
	ASSERT_EQ(std::string("application/json"), env["CONTENT_TYPE"]);
}

/* ------------------------------------------------------------------------ */
/* 4. Content-Length header must be forwarded to CONTENT_LENGTH env var.     */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, ContentLengthFromHeaders)
{
	HttpRequest req;
	req.setMethod(POST)
	   .setUriPath("/cgi-bin/upload.py")
	   .setVersion("HTTP/1.1")
	   .setBody("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	// Note: Content-Length is built from body size in CgiEnvBuilder

	LocationConfig loc = create_test_location();
	CgiEnvBuilder builder(req, loc);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());

	ASSERT_EQ(std::string("44"), env["CONTENT_LENGTH"]);
}

/* ------------------------------------------------------------------------ */
/* 5. PATH_INFO must be extracted from the URI after the script name.        */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, PathInfoFromUri)
{
	HttpRequest req;
	req.setMethod(GET)
	   .setUriPath("/cgi-bin/script.py/extra/path")
	   .setVersion("HTTP/1.1");

	LocationConfig loc = create_test_location();
	CgiEnvBuilder builder(req, loc);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());
	
	// PATH_INFO is extracted from URI after CGI script extension
	// uri_path="/cgi-bin/script.py/extra/path" with .py handler → PATH_INFO="/extra/path"
	ASSERT_EQ(std::string("/extra/path"), env["PATH_INFO"]);
}

/* ------------------------------------------------------------------------ */
/* 6. QUERY_STRING must be extracted from the URI after '?'.                 */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, QueryStringFromUri)
{
	HttpRequest req;
	req.setMethod(GET)
	   .setUriPath("/cgi-bin/search.py")
	   .setVersion("HTTP/1.1")
	   .addQueryParameter("key", "value");

	LocationConfig loc = create_test_location();
	CgiEnvBuilder builder(req, loc);
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

	CgiEnvBuilder builder(req, create_test_location());
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

	LocationConfig loc = create_test_location();
	CgiEnvBuilder builder(req, loc);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());
	ASSERT_EQ(std::string("/cgi-bin/test.py"), env["SCRIPT_NAME"]);
	ASSERT_EQ(std::string("foo=bar"), env["QUERY_STRING"]);

	ASSERT_EQ(std::string("HTTP/1.1"), env["SERVER_PROTOCOL"]);
}

/* ------------------------------------------------------------------------ */
/* 8. Required CGI environment variables must be present.                   */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, EssentialVarsPresent)
{
	HttpRequest req;
	req.setMethod(POST)
	   .setUriPath("/cgi-bin/full.py")
	   .setVersion("HTTP/1.1")
	   .setBody("payload")
	   .addHeader("Content-Type", "text/plain");

	LocationConfig loc = create_test_location();
	CgiEnvBuilder builder(req, loc);
	std::map<std::string, std::string> env = envp_to_map(builder.getEnvp());

	const char* required[] = {
		"REQUEST_METHOD",
		"CONTENT_TYPE",
		"CONTENT_LENGTH",
		"PATH_INFO",
		"QUERY_STRING",
		"SERVER_PROTOCOL",
		"GATEWAY_INTERFACE"
	};

	for (int i = 0; i < 7; ++i) {
		std::string key(required[i]);
        ASSERT_TRUE(env.find(key) != env.end());
        if (key != "QUERY_STRING" && key != "PATH_INFO") {
            ASSERT_TRUE(!env[key].empty());
        }
	}
}

MINITEST_MAIN()
