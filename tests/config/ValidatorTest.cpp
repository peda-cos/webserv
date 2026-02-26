#include <iostream>
#include "../../includes/config/SemanticValidator.hpp"
#include "../../includes/config/Parser.hpp"
#include "../../includes/config/Lexer.hpp"

int main() {
    // Test 1: Valid config
    {
        Lexer lex("server { listen 80; root /var/www; }");
        Parser p(lex);
        ConfigNode* root = p.parse();
        if (!root) {
            std::cerr << "Test 1: Parse failed" << std::endl;
            return 1;
        }
        
        SemanticValidator validator;
        std::vector<ServerConfig> configs;
        std::vector<ValidationError> errors;
        
        bool result = validator.validate(root, configs, errors);
        delete root;
        
        if (!result) {
            std::cerr << "Test 1: Validation should pass" << std::endl;
            return 1;
        }
        if (configs.size() != 1) {
            std::cerr << "Test 1: Expected 1 server config" << std::endl;
            return 1;
        }
        if (configs[0].listens.empty() || configs[0].listens[0].port != 80) {
            std::cerr << "Test 1: Port should be 80" << std::endl;
            return 1;
        }
        if (configs[0].root != "/var/www") {
            std::cerr << "Test 1: Root should be /var/www" << std::endl;
            return 1;
        }
        std::cout << "Test 1 passed: Valid config" << std::endl;
    }
    
    // Test 2: Invalid port
    {
        Lexer lex("server { listen 99999; }");
        Parser p(lex);
        ConfigNode* root = p.parse();
        if (!root) {
            std::cerr << "Test 2: Parse failed" << std::endl;
            return 1;
        }
        
        SemanticValidator validator;
        std::vector<ServerConfig> configs;
        std::vector<ValidationError> errors;
        
        bool result = validator.validate(root, configs, errors);
        delete root;
        
        if (result) {
            std::cerr << "Test 2: Should fail with invalid port" << std::endl;
            return 1;
        }
        if (errors.empty()) {
            std::cerr << "Test 2: Should have errors" << std::endl;
            return 1;
        }
        std::cout << "Test 2 passed: Invalid port detected" << std::endl;
    }
    
    // Test 3: Inheritance
    {
        Lexer lex("server { listen 80; client_max_body_size 10M; location / { root /var; } }");
        Parser p(lex);
        ConfigNode* root = p.parse();
        if (!root) {
            std::cerr << "Test 3: Parse failed" << std::endl;
            return 1;
        }
        
        SemanticValidator validator;
        std::vector<ServerConfig> configs;
        std::vector<ValidationError> errors;
        
        bool result = validator.validate(root, configs, errors);
        delete root;
        
        if (!result) {
            std::cerr << "Test 3: Validation should pass" << std::endl;
            return 1;
        }
        if (configs[0].client_max_body_size != 10 * 1024 * 1024) {
            std::cerr << "Test 3: Server size should be 10M" << std::endl;
            return 1;
        }
        if (configs[0].locations.empty()) {
            std::cerr << "Test 3: Should have location" << std::endl;
            return 1;
        }
        if (configs[0].locations[0].client_max_body_size != 10 * 1024 * 1024) {
            std::cerr << "Test 3: Location should inherit 10M" << std::endl;
            return 1;
        }
        std::cout << "Test 3 passed: Inheritance works" << std::endl;
    }
    
    std::cout << "All validator tests passed!" << std::endl;
    return 0;
}
