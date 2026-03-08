
#include "ConfigLexer.hpp"
#include "ParsingUtils.hpp"
#include "ConfigUtils.hpp"
#include "ConfigParser.hpp"
#include "Logger.hpp"
#include <iostream>

void testConfigParser() {
    std::string config_source = "server {\n"
                                "    listen 80;\n"
                                "    client_max_body_size 1048576;\n"
                                "    error_page 404 /404.html;\n"
                                "}\n";
    ConfigLexer lexer(config_source);
    std::vector<ConfigToken> tokens = lexer.tokenize();
    ConfigParser parser(tokens);
    Config config = parser.parse();

    std::cout << "Parsed " << config.servers.size() << " server(s) from configuration." << std::endl;
    std::cout << "Conf RAW: ";
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << tokens[i].value << " ";
    }
    std::cout << std::endl;
}

int main() {
    try
    {
        testConfigParser();
    }
    catch(const std::exception& e)
    {
        Logger::error(e.what());
    }
    return 0;
}