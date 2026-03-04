/* ************************************************************************** */
/*  test_cgi_env.cpp — TDD tests for CGI environment variable builder         */
/*                                                                            */
/*  Compile:                                                                  */
/*    c++ -std=c++98 -Wall -Wextra -Werror -I tests/framework                */
/*        tests/unit/test_cgi_env.cpp -o test_cgi_env                         */
/* ************************************************************************** */

#include "minitest.hpp"

#include <string>
#include <map>

/* ===== STUBS — replace with real includes once implemented =============== */
/* These stubs define the expected API surface for CGI env building.          */
/* They will fail tests deterministically until real implementation exists.   */

struct HttpRequest {
	std::string                        method;
	std::string                        uri;
	std::string                        path;
	std::string                        queryString;
	std::string                        httpVersion;
	std::map<std::string, std::string> headers;
	std::string                        body;
};

class CgiEnv {
public:
	static std::map<std::string, std::string> build(
			const HttpRequest& req, const std::string& scriptPath) {
		(void)req;
		(void)scriptPath;
		/* Stub: always returns empty map so every test fails. */
		return std::map<std::string, std::string>();
	}

private:
	CgiEnv();
};

/* ===== END STUBS ======================================================== */

/* ------------------------------------------------------------------------ */
/* 1. A GET request must set REQUEST_METHOD to "GET".                        */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, RequestMethodGet)
{
	HttpRequest req;
	req.method      = "GET";
	req.uri         = "/cgi-bin/test.py";
	req.path        = "/cgi-bin/test.py";
	req.queryString = "";
	req.httpVersion = "HTTP/1.1";
	req.body        = "";

	std::map<std::string, std::string> env = CgiEnv::build(req, "/var/www/cgi/test.py");
	ASSERT_EQ(std::string("GET"), env["REQUEST_METHOD"]);
}

/* ------------------------------------------------------------------------ */
/* 2. A POST request must set REQUEST_METHOD to "POST".                     */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, RequestMethodPost)
{
	HttpRequest req;
	req.method      = "POST";
	req.uri         = "/cgi-bin/submit.py";
	req.path        = "/cgi-bin/submit.py";
	req.queryString = "";
	req.httpVersion = "HTTP/1.1";
	req.body        = "data=hello";

	std::map<std::string, std::string> env = CgiEnv::build(req, "/var/www/cgi/submit.py");
	ASSERT_EQ(std::string("POST"), env["REQUEST_METHOD"]);
}

/* ------------------------------------------------------------------------ */
/* 3. Content-Type header must be forwarded to CONTENT_TYPE env var.         */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, ContentTypeFromHeaders)
{
	HttpRequest req;
	req.method      = "POST";
	req.uri         = "/cgi-bin/api.py";
	req.path        = "/cgi-bin/api.py";
	req.queryString = "";
	req.httpVersion = "HTTP/1.1";
	req.body        = "{\"key\":\"value\"}";
	req.headers["Content-Type"] = "application/json";

	std::map<std::string, std::string> env = CgiEnv::build(req, "/var/www/cgi/api.py");
	ASSERT_EQ(std::string("application/json"), env["CONTENT_TYPE"]);
}

/* ------------------------------------------------------------------------ */
/* 4. Content-Length header must be forwarded to CONTENT_LENGTH env var.     */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, ContentLengthFromHeaders)
{
	HttpRequest req;
	req.method      = "POST";
	req.uri         = "/cgi-bin/upload.py";
	req.path        = "/cgi-bin/upload.py";
	req.queryString = "";
	req.httpVersion = "HTTP/1.1";
	req.body        = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	req.headers["Content-Length"] = "42";

	std::map<std::string, std::string> env = CgiEnv::build(req, "/var/www/cgi/upload.py");
	ASSERT_EQ(std::string("42"), env["CONTENT_LENGTH"]);
}

/* ------------------------------------------------------------------------ */
/* 5. PATH_INFO must be extracted from the URI after the script name.        */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, PathInfoFromUri)
{
	HttpRequest req;
	req.method      = "GET";
	req.uri         = "/cgi-bin/script.py/extra/path";
	req.path        = "/cgi-bin/script.py/extra/path";
	req.queryString = "";
	req.httpVersion = "HTTP/1.1";
	req.body        = "";

	std::map<std::string, std::string> env = CgiEnv::build(req, "/var/www/cgi/script.py");
	ASSERT_EQ(std::string("/extra/path"), env["PATH_INFO"]);
}

/* ------------------------------------------------------------------------ */
/* 6. QUERY_STRING must be extracted from the URI after '?'.                 */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, QueryStringFromUri)
{
	HttpRequest req;
	req.method      = "GET";
	req.uri         = "/cgi-bin/search.py?key=value";
	req.path        = "/cgi-bin/search.py";
	req.queryString = "key=value";
	req.httpVersion = "HTTP/1.1";
	req.body        = "";

	std::map<std::string, std::string> env = CgiEnv::build(req, "/var/www/cgi/search.py");
	ASSERT_EQ(std::string("key=value"), env["QUERY_STRING"]);
}

/* ------------------------------------------------------------------------ */
/* 7. SERVER_PROTOCOL must reflect the HTTP version of the request.          */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, ServerProtocol)
{
	HttpRequest req;
	req.method      = "GET";
	req.uri         = "/cgi-bin/info.py";
	req.path        = "/cgi-bin/info.py";
	req.queryString = "";
	req.httpVersion = "HTTP/1.1";
	req.body        = "";

	std::map<std::string, std::string> env = CgiEnv::build(req, "/var/www/cgi/info.py");
	ASSERT_EQ(std::string("HTTP/1.1"), env["SERVER_PROTOCOL"]);
}

/* ------------------------------------------------------------------------ */
/* 8. All seven required CGI environment variables must be present and       */
/*    non-empty in the returned map.                                         */
/* ------------------------------------------------------------------------ */
TEST(CgiEnv, AllSevenVarsPresent)
{
	HttpRequest req;
	req.method      = "POST";
	req.uri         = "/cgi-bin/full.py/pathinfo?q=1";
	req.path        = "/cgi-bin/full.py/pathinfo";
	req.queryString = "q=1";
	req.httpVersion = "HTTP/1.1";
	req.body        = "payload";
	req.headers["Content-Type"]   = "text/plain";
	req.headers["Content-Length"] = "7";

	std::map<std::string, std::string> env = CgiEnv::build(req, "/var/www/cgi/full.py");

	const char* required[] = {
		"REQUEST_METHOD",
		"CONTENT_TYPE",
		"CONTENT_LENGTH",
		"PATH_INFO",
		"QUERY_STRING",
		"SERVER_PROTOCOL",
		"SCRIPT_FILENAME"
	};

	for (int i = 0; i < 7; ++i) {
		std::string key(required[i]);
		ASSERT_TRUE(env.find(key) != env.end());
		ASSERT_TRUE(!env[key].empty());
	}
}

MINITEST_MAIN()
