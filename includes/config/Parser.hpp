#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <string>
#include <sstream>
#include "../../includes/config/Lexer.hpp"
#include "../../includes/config/ConfigNode.hpp"

class Parser {
public:
    explicit Parser(Lexer& lexer);
    ConfigNode* parse();
    const std::vector<std::string>& getErrors() const;

private:
    Lexer& lexer_;
    std::vector<std::string> errors_;

    void parseBlock(ConfigNode* parent);
    void reportError(const std::string& msg, size_t line);
};

#endif
