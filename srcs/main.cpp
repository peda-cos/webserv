
#include <ParsingUtils.hpp>
#include <ConfigParser.hpp>
#include <ConfigLexer.hpp>
#include <ConfigUtils.hpp>
#include <Logger.hpp>
#include <iostream>

// Mínimo Funcional
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

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "config/default.conf";
    try {
        std::string config_source = ConfigUtils::get_config_content(path);
        if (config_source.empty()) {
            Logger::error("Conf file is empty in path: " + path);
            return 1;
        }
        testConfigParser(config_source);
    } catch(const std::exception& e) {
        Logger::error(e.what());
        return 1;
    }
    return 0;
}