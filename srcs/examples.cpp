#include "ConfigParser.hpp"
#include "ConfigLexer.hpp"
#include "ConfigUtils.hpp"
#include "Logger.hpp"
#include <iostream>

void testConfigParser(std::string config_source) {
    ConfigLexer lexer(config_source);
    std::vector<ConfigToken> tokens = lexer.tokenize();
    ConfigParser parser(tokens);
    Config config = parser.parse();

    std::cout << "Lexer Raw Tokens:" << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const ConfigToken& token = tokens[i];
        std::cout << token.value << " ";
    }
    std::cout << std::endl << std::endl;

    std::cout << "Parsed " << config.servers.size() << " server(s) from configuration." << std::endl;
    std::cout << "Config properties:" << std::endl;
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig& server = config.servers[i];
        std::cout << std::endl << "Server " << i + 1 << ":" << std::endl;
        std::cout << "  Host: " << (server.host.empty() ? "default" : server.host) << std::endl;
        std::cout << "  Port: " << (server.port.empty() ? "default" : server.port) << std::endl;
        std::cout << "  Client Max Body Size: " << server.client_max_body_size << " bytes" << std::endl;
        std::cout << "  Error Pages:" << std::endl;
        for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); it != server.error_pages.end(); ++it) {
            std::cout << "    " << it->first << " -> " << it->second << std::endl;
        }
        std::cout << "  Server Names: ";
        for (size_t j = 0; j < server.server_names.size(); ++j) {
            std::cout << server.server_names[j] << (j < server.server_names.size() - 1 ? ", " : "");
        }
        std::cout << std::endl;
        std::cout << "  Locations: " << server.locations.size() << std::endl;
        for (size_t j = 0; j < server.locations.size(); ++j) {
            const LocationConfig& location = server.locations[j];
            std::cout << "    Location " << j + 1 << ":" << std::endl;
            std::cout << "      Path: " << location.path << std::endl;
            std::cout << "      Root: " << location.root << std::endl;
            std::cout << "      Index: " << location.index << std::endl;
            std::cout << "      AutoIndex: " << (location.autoindex == ON ? "ON" : "OFF") << std::endl;
            std::cout << "      Limit Except: ";
            for (size_t k = 0; k < location.limit_except.size(); ++k) {
                std::cout << location.limit_except[k] << (k < location.limit_except.size() - 1 ? ", " : "");
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

#include <CgiExecutor.hpp>
#include <CgiEnvBuilder.hpp>
#include <HttpRequest.hpp>

void testCGIExecutor() {
    HttpRequest http_request;

    http_request.setMethod(POST)
                .setUriPath("/cgi-bin/script.cgi")
                .setVersion("HTTP/1.1")
                .addHeader("Host", "localhost")
                .addHeader("User-Agent", "Mozilla/5.0")
                .addHeader("Content-Type", "application/x-www-form-urlencoded")
                .addHeader("Content-Length", "27")
                .setBody("name=John&age=30");

    LocationConfig location_config;
    location_config.path = "/cgi-bin/";
    location_config.cgi_pass = "/usr/lib/cgi-bin/script.cgi";

    CgiEnvBuilder env_builder(http_request, location_config);
    char** envp = env_builder.getEnvp();

    std::cout << "CGI Environment Variables:" << std::endl << std::endl;
    for (size_t i = 0; envp[i] != NULL; ++i) {
        std::cout << envp[i] << std::endl;
    }
}