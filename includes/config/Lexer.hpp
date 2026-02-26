#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include "Token.hpp"

class Lexer {
public:
    explicit Lexer(const std::string& input);
    Token nextToken();
    bool hasError() const;
    std::string getError() const;
    size_t getCurrentLine() const;

private:
    std::string input_;
    size_t pos_;
    size_t line_;
    std::string error_;

    char currentChar() const;
    void advance();
    void skipWhitespace();
    void skipComment();
    Token readString();
    Token readQuotedString();
    Token makeToken(TokenType type, const std::string& content);
    bool isValidWordChar(char c) const;
};

#endif
