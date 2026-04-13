
#include <ParsingUtils.hpp>
#include <ConfigParser.hpp>
#include <ConfigLexer.hpp>
#include <ConfigUtils.hpp>
#include <Logger.hpp>
#include <iostream>
#include <sstream>
#include <Server.hpp> 

void testCGIExecutor();

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "config/default.conf";
    try {
        std::string config_source = ConfigUtils::get_config_content(path);
        if (config_source.empty()) {
            Logger::error("Conf file is empty in path: " + path);
            return 1;
        }
        ConfigLexer lexer(config_source);
        std::vector<ConfigToken> tokens = lexer.tokenize();

        ConfigParser parser(tokens);
        Config config = parser.parse();

        if (config.servers.empty()) {
            Logger::error("No server blocks found in: " + path);
            return 1;
        }

        std::stringstream ss;
        ss << config.servers.size();
        Logger::info("Configuration loaded: " + path + " (" + ss.str() + " server(s))");

        Server server(config);
        server.run();
    } catch(const std::exception& e) {
        Logger::error(e.what());
        return 1;
    }
    return 0;
}