#include "minitest.hpp"

#include <CgiExecutor.hpp>
#include <CgiException.hpp>
#include <HttpRequest.hpp>
#include <LocationConfig.hpp>
#include <ServerConfig.hpp>

#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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
		ServerConfig srv;
	CgiProcessInfo info = executor.start_cgi(req, config, srv, -1);
	
	std::string output;
	char buffer[4096];
	size_t body_written = 0;
	bool stdin_closed = false;
	bool stdout_done = false;
	
	while (!stdout_done) {
		if (!stdin_closed) {
			if (body_written < req.body.length()) {
				ssize_t wn = write(info.stdin_fd, req.body.c_str() + body_written, req.body.length() - body_written);
				if (wn > 0) body_written += wn;
				else if (wn == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
					close(info.stdin_fd);
					stdin_closed = true;
				}
			} else {
				close(info.stdin_fd);
				stdin_closed = true;
			}
		}

		ssize_t rn = read(info.stdout_fd, buffer, sizeof(buffer));
		if (rn > 0) {
			output.append(buffer, rn);
		} else if (rn == 0) {
			stdout_done = true;
		} else if (rn == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
			stdout_done = true;
		}
		
		if (!stdout_done) usleep(100);
	}
	close(info.stdout_fd);
	if (!stdin_closed) close(info.stdin_fd);
	waitpid(info.pid, NULL, 0);

	ASSERT_TRUE(output.find("OK:QUERY") != std::string::npos);

	ScriptFactory::cleanup(script);
}

TEST(CgiParserContract, MissingExtensionThrowsCgiException)
{
	HttpRequest req;
	req.setMethod(GET)
	   .setUri("/script_without_extension")
	   .setPath("/script_without_extension")
	   .setVersion("HTTP/1.1");

	LocationConfig config;
	config.root = "/tmp";
	config.cgi_handlers[".py"] = "/usr/bin/python3";

	CgiExecutor executor;
		ServerConfig srv;
	ASSERT_THROWS(executor.start_cgi(req, config, srv, -1), CgiException);
}

MINITEST_MAIN()
