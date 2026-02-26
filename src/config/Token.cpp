#include "../../includes/config/Token.hpp"

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TK_WORD: return "WORD";
        case TK_SEMICOLON: return "SEMICOLON";
        case TK_CURLY_OPEN: return "CURLY_OPEN";
        case TK_CURLY_CLOSE: return "CURLY_CLOSE";
        case TK_EOF: return "EOF";
        case TK_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
