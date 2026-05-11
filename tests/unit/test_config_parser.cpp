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

TEST(ConfigParser, ValidSingleServerBlock)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    root www;\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
}

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

TEST(ConfigParser, LocationSubDirectives)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n"
		"\n"
		"    location /upload {\n"
		"        root www/upload;\n"
		"        index upload.html;\n"
		"        autoindex on;\n"
		"        limit_except POST DELETE;\n"
		"        upload_store www/uploads;\n"
		"    }\n"
		"\n"
		"    location /cgi-bin {\n"
		"        root www/cgi;\n"
		"        cgi_pass .py /usr/bin/python3;\n"
		"    }\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(2, static_cast<int>(result[0].locations.size()));

	const LocationConfig& upload = result[0].locations[0];
	ASSERT_EQ(std::string("/upload"), upload.path);
	ASSERT_EQ(std::string("www/upload"), upload.root);
	ASSERT_EQ(std::string("upload.html"), upload.index);
	ASSERT_EQ(ON, upload.autoindex);
	ASSERT_EQ(2, static_cast<int>(upload.limit_except.size()));
	ASSERT_EQ(POST, upload.limit_except[0]);
	ASSERT_EQ(DELETE, upload.limit_except[1]);
	ASSERT_EQ(std::string("www/uploads"), upload.upload_store);

	const LocationConfig& cgi = result[0].locations[1];
	ASSERT_EQ(std::string("/cgi-bin"), cgi.path);
	ASSERT_EQ(std::string("www/cgi"), cgi.root);
	ASSERT_EQ(1, static_cast<int>(cgi.cgi_handlers.size()));
	ASSERT_TRUE(cgi.cgi_handlers.find(".py") != cgi.cgi_handlers.end());
	ASSERT_EQ(std::string("/usr/bin/python3"), cgi.cgi_handlers.at(".py"));
}

TEST(ConfigParser, MissingListenUsesDefaults)
{
	std::string conf =
		"server {\n"
		"    root www;\n"
		"}\n";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(1, static_cast<int>(result.size()));
	ASSERT_EQ(std::string("localhost"), result[0].host);
	ASSERT_EQ(8080, parse_port_to_int(result[0].port));
}

TEST(ConfigParser, MalformedConfigThrows)
{
	std::string conf =
		"server {\n"
		"    listen 0.0.0.0:8080;\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

TEST(ConfigParser, EmptyFileReturnsNoServers)
{
	std::string conf = "";

	std::vector<ServerConfig> result = TestConfigParser::parse(conf);
	ASSERT_EQ(0, static_cast<int>(result.size()));
}

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

TEST(ConfigParser, ListenPortOutOfRangeThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:70000;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

TEST(ConfigParser, ListenPortZeroThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:0;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

TEST(ConfigParser, ListenInvalidIpv4Throws)
{
	std::string conf =
		"server {\n"
		"    listen 256.0.0.1:8080;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

// RFC 7231: return directive requires 3xx status
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

// RFC: error_page requires 4xx/5xx status
TEST(ConfigParser, ErrorPageWith2xxStatusThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    error_page 200 /ok.html;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

TEST(ConfigParser, ErrorPageWithStatusOutOfRangeThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    error_page 900 /error.html;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

TEST(ConfigParser, ClientMaxBodySizeInvalidSuffixThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    client_max_body_size 10T;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

TEST(ConfigParser, ClientMaxBodySizeNoDigitsThrows)
{
	std::string conf =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    client_max_body_size M;\n"
		"}\n";

	ASSERT_THROWS(TestConfigParser::parse(conf), ConfigParseException);
}

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

// RFC 7231 § 4.3.2: GET implies HEAD
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

	bool has_get = false, has_head = false;
	for (size_t i = 0; i < loc.limit_except.size(); ++i) {
		if (loc.limit_except[i] == GET) has_get = true;
		if (loc.limit_except[i] == HEAD) has_head = true;
	}
	ASSERT_TRUE(has_get);
	ASSERT_TRUE(has_head);
}

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

	ASSERT_EQ(std::string("/api/not_found.json"), loc.error_pages.at(404));
	ASSERT_EQ(std::string("/errors/500.html"), loc.error_pages.at(500));
	ASSERT_EQ(2, static_cast<int>(loc.error_pages.size()));
}

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

	ASSERT_EQ(2, static_cast<int>(loc.error_pages.size()));
	ASSERT_EQ(std::string("/errors/404.html"), loc.error_pages.at(404));
	ASSERT_EQ(std::string("/service/unavailable.html"), loc.error_pages.at(503));
}

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

	ASSERT_EQ(std::string("/api/404.json"), api_loc.error_pages.at(404));
	ASSERT_EQ(std::string("/errors/500.html"), api_loc.error_pages.at(500));

	ASSERT_EQ(std::string("/errors/404.html"), admin_loc.error_pages.at(404));
	ASSERT_EQ(std::string("/admin/500.html"), admin_loc.error_pages.at(500));
}

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

	ASSERT_EQ(6, static_cast<int>(loc.error_pages.size()));
	ASSERT_EQ(std::string("/api/client_error.json"), loc.error_pages.at(400));
	ASSERT_EQ(std::string("/api/client_error.json"), loc.error_pages.at(402));
	ASSERT_EQ(std::string("/errors/generic.html"), loc.error_pages.at(404));
}

MINITEST_MAIN()