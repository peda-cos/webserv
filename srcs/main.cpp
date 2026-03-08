
#include "ConfigLexer.hpp"
#include "ParsingUtils.hpp"
#include "ConfigUtils.hpp"
#include <iostream>

void testConfigLexer() {
    std::string configSource = "server {\n"
                               "    listen 80;\n"
                               "    server_name example.com;\n"
                               "    locatio@n / {\n"
                               "        root /var/www/html;\n"
                               "        index index.html;\n"
                               "    }\n"
                               "}\n";
    
    ConfigLexer lexer(configSource);
    std::vector<ConfigToken> tokens = lexer.tokenize();

    std::cout << "Tokens:\n";
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        const ConfigToken& token = tokens[i];
        std::cout << "Token: " << token.value 
                  << ", Type: " << token.type 
                  << ", Line: " << token.sourcePosition.line 
                  << ", Column: " << token.sourcePosition.column 
                  << std::endl;
    }
}

int main() {
    testConfigLexer();
    return 0;
}