
#include <sstream>
#include <Logger.hpp>
#include <ParsingUtils.hpp>
#include <ParserDirectiveUtils.hpp>
#include <ConfigParserLocation.hpp>
#include <ConfigParseSyntaxException.hpp>

ConfigParserLocation::ConfigParserLocation(ConfigParser &parser) : parser(parser) {
    location_config.path = "/";
    location_config.root = "/var/www/html";
    location_config.index = "";
    location_config.autoindex = OFF;
    location_config.limit_except = std::vector<HttpMethod>();
    location_config.upload_store = "";
    location_config.cgi_pass = "";
    location_config.return_url = "";
    location_config.return_code = 0;
}

void ConfigParserLocation::throw_unexpected_token_error(const std::string& message) {
    throw ConfigParse::SyntaxException(message,
        ConfigParse::CONFIG_PARSER_LOCATION, parser.current_token.source_position);
}

void ConfigParserLocation::parse_location_value() {
    std::vector<std::string> parts = std::vector<std::string>();
    while (parser.current_token.type == WORD) {
        parts.push_back(parser.current_token.value);
        parser.advance();
    }
    int parts_size = static_cast<int>(parts.size());
    if (parts_size > 2)
        throw_unexpected_token_error("Too many arguments for 'location' directive: " + parser.current_token.value);
    if (parts_size == 0)
        throw_unexpected_token_error("location directive requires a path");
    location_config.path = parts[parts_size - 1];
    if (location_config.path[0] != '/' && location_config.path[0] != '@')
        throw_unexpected_token_error("location path must start with '/' or '@': " + location_config.path);
    if (parts_size > 1) {
        location_config.modifier = parts[0];
        if (location_config.modifier != "=" 
            && location_config.modifier != "~" 
            && location_config.modifier != "~*")
            throw_unexpected_token_error("Invalid location modifier: " + location_config.modifier);
    }
}

void ConfigParserLocation::parse_location_index() {
    parser.validates_directive_value_for("index");
    location_config.index = parser.current_token.value;
    parser.validates_extra_arguments_in("index");
}

void ConfigParserLocation::parse_location_autoindex() {
    parser.validates_directive_value_for("autoindex");
    if (parser.current_token.value == "on") {
        location_config.autoindex = ON;
    } else if (parser.current_token.value == "off") {
        location_config.autoindex = OFF;
    } else {
        throw_unexpected_token_error("Invalid value for 'autoindex' directive: " + parser.current_token.value);
    }
    parser.validates_extra_arguments_in("autoindex");
}

void ConfigParserLocation::parse_location_methods() {
    parser.validates_directive_value_for("limit_except");
    bool has_at_least_one_method = false;
    while (parser.current_token.type == WORD) {
        HttpMethod method;
        if (parser.current_token.value == "GET") method = GET;
        else if (parser.current_token.value == "POST") method = POST;
        else if (parser.current_token.value == "DELETE") method = DELETE;
        else if (parser.current_token.value == "PUT") method = PUT;
        else if (parser.current_token.value == "HEAD") method = HEAD;
        else if (parser.current_token.value == "CONNECT") method = CONNECT;
        else if (parser.current_token.value == "OPTIONS") method = OPTIONS;
        else if (parser.current_token.value == "TRACE") method = TRACE;
        else if (parser.current_token.value == "PATCH") method = PATCH;
        else throw_unexpected_token_error("Invalid HTTP method in 'limit_except' directive: " + parser.current_token.value);

        if (ParserDirectiveUtils::has_method(location_config.limit_except, method)) {
            throw_unexpected_token_error(
                "Duplicated HTTP method in 'limit_except' directive: " + parser.current_token.value);
        }
        location_config.limit_except.push_back(method);
        has_at_least_one_method = true;
        parser.advance();
    }
    if (!has_at_least_one_method)
        throw_unexpected_token_error("'limit_except' directive requires at least one HTTP method");
    if (parser.current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'limit_except' directive value: " + parser.current_token.value);
}

void ConfigParserLocation::parse_location_upload_store() {
    parser.validates_directive_value_for("upload_store");
    location_config.upload_store = parser.current_token.value;
    parser.validates_extra_arguments_in("upload_store");
}

void ConfigParserLocation::parse_location_cgi_pass() {
    parser.validates_directive_value_for("cgi_pass");
    location_config.cgi_pass = parser.current_token.value;
    parser.validates_extra_arguments_in("cgi_pass");
}

void ConfigParserLocation::parse_location_redirect() {
    parser.validates_directive_value_for("return");
    if (location_config.return_code != 0) {
        throw_unexpected_token_error(
            "Multiple 'return' directives in the same location block are not allowed");
    }
    std::vector<std::string> parts = std::vector<std::string>();
    while (parser.current_token.type == WORD) {
        parts.push_back(parser.current_token.value);
        parser.advance();
    }
    if (parser.current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'return' directive value: " + parser.current_token.value);
    if (parts.size() != 2)
        throw_unexpected_token_error("'return' requires exactly 2 arguments: code and URL");
    std::stringstream ss(parts[0]);
    int return_code = 0;
    if (!(ss >> return_code) || !ss.eof())
        throw_unexpected_token_error("Invalid HTTP status code: " + parts[0]);
    if (!ParserDirectiveUtils::is_valid_redirect_status_code(return_code))
        throw_unexpected_token_error("'return' directive only supports redirect status codes (3xx): " + parts[0]);
    if (!ParserDirectiveUtils::is_valid_redirect_target(parts[1]))
        throw_unexpected_token_error("Invalid URL in 'return' directive: " + parts[1]);

    location_config.return_code = return_code;
    location_config.return_url = parts[1];
}

void ConfigParserLocation::parse_location_root() {
    parser.validates_directive_value_for("root");
    location_config.root = parser.current_token.value;
    parser.validates_extra_arguments_in("root");
}

LocationConfig ConfigParserLocation::parse() {
    std::string directive("location");
    parser.validates_directive_value_for(directive);
    parse_location_value();
    parser.validates_char_block_at(directive);
    while (parser.current_token.type != RIGHT_BRACE) {
        if (parser.current_token.type == EOF_TOKEN)
            throw_unexpected_token_error("Unexpected end of file while parsing location block");
        parser.current_token.directive_type  = ParsingUtils::get_location_directive_type(parser.current_token.value);
        switch (parser.current_token.directive_type) {
            case LOCATION_ROOT: parse_location_root(); break;
            case LOCATION_INDEX: parse_location_index(); break;
            case LOCATION_LIMITS_EXCEPT: parse_location_methods(); break;
            case LOCATION_AUTOINDEX: parse_location_autoindex(); break;
            case LOCATION_UPLOAD_STORE: parse_location_upload_store(); break;
            case LOCATION_CGI_PASS: parse_location_cgi_pass(); break;
            case LOCATION_REDIRECT: parse_location_redirect(); break;
            default: parser.log_token_alert(directive); break;
        }
        parser.advance();
    }
    parser.validates_char_block_at(directive);

    return location_config;
}
