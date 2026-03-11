#include <sstream>
#include <Enums.hpp>
#include <Logger.hpp>
#include <ConfigParser.hpp>
#include <ParsingUtils.hpp>
#include <ConfigParserServer.hpp>
#include <ConfigParseSyntaxException.hpp>

ConfigParser::ConfigParser(std::vector<ConfigToken> &config_tokens) :
    tokens(config_tokens), current_token(tokens[0]), current_token_index(0), total_tokens(config_tokens.size()) {}

void ConfigParser::throw_unexpected_token_error(const std::string& message) {
    throw ConfigParse::SyntaxException(message,
        ConfigParse::CONFIG_PARSER, current_token.source_position);   
}

void ConfigParser::log_token_alert(const std::string& block_name) {
    std::stringstream ss;
    ss << "Unknown directive '" << current_token.value << "' in " << block_name 
       << " block at line " << current_token.source_position.line 
       << ", column " << current_token.source_position.column;
    Logger::warn(ss.str());
}

void ConfigParser::advance() {
    if (current_token_index < total_tokens - 1) {
        current_token_index++;
        current_token = tokens[current_token_index];
    }
}

void ConfigParser::validates_char_block_at(const std::string& block_name) {
    if (current_token.type != LEFT_BRACE && current_token.type != RIGHT_BRACE) {
        throw_unexpected_token_error(
            "Expected '{' to start '" + block_name 
            + "' block, but got: " + current_token.value);
    }
    advance();
}

void ConfigParser::validates_directive_value_for(const std::string& directive_name) {
    advance();
    if (current_token.type != WORD) {
        throw_unexpected_token_error(
            "Expected a value for '" + directive_name 
            + "' directive, but got: " + current_token.value);
    }
}

void ConfigParser::validates_extra_arguments_in(const std::string& directive_name) {
    advance();
    if (current_token.type != SEMICOLON) {
        throw_unexpected_token_error(
            "Unexpected token after '" + directive_name 
            + "' directive value: " + current_token.value);
    }
}

Config ConfigParser::parse() {
    while (current_token.type != EOF_TOKEN) {
        current_token.directive_type  = ParsingUtils::get_root_directive_type(current_token.value);
        switch (current_token.directive_type) {
            case ROOT_SERVER: {
                ConfigParserServer parser_server(*this);
                ServerConfig server_config = parser_server.parse();
                config.servers.push_back(server_config);
                continue;
            }
            default: {
                std::string detail_message("Unexpected token at root level: " + current_token.value);
                throw_unexpected_token_error(detail_message);
            }
        }
        advance();
    }

    return config;
}