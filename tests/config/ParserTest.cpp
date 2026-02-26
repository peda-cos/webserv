#include <iostream>
#include "../../includes/config/Parser.hpp"

int main() {
    // Test 1: Simple directive
    {
        Lexer lex("listen 80;");
        Parser p(lex);
        ConfigNode* root = p.parse();
        if (!root) {
            std::cerr << "Test 1 failed: root is NULL" << std::endl;
            return 1;
        }
        if (root->children.size() != 1) {
            std::cerr << "Test 1 failed: expected 1 child, got " << root->children.size() << std::endl;
            return 1;
        }
        if (root->children[0]->name != "listen") {
            std::cerr << "Test 1 failed: expected 'listen', got " << root->children[0]->name << std::endl;
            return 1;
        }
        if (root->children[0]->args.size() != 1 || root->children[0]->args[0] != "80") {
            std::cerr << "Test 1 failed: args incorrect" << std::endl;
            return 1;
        }
        delete root;
        std::cout << "Test 1 passed: Simple directive" << std::endl;
    }
    
    // Test 2: Block with directive
    {
        Lexer lex("server { listen 80; }");
        Parser p(lex);
        ConfigNode* root = p.parse();
        if (!root) {
            std::cerr << "Test 2 failed: root is NULL" << std::endl;
            return 1;
        }
        if (root->children.size() != 1 || root->children[0]->name != "server") {
            std::cerr << "Test 2 failed: server block" << std::endl;
            return 1;
        }
        if (root->children[0]->children.size() != 1 || root->children[0]->children[0]->name != "listen") {
            std::cerr << "Test 2 failed: nested listen" << std::endl;
            return 1;
        }
        delete root;
        std::cout << "Test 2 passed: Block with directive" << std::endl;
    }
    
    std::cout << "All parser tests passed!" << std::endl;
    return 0;
}
