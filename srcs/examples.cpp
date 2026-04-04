
#include <ParsingUtils.hpp>
#include <ConfigParser.hpp>
#include <ConfigLexer.hpp>
#include <ConfigUtils.hpp>
#include <Logger.hpp>
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
            std::cout << "      Upload Store: " << (location.upload_store.empty() ? "(none)" : location.upload_store) << std::endl;
            std::cout << "      CGI Handlers:" << std::endl;
            if (location.cgi_handlers.empty()) {
                std::cout << "        (none)" << std::endl;
            } else {
                for (std::map<std::string, std::string>::const_iterator it = location.cgi_handlers.begin(); it != location.cgi_handlers.end(); ++it) {
                    std::cout << "        " << it->first << " -> " << it->second << std::endl;
                }
            }
            if (location.return_code != 0) {
                std::cout << "      Return: " << location.return_code << " " << location.return_url << std::endl;
            }
            std::cout << "      Error Pages (inherited from server):" << std::endl;
            if (location.error_pages.empty()) {
                std::cout << "        (none)" << std::endl;
            } else {
                for (std::map<int, std::string>::const_iterator it = location.error_pages.begin(); it != location.error_pages.end(); ++it) {
                    std::cout << "        " << it->first << " -> " << it->second << std::endl;
                }
            }
        }
    }
    std::cout << std::endl;
}

#include <CgiExecutor.hpp>
#include <CgiEnvBuilder.hpp>
#include <HttpRequest.hpp>
#include <sstream>

void testCGIExecutor() {
    HttpRequest http_request;

    http_request.setMethod("POST")
                .setUri("/cgi/script.py")
                .setHttpVersion("1.1")
                .setClientIp("127.0.0.1")
                .addHeader("Host", "localhost")
                .addHeader("User-Agent", "Mozilla/5.0")
                .addHeader("Content-Type", "application/x-www-form-urlencoded")
                .addHeader("Content-Length", "27")
                .setBody("Client data for CGI script");

    LocationConfig location_config;
    location_config.path = "/cgi/";
    location_config.root = "www";
    location_config.cgi_handlers[".py"] = "/usr/bin/python3";

    CgiExecutor executor;
    CgiResult result = executor.execute(http_request, location_config);
    std::stringstream ss;
    ss << "CGI Execution Result:" << std::endl;
    ss << "  Status: ";
    switch (result.status) {
        case CgiResult::SUCCESS: ss << "SUCCESS"; break;
        case CgiResult::TIMEOUT: ss << "TIMEOUT"; break;
        case CgiResult::EXECUTION_ERROR: ss << "EXECUTION_ERROR"; break;
        case CgiResult::NOT_FOUND: ss << "NOT_FOUND"; break;
        case CgiResult::NO_HANDLER: ss << "NO_HANDLER"; break;
    }
    ss << std::endl;
    ss << "  Output: " << std::endl << result.output << std::endl;
    Logger::info(ss.str());
}