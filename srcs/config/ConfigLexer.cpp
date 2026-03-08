#include "ConfigLexer.hpp"
#include "ParsingUtils.hpp"
#include "ConfigUtils.hpp"

void ConfigLexer::skipComment() {
    if (position < sourceLength && source[position] == '#') {
        while (position < sourceLength && source[position] != '\n') {
            advance();
        }
    }
}

void ConfigLexer::advance() {
    if (position < sourceLength) {
        if (source[position] == '\n') {
            sourcePosition.line++;
            sourcePosition.column = 1;
        } else {
            sourcePosition.column++;
        }
        position++;
    }
}

#include <iostream>

ConfigToken ConfigLexer::buildToken() {
    std::string value;
    while (position < sourceLength
        && !ParsingUtils::isWhitespace(source[position]) 
        && !ParsingUtils::isConfigDelimiter(source[position])) {
        if (!ConfigUtils::isValidCharForConfigWord(source[position])) {
            // TODO: precisa melhorar, criar um fluxo de error para ser processado pelo parser
            std::string invalid(1, source[position]);
            advance();
            return ConfigToken(invalid, UNKNOWN, sourcePosition);
        }
        value += source[position];
        advance();
    }
    return ConfigToken(value, WORD, sourcePosition);
}

ConfigLexer::ConfigLexer(const std::string& source) : 
    source(source), position(0), sourceLength(source.length()), sourcePosition() {}

ConfigToken ConfigLexer::getNextToken() {
    while (position < sourceLength && ParsingUtils::isWhitespace(source[position])) {
        advance();
    }
    if (position >= sourceLength) {
        return ConfigToken("", EOF_TOKEN, sourcePosition);
    }
    if (source[position] == '#') {
        skipComment();
        return getNextToken();
    }
    char current_char = source[position];
    
    if (ParsingUtils::isConfigDelimiter(current_char)) {
        std::string value(1, current_char);
        ConfigTokenType type(ConfigUtils::getConfigDelimiterType(current_char));
        ConfigToken token(value, type, sourcePosition);
        advance();
        return token;
    }

    return buildToken();
}

std::vector<ConfigToken> ConfigLexer::tokenize() {
    std::vector<ConfigToken> tokens;
    while (position < sourceLength) {
        ConfigToken token = getNextToken();
        tokens.push_back(token);
    }
    return tokens;
}