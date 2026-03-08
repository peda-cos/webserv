#include "ConfigLexer.hpp"
#include "ParsingUtils.hpp"
#include "ConfigUtils.hpp"
#include <sstream>

ConfigLexer::ConfigLexer(const std::string& source) : 
    source(source), position(0), source_length(source.length()), source_position() {}

const char* ConfigLexer::SourceInvalidSyntaxException::what() const throw() {
    static std::string message;
    std::stringstream ss;
    ss << "'ConfigLexer': Invalid syntax in configuration file at "
        << "line " << source_position.line  << ", column " << source_position.column;
    message = ss.str();
    return message.c_str();
}

void ConfigLexer::skip_comment() {
    if (position < source_length && source[position] == '#') {
        while (position < source_length && source[position] != '\n') {
            advance();
        }
    }
}

void ConfigLexer::advance() {
    if (position < source_length) {
        if (source[position] == '\n') {
            source_position.line++;
            source_position.column = 1;
        } else {
            source_position.column++;
        }
        position++;
    }
}

ConfigToken ConfigLexer::build_token() {
    std::string value;
    while (position < source_length
        && !ConfigUtils::is_delimiter(source[position])
        && !ParsingUtils::is_whitespace(source[position])) {
        if (!ConfigUtils::is_valid_char_for_config_word(source[position])) {
            std::string invalid(1, source[position]);
            throw SourceInvalidSyntaxException(source_position);
        }
        value += source[position];
        advance();
    }
    return ConfigToken(value, WORD, source_position);
}

ConfigToken ConfigLexer::get_next_token() {
    while (position < source_length && ParsingUtils::is_whitespace(source[position])) {
        advance();
    }
    if (position >= source_length) {
        return ConfigToken("", EOF_TOKEN, source_position);
    }
    if (source[position] == '#') {
        skip_comment();
        return get_next_token();
    }
    char current_char = source[position];
    
    if (ConfigUtils::is_delimiter(current_char)) {
        std::string value(1, current_char);
        ConfigTokenType type(ConfigUtils::getConfigDelimiterType(current_char));
        ConfigToken token(value, type, source_position);
        advance();
        return token;
    }

    return build_token();
}

std::vector<ConfigToken> ConfigLexer::tokenize() {
    std::vector<ConfigToken> tokens;
    while (position < source_length) {
        ConfigToken token = get_next_token();
        tokens.push_back(token);
    }
    return tokens;
}