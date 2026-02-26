#include "../../includes/config/Lexer.hpp"
#include <cctype>

Lexer::Lexer(const std::string& input)
    : input_(input), pos_(0), line_(1), error_("") {}

char Lexer::currentChar() const {
    if (pos_ >= input_.length())
        return '\0';
    return input_[pos_];
}

void Lexer::advance() {
    if (pos_ < input_.length()) {
        if (input_[pos_] == '\n')
            line_++;
        pos_++;
    }
}

void Lexer::skipWhitespace() {
    while (pos_ < input_.length()) {
        char c = currentChar();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    if (currentChar() == '#') {
        while (pos_ < input_.length() && currentChar() != '\n') {
            advance();
        }
        if (currentChar() == '\n') {
            advance();
        }
        // After skipping comment, skip any following whitespace/comments
        skipWhitespace();
        if (currentChar() == '#') {
            skipComment();
        }
    }
}

bool Lexer::isValidWordChar(char c) const {
    return std::isalnum(static_cast<unsigned char>(c)) ||
           c == '_' || c == '.' || c == '-' || c == '/';
}

Token Lexer::readString() {
    std::string result;
    
    while (pos_ < input_.length() && isValidWordChar(currentChar())) {
        result += currentChar();
        advance();
    }
    
    return makeToken(TK_WORD, result);
}

Token Lexer::readQuotedString() {
    char quote = currentChar();
    advance(); // Skip opening quote
    
    std::string result;
    
    while (pos_ < input_.length()) {
        char c = currentChar();
        
        if (c == quote) {
            advance(); // Skip closing quote
            return makeToken(TK_WORD, result);
        }
        
        if (c == '\n') {
            error_ = "Unterminated quoted string";
            return makeToken(TK_ERROR, result);
        }
        
        // Full escape sequence handling
        if (c == '\\' && pos_ + 1 < input_.length()) {
            advance();
            char escaped = currentChar();
            switch (escaped) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                case '\'': result += '\''; break;
                default:
                    error_ = "Invalid escape sequence: " + std::string(1, escaped);
                    return makeToken(TK_ERROR, "");
            }
            advance();
        } else {
            result += c;
            advance();
        }
    }
    error_ = "Unterminated quoted string";
    return makeToken(TK_ERROR, result);
}

Token Lexer::makeToken(TokenType type, const std::string& content) {
    return Token(type, content, line_);
}

Token Lexer::nextToken() {
    skipWhitespace();
    skipComment();

    if (pos_ >= input_.length())
        return makeToken(TK_EOF, "");

    char c = currentChar();

    switch (c) {
        case '{':
            advance();
            return makeToken(TK_CURLY_OPEN, "{");
        case '}':
            advance();
            return makeToken(TK_CURLY_CLOSE, "}");
        case ';':
            advance();
            return makeToken(TK_SEMICOLON, ";");
        case '"':
        case '\'':
            return readQuotedString();
        default:
            if (isValidWordChar(c))
                return readString();
            else {
                error_ = "Unexpected character: " + std::string(1, c);
                advance();
                return makeToken(TK_ERROR, std::string(1, c));
            }
    }
}

bool Lexer::hasError() const {
    return !error_.empty();
}

std::string Lexer::getError() const {
    return error_;
}

size_t Lexer::getCurrentLine() const {
    return line_;
}
