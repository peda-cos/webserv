#include <iostream>
#include <fstream>
#include "../../includes/config/ConfigLoader.hpp"

int main() {
    int passed = 0;
    int failed = 0;
    
    // Test 1: Load valid config
    {
        std::vector<ServerConfig> configs;
        std::string error;
        bool result = ConfigLoader::load("tests/fixtures/config/valid.conf", configs, error);
        if (result && configs.size() == 1) {
            std::cout << "Test 1 PASS: Valid config loaded" << std::endl;
            passed++;
        } else {
            std::cout << "Test 1 FAIL: " << error << std::endl;
            failed++;
        }
    }
    
    // Test 2: Syntax error detection
    {
        std::vector<ServerConfig> configs;
        std::string error;
        bool result = ConfigLoader::load("tests/fixtures/config/syntax_error.conf", configs, error);
        if (!result) {
            std::cout << "Test 2 PASS: Syntax error detected" << std::endl;
            passed++;
        } else {
            std::cout << "Test 2 FAIL: Should have failed" << std::endl;
            failed++;
        }
    }
    
    // Test 3: Semantic error detection
    {
        std::vector<ServerConfig> configs;
        std::string error;
        bool result = ConfigLoader::load("tests/fixtures/config/semantic_error.conf", configs, error);
        if (!result && error.find("port") != std::string::npos) {
            std::cout << "Test 3 PASS: Invalid port detected" << std::endl;
            passed++;
        } else {
            std::cout << "Test 3 FAIL: " << error << std::endl;
            failed++;
        }
    }
    
    // Test 4: Multiple servers
    {
        std::vector<ServerConfig> configs;
        std::string error;
        bool result = ConfigLoader::load("tests/fixtures/config/multi_server.conf", configs, error);
        if (result && configs.size() == 2) {
            std::cout << "Test 4 PASS: Multiple servers loaded" << std::endl;
            passed++;
        } else {
            std::cout << "Test 4 FAIL: Expected 2 servers, got " << configs.size() << std::endl;
            failed++;
        }
    }
    
    // Test 5: Non-existent file
    {
        std::vector<ServerConfig> configs;
        std::string error;
        bool result = ConfigLoader::load("tests/fixtures/config/nonexistent.conf", configs, error);
        if (!result) {
            std::cout << "Test 5 PASS: Non-existent file handled" << std::endl;
            passed++;
        } else {
            std::cout << "Test 5 FAIL: Should have failed" << std::endl;
            failed++;
        }
    }
    
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
