#include <sstream>
#include <ConfigUtils.hpp>
#include <ConfigLexer.hpp>
#include <ParsingUtils.hpp>
#include <ConfigParseSyntaxException.hpp>

ConfigLexer::ConfigLexer(const std::string& source) : 
    source(source), position(0), source_length(source.length()), source_position() {}

void ConfigLexer::throw_unexpected_char_error(char invalid_char) {
    std::stringstream ss;
    ss << "Unexpected character '" << invalid_char << "'";
    throw ConfigParse::SyntaxException(ss.str(), ConfigParse::CONFIG_LEXER, source_position);
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
            throw_unexpected_char_error(source[position]);
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
        if (token.type == EOF_TOKEN)
            break;
        tokens.push_back(token);
    }
    if (position >= source_length) {
        ConfigToken eof_token("", EOF_TOKEN, source_position);
        tokens.push_back(eof_token);
    }
    return tokens;
}