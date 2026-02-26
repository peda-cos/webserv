#include "../../includes/config/ConfigLoader.hpp"
#include "../../includes/config/Lexer.hpp"
#include "../../includes/config/Parser.hpp"
#include "../../includes/config/SemanticValidator.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

bool ConfigLoader::load(const std::string& filepath,
                        std::vector<ServerConfig>& configs,
                        std::string& error) {
    // 1. Read file
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        error = "Cannot open file: " + filepath;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    if (content.empty()) {
        error = "Empty config file: " + filepath;
        return false;
    }
    
    // 2. Lexer
    Lexer lexer(content);
    
    // 3. Parser
    Parser parser(lexer);
    ConfigNode* root = parser.parse();
    if (!root) {
        const std::vector<std::string>& parseErrors = parser.getErrors();
        if (!parseErrors.empty()) {
            error = parseErrors[0];
        } else {
            error = "Parse error";
        }
        return false;
    }
    
    // 4. Validator
    SemanticValidator validator;
    std::vector<ValidationError> valErrors;
    
    if (!validator.validate(root, configs, valErrors)) {
        std::stringstream ss;
        for (size_t i = 0; i < valErrors.size(); ++i) {
            ss << valErrors[i].toString() << "\n";
        }
        error = ss.str();
        delete root;
        return false;
    }
    
    delete root;
    return true;
}
