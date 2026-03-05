/* ************************************************************************** */
/*  test_config_parser.cpp — TDD tests for the config parser                  */
/*                                                                            */
/*  Compile:                                                                  */
/*    c++ -std=c++98 -Wall -Wextra -Werror -I tests/framework                */
/*        tests/unit/test_config_parser.cpp -o test_config_parser             */
/* ************************************************************************** */

#include "minitest.hpp"

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstddef>

/* ===== REAL INCLUDES ==================================================== */
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

/* ===== END INCLUDES ===================================================== */

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

	std::vector<ServerConfig> result = ConfigParser::parse(conf);
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

	std::vector<ServerConfig> result = ConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(std::string("127.0.0.1"), result[0].host);
	ASSERT_EQ(8080, result[0].port);
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

	std::vector<ServerConfig> result = ConfigParser::parse(conf);
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

	std::vector<ServerConfig> result = ConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(static_cast<std::size_t>(10485760), result[0].clientMaxBodySize);
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

	std::vector<ServerConfig> result = ConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_TRUE(result[0].errorPages.find(404) != result[0].errorPages.end());
	ASSERT_EQ(std::string("/errors/404.html"), result[0].errorPages[404]);
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
		"        cgi_pass .php /usr/bin/php-cgi;\n"
		"        cgi_pass .py /usr/bin/python3;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> result = ConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(2, static_cast<int>(result[0].locations.size()));

	/* First location: /upload */
	const LocationConfig& upload = result[0].locations[0];
	ASSERT_EQ(std::string("/upload"), upload.path);
	ASSERT_EQ(std::string("/var/www/upload"), upload.root);
	ASSERT_EQ(std::string("upload.html"), upload.index);
	ASSERT_TRUE(upload.autoindex);
	ASSERT_EQ(2, static_cast<int>(upload.limitExcept.size()));
	ASSERT_EQ(std::string("POST"), upload.limitExcept[0]);
	ASSERT_EQ(std::string("DELETE"), upload.limitExcept[1]);
	ASSERT_EQ(std::string("/var/www/uploads"), upload.uploadStore);

	/* Second location: /cgi-bin */
	const LocationConfig& cgi = result[0].locations[1];
	ASSERT_EQ(std::string("/cgi-bin"), cgi.path);
	ASSERT_EQ(std::string("/var/www/cgi"), cgi.root);
	ASSERT_TRUE(cgi.cgiPass.find(".php") != cgi.cgiPass.end());
	ASSERT_EQ(std::string("/usr/bin/php-cgi"), cgi.cgiPass.find(".php")->second);
	ASSERT_TRUE(cgi.cgiPass.find(".py") != cgi.cgiPass.end());
	ASSERT_EQ(std::string("/usr/bin/python3"), cgi.cgiPass.find(".py")->second);
}

/* ------------------------------------------------------------------------ */
/* 7. A server block without the mandatory listen directive must throw.      */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, MissingMandatoryFieldThrows)
{
	std::string conf =
		"server {\n"
		"    root /var/www/html;\n"
		"}\n";

	ASSERT_THROWS(ConfigParser::parse(conf), ConfigParseException);
}

/* ------------------------------------------------------------------------ */
/* 8. A malformed config (unclosed brace) must throw.                       */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, MalformedConfigThrows)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n";

	ASSERT_THROWS(ConfigParser::parse(conf), ConfigParseException);
}

/* ------------------------------------------------------------------------ */
/* 9. An empty input string must throw.                                     */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, EmptyFileThrows)
{
	std::string conf = "";

	ASSERT_THROWS(ConfigParser::parse(conf), ConfigParseException);
}

/* ------------------------------------------------------------------------ */
/* 10. Two servers listening on the same address:port must be detected.     */
/* ------------------------------------------------------------------------ */
TEST(ConfigParser, DuplicateListenDetected)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"}\n"
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"}\n";

	ASSERT_THROWS(ConfigParser::parse(conf), ConfigParseException);
}

MINITEST_MAIN()
