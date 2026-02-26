#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

enum TokenType {
    TK_WORD,         // Diretivas ou argumentos
    TK_SEMICOLON,    // ;
    TK_CURLY_OPEN,   // {
    TK_CURLY_CLOSE,  // }
    TK_EOF,          // Fim de arquivo
    TK_ERROR         // Token de erro
};

struct Token {
    TokenType type;
    std::string content;
    size_t line;

    Token(TokenType t = TK_ERROR, const std::string& c = "", size_t l = 0)
        : type(t), content(c), line(l) {}

    bool isError() const { return type == TK_ERROR; }
};

std::string tokenTypeToString(TokenType type);

#endif
