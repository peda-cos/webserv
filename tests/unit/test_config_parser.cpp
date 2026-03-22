/* ************************************************************************** */
/*  test_config_parser.cpp — TDD tests for the config parser                  */
/*                                                                            */
/*  Compile is handled by tests/Makefile                                      */
/* ************************************************************************** */

#include "minitest.hpp"

#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <sstream>

#include <Config.hpp>
#include <ConfigLexer.hpp>
#include <ConfigParser.hpp>
#include <ConfigParseSyntaxException.hpp>
#include <ServerConfig.hpp>
#include <LocationConfig.hpp>
#include <Enums.hpp>

typedef ConfigParse::SyntaxException ConfigParseException;

namespace {
	class TestConfigParser {
		public:
			static std::vector<ServerConfig> parse(const std::string& content) {
				ConfigLexer lexer(content);
				std::vector<ConfigToken> tokens = lexer.tokenize();
				ConfigParser parser(tokens);
				Config config = parser.parse();
				return config.servers;
			}
	};

	static int parse_port_to_int(const std::string& port_as_text) {
		std::stringstream ss(port_as_text);
		int port_value = 0;
		ss >> port_value;
		return port_value;
	}
}

/* ------------------------------------------------------------------------ */
/* 1. A valid single server block should produce exactly one ServerConfig.   */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, ValidSingleServerBlock)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    root /var/www/html;\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
}

/* ------------------------------------------------------------------------ */
/* 2. The listen directive must extract the correct host and port.           */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, ListenDirectiveExtractsIpPort)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(std::string("127.0.0.1"), result[0].host);
	ASSERT_EQ(8080, parse_port_to_int(result[0].port));
}

/* ------------------------------------------------------------------------ */
/* 3. Multiple server blocks should produce the corresponding number of     */
/*    ServerConfig entries.                                                  */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, MultipleServerBlocks)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"}\n"
		"server {\n"
		"    listen 0.0.0.0:8081;\n"
		"}\n"
		"server {\n"
		"    listen 0.0.0.0:8082;\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(3, static_cast<int>(result.size()));
}

/* ------------------------------------------------------------------------ */
/* 4. client_max_body_size with unit suffix "M" must be converted to bytes. */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, ClientMaxBodySizeParsesUnits)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    client_max_body_size 10M;\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(static_cast<std::size_t>(10485760), result[0].client_max_body_size);
}

/* ------------------------------------------------------------------------ */
/* 5. error_page must map a status code to the correct file path.           */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, ErrorPageMapsCodeToPath)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    error_page 404 /errors/404.html;\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_TRUE(result[0].error_pages.find(404) != result[0].error_pages.end());
	ASSERT_EQ(std::string("/errors/404.html"), result[0].error_pages[404]);
}

/* ------------------------------------------------------------------------ */
/* 6. Location sub-directives must all be extracted correctly.              */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, LocationSubDirectives)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"\n"
		"    location /upload {\n"
		"        root /var/www/upload;\n"
		"        index upload.html;\n"
		"        autoindex on;\n"
		"        limit_except POST DELETE;\n"
		"        upload_store /var/www/uploads;\n"
		"    }\n"
		"\n"
		"    location /cgi-bin {\n"
		"        root /var/www/cgi;\n"
		"        cgi_pass .py /usr/bin/python3;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(2, static_cast<int>(result[0].locations.size()));

	/* First location: /upload */
	const LocationConfig& upload = result[0].locations[0];
	ASSERT_EQ(std::string("/upload"), upload.path);
	ASSERT_EQ(std::string("/var/www/upload"), upload.root);
	ASSERT_EQ(std::string("upload.html"), upload.index);
	ASSERT_EQ(ON, upload.autoindex);
	ASSERT_EQ(2, static_cast<int>(upload.limit_except.size()));
	ASSERT_EQ(POST, upload.limit_except[0]);
	ASSERT_EQ(DELETE, upload.limit_except[1]);
	ASSERT_EQ(std::string("/var/www/uploads"), upload.upload_store);

	/* Second location: /cgi-bin */
	const LocationConfig& cgi = result[0].locations[1];
	ASSERT_EQ(std::string("/cgi-bin"), cgi.path);
	ASSERT_EQ(std::string("/var/www/cgi"), cgi.root);
	ASSERT_EQ(1, static_cast<int>(cgi.cgi_handlers.size()));
	ASSERT_TRUE(cgi.cgi_handlers.find(".py") != cgi.cgi_handlers.end());
	ASSERT_EQ(std::string("/usr/bin/python3"), cgi.cgi_handlers.at(".py"));
}

/* ------------------------------------------------------------------------ */
/* 7. A server block without listen uses parser defaults.                    */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, MissingListenUsesDefaults)
{
	std::string conf =
		"server {\n"
		"    root /var/www/html;\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(std::string("localhost"), result[0].host);
	ASSERT_EQ(8080, parse_port_to_int(result[0].port));
}

/* ------------------------------------------------------------------------ */
/* 8. A malformed config (unclosed brace) must throw.                       */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, MalformedConfigThrows)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* ------------------------------------------------------------------------ */
/* 9. An empty input string returns zero server blocks.                      */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, EmptyFileReturnsNoServers)
{
	std::string conf = "";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(0, static_cast<int>(result.size()));
}

/* ------------------------------------------------------------------------ */
/* 10. Duplicate listen pairs are currently accepted by parser.              */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, DuplicateListenAcceptedForNow)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"}\n"
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(2, static_cast<int>(result.size()));
}

/* -------- RFC Validation Tests (newly added validations) -------------- */

/* 11. Listen directive with port > 65535 must throw (RFC: port range).     */
TEST(ConfigParser, ListenPortOutOfRangeThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:70000;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* 12. Listen directive with port < 1 must throw.                          */
TEST(ConfigParser, ListenPortZeroThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:0;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* 13. Listen with invalid IPv4 address must throw.                        */
TEST(ConfigParser, ListenInvalidIpv4Throws)
{
	std::string conf =
		"server {\n"
		"    listen 256.0.0.1:8080;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* 14. Return with status 200 (not 3xx) must throw (RFC 7231 redirect).   */
TEST(ConfigParser, ReturnWith2xxStatusThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    location /old {\n"
		"        return 200 /new;\n"
		"    }\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* 15. Return with status 400 (4xx, not 3xx) must throw.                  */
TEST(ConfigParser, ReturnWith4xxStatusThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    location /old {\n"
		"        return 403 /forbidden;\n"
		"    }\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* 16. Return with valid 301 status must parse successfully.              */
TEST(ConfigParser, ReturnWith301StatusSucceeds)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    location /old {\n"
		"        return 301 /new;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(1, static_cast<int>(result[0].locations.size()));
	ASSERT_EQ(301, result[0].locations[0].return_code);
	ASSERT_EQ(std::string("/new"), result[0].locations[0].return_url);
}

/* 17. error_page with status 200 (not 4xx/5xx) must throw (RFC on errors). */
TEST(ConfigParser, ErrorPageWith2xxStatusThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    error_page 200 /ok.html;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* 18. error_page with invalid status 900 (>599) must throw.              */
TEST(ConfigParser, ErrorPageWithStatusOutOfRangeThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    error_page 900 /error.html;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* 19. client_max_body_size with invalid suffix must throw.               */
TEST(ConfigParser, ClientMaxBodySizeInvalidSuffixThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    client_max_body_size 10T;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* 20. client_max_body_size with no digits must throw.                    */
TEST(ConfigParser, ClientMaxBodySizeNoDigitsThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    client_max_body_size M;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* ------------------------------------------------------------------------ */
/* 21. Multiple CGI handlers (.py and .php) must all be stored.             */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, MultipleCgiHandlers)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    location /cgi-bin {\n"
		"        cgi_pass .py /usr/bin/python3;\n"
		"        cgi_pass .php /usr/bin/php-cgi;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	const LocationConfig& loc = result[0].locations[0];
	ASSERT_EQ(2, static_cast<int>(loc.cgi_handlers.size()));
	ASSERT_TRUE(loc.cgi_handlers.find(".py") != loc.cgi_handlers.end());
	ASSERT_TRUE(loc.cgi_handlers.find(".php") != loc.cgi_handlers.end());
	ASSERT_EQ(std::string("/usr/bin/python3"), loc.cgi_handlers.at(".py"));
	ASSERT_EQ(std::string("/usr/bin/php-cgi"), loc.cgi_handlers.at(".php"));
}

/* ------------------------------------------------------------------------ */
/* 22. CGI extension missing dot must throw.                                */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, CgiExtensionMissingDotThrows)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    location /cgi {\n"
		"        cgi_pass py /usr/bin/python3;\n"
		"    }\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* ------------------------------------------------------------------------ */
/* 23. Duplicate CGI extension in same location must throw.                 */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, DuplicateCgiExtensionThrows)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    location /cgi {\n"
		"        cgi_pass .py /usr/bin/python3;\n"
		"        cgi_pass .py /usr/bin/python2;\n"
		"    }\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* ------------------------------------------------------------------------ */
/* 24. CGI handler path must be absolute (start with /).                    */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, CgiHandlerRelativePathThrows)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    location /cgi {\n"
		"        cgi_pass .py usr/bin/python3;\n"
		"    }\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

/* ========================================================================== */
/* RFC 7231 & NGINX Behavior Compliance Tests (25-30)                        */
/* ========================================================================== */

/* ------------------------------------------------------------------------ */
/* 25. GET in limit_except must automatically add HEAD (RFC 7231 § 4.3.2).   */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, GetImpliesHeadRfc7231)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    location / {\n"
		"        limit_except GET;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> servers = TestConfigParser::parse(conf);
	const LocationConfig& loc = servers[0].locations[0];
	
	// Both GET and HEAD must be present
	bool has_get = false, has_head = false;
	for (size_t i = 0; i < loc.limit_except.size(); ++i) {
		if (loc.limit_except[i] == GET) has_get = true;
		if (loc.limit_except[i] == HEAD) has_head = true;
	}
	ASSERT_TRUE(has_get);
	ASSERT_TRUE(has_head);
}

/* ------------------------------------------------------------------------ */
/* 26. Location inherits server error_pages when not overridden.             */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, LocationInheritsServerErrorPages)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    error_page 404 /errors/404.html;\n"
		"    error_page 500 /errors/500.html;\n"
		"    location / {\n"
		"        limit_except GET;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> servers = TestConfigParser::parse(conf);
	const LocationConfig& loc = servers[0].locations[0];
	
	ASSERT_EQ(2, static_cast<int>(loc.error_pages.size()));
	ASSERT_EQ(std::string("/errors/404.html"), loc.error_pages.at(404));
	ASSERT_EQ(std::string("/errors/500.html"), loc.error_pages.at(500));
}

/* ------------------------------------------------------------------------ */
/* 27. Location can override specific error_page from server.                */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, LocationOverridesServerErrorPage)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    error_page 404 /errors/404.html;\n"
		"    error_page 500 /errors/500.html;\n"
		"    location /api {\n"
		"        limit_except GET POST;\n"
		"        error_page 404 /api/not_found.json;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> servers = TestConfigParser::parse(conf);
	const LocationConfig& loc = servers[0].locations[0];
	
	// 404 should be overridden to /api/not_found.json
	ASSERT_EQ(std::string("/api/not_found.json"), loc.error_pages.at(404));
	// 500 should remain from server inheritance
	ASSERT_EQ(std::string("/errors/500.html"), loc.error_pages.at(500));
	ASSERT_EQ(2, static_cast<int>(loc.error_pages.size()));
}

/* ------------------------------------------------------------------------ */
/* 28. Location can add new error_page codes not in server.                 */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, LocationAddsNewErrorPageCode)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    error_page 404 /errors/404.html;\n"
		"    location /service {\n"
		"        limit_except GET;\n"
		"        error_page 503 /service/unavailable.html;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> servers = TestConfigParser::parse(conf);
	const LocationConfig& loc = servers[0].locations[0];
	
	// Should have both inherited 404 and new 503
	ASSERT_EQ(2, static_cast<int>(loc.error_pages.size()));
	ASSERT_EQ(std::string("/errors/404.html"), loc.error_pages.at(404));
	ASSERT_EQ(std::string("/service/unavailable.html"), loc.error_pages.at(503));
}

/* ------------------------------------------------------------------------ */
/* 29. Multiple locations can override different error_page codes.           */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, MultipleLocationErrorPageOverrides)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    error_page 404 /errors/404.html;\n"
		"    error_page 500 /errors/500.html;\n"
		"    location /api {\n"
		"        limit_except GET;\n"
		"        error_page 404 /api/404.json;\n"
		"    }\n"
		"    location /admin {\n"
		"        limit_except POST;\n"
		"        error_page 500 /admin/500.html;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> servers = TestConfigParser::parse(conf);
	const LocationConfig& api_loc = servers[0].locations[0];
	const LocationConfig& admin_loc = servers[0].locations[1];
	
	// /api overrides 404
	ASSERT_EQ(std::string("/api/404.json"), api_loc.error_pages.at(404));
	ASSERT_EQ(std::string("/errors/500.html"), api_loc.error_pages.at(500));
	
	// /admin overrides 500
	ASSERT_EQ(std::string("/errors/404.html"), admin_loc.error_pages.at(404));
	ASSERT_EQ(std::string("/admin/500.html"), admin_loc.error_pages.at(500));
}

/* ------------------------------------------------------------------------ */
/* 30. Location error_page with multiple codes on same directive.            */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, LocationErrorPageMultipleCodes)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"    error_page 404 500 502 503 /errors/generic.html;\n"
		"    location /api {\n"
		"        limit_except GET;\n"
		"        error_page 400 402 /api/client_error.json;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> servers = TestConfigParser::parse(conf);
	const LocationConfig& loc = servers[0].locations[0];
	
	// Inherited from server: 404, 500, 502, 503
	// Added by location: 400, 402
	ASSERT_EQ(6, static_cast<int>(loc.error_pages.size()));
	ASSERT_EQ(std::string("/api/client_error.json"), loc.error_pages.at(400));
	ASSERT_EQ(std::string("/api/client_error.json"), loc.error_pages.at(402));
	ASSERT_EQ(std::string("/errors/generic.html"), loc.error_pages.at(404));
}

MINITEST_MAIN()

