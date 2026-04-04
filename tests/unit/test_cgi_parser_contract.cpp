#include "minitest.hpp"

#include <CgiExecutor.hpp>
#include <CgiException.hpp>
#include <HttpRequest.hpp>
#include <LocationConfig.hpp>

#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

namespace {
	class ScriptFactory {
		public:
			static bool pythonAvailable() {
				return system("which python3 > /dev/null 2>&1") == 0;
			}

			static std::string createScript(const std::string& name, const std::string& code) {
				std::string path = "/tmp/" + name;
				std::ofstream file(path.c_str());
				if (!file)
					return "";
				file << "#!/usr/bin/python3\n" << code;
				file.close();
				chmod(path.c_str(), 0755);
				return path;
			}

			static void cleanup(const std::string& path) {
				std::remove(path.c_str());
			}
	};
}

TEST(CgiParserContract, PathResolutionIgnoresQueryString)
{
	if (!ScriptFactory::pythonAvailable()) {
		ASSERT_TRUE(true);
		return;
	}

	std::string script = ScriptFactory::createScript("query_contract.py",
		"import sys\nsys.stdout.write('OK:QUERY')\n");
	if (script.empty()) {
		ASSERT_TRUE(false);
		return;
	}

	HttpRequest req;
	req.setMethod(GET)
	   .setUri("/query_contract.py?foo=bar")
	   .setPath("/query_contract.py")
	   .setQueryString("foo=bar")
	   .setVersion("HTTP/1.1");

	LocationConfig config;
	config.root = "/tmp";
	config.cgi_handlers[".py"] = "/usr/bin/python3";

	CgiExecutor executor;
	CgiResult result = executor.execute(req, config);
	ASSERT_TRUE(result.output.find("OK:QUERY") != std::string::npos);

	ScriptFactory::cleanup(script);
}

TEST(CgiParserContract, MissingExtensionThrowsCgiException)
{
	HttpRequest req;
	req.setMethod(GET)
	   .setUriPath("/script_without_extension")
	   .setVersion("HTTP/1.1");

	LocationConfig config;
	config.root = "/tmp";
	config.cgi_handlers[".py"] = "/usr/bin/python3";

	CgiExecutor executor;
	ASSERT_THROWS(executor.execute(req, config), CgiException);
}

MINITEST_MAIN()
