#include "ConfigParser.hpp"
#include "ParsingUtils.hpp"
#include <Logger.hpp>
#include "Enums.hpp"
#include <sstream>

// Auxiliary Functions

void log_token_alert(const std::string& block_name, const ConfigToken& token) {
    std::stringstream ss;
    ss << "Unknown directive '" << token.value << "' in " << block_name 
       << " block at line " << token.source_position.line 
       << ", column " << token.source_position.column;
    Logger::warn(ss.str());
}

// Resources of Class

ConfigParser::ConfigParser(std::vector<ConfigToken> &config_tokens) : 
    config(), tokens(config_tokens), current_token_index(0),
    total_tokens(config_tokens.size()), current_token(tokens[0]),
    current_server_config(), current_location_config() {}

const char* ConfigParser::InvalidSyntaxException::what() const throw() {
    static std::string message;
    std::stringstream ss;
    ss  << "'ConfigParser': Invalid directive in configuration file at "
        << "line " << source_position.line  << ", column " << source_position.column
        << std::endl << custom_message;
    message = ss.str();

    return message.c_str();
}

void ConfigParser::throw_unexpected_token_error(const std::string& message) {
    throw InvalidSyntaxException(current_token.source_position, message);
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

void ConfigParser::init_default_server_config() {
    current_server_config.host = "localhost";
    current_server_config.port = "8080";
    current_server_config.server_names = std::vector<std::string>();
    current_server_config.error_pages = std::map<int, std::string>();
    current_server_config.client_max_body_size = 1024 * 1024 * 10;
    current_server_config.autoindex = OFF;
    current_server_config.limit_except = std::vector<HttpMethod>();
    current_server_config.locations = std::vector<LocationConfig>();
    current_server_config.root = "/var/www/html";
    current_server_config.index = std::vector<std::string>();
}

void ConfigParser::init_default_location_config() {
    current_location_config.path = "/";
    current_location_config.root = "/var/www/html";
    current_location_config.index = "";
    current_location_config.autoindex = OFF;
    current_location_config.limit_except = std::vector<HttpMethod>();
    current_location_config.upload_store = "";
    current_location_config.cgi_pass = "";
    current_location_config.return_url = "";
    current_location_config.return_code = 0;
}

void ConfigParser::parse_server_listen() {
    validates_directive_value_for("listen");
    std::vector<std::string> parts = ParsingUtils::split(current_token.value, ':');
    if (parts.size() == 0 || parts.size() > 2)
        throw_unexpected_token_error("Invalid format for 'listen' directive: " + current_token.value);
    if (parts.size() == 1) {
        current_server_config.port = parts[0];
    } else {
        current_server_config.host = parts[0];
        current_server_config.port = parts[1];
    }
    validates_extra_arguments_in("listen");
}

void ConfigParser::parse_server_names() {
    validates_directive_value_for("server_name");
    while (current_token.type == WORD) {
        current_server_config.server_names.push_back(current_token.value);
        advance();
    }
    if (current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'server_name' directive value: " + current_token.value);
}

void ConfigParser::parse_client_max_body_size() {
    validates_directive_value_for("client_max_body_size");
    std::stringstream ss(current_token.value);
    size_t value;
    if (!(ss >> value))
        throw_unexpected_token_error("Invalid value for 'client_max_body_size' directive: " + current_token.value);
    current_server_config.client_max_body_size = value;
    validates_extra_arguments_in("client_max_body_size");
}

void ConfigParser::parse_error_page() {
    validates_directive_value_for("error_page");
    std::vector<std::string> parts = std::vector<std::string>();
    while (current_token.type == WORD) {
        parts.push_back(current_token.value);
        advance();
    }
    std::string path = parts[parts.size() - 1];
    if (path.empty())
        throw_unexpected_token_error("error_page requires a non-empty path");
    if (parts.size() < 2)
        throw_unexpected_token_error("error_page directive requires at least one status code and a path");
    if (path[0] != '/' && path[0] != '@' && path.substr(0, 4) != "http")
        throw_unexpected_token_error("error_page path must start with '/', '@', or 'http': " + path);
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        std::stringstream ss(parts[i]);
        int error_code;
        if (!(ss >> error_code))
            throw_unexpected_token_error("Invalid error code: " + parts[i]);
        current_server_config.error_pages[error_code] = path;
    }
}

void ConfigParser::parse_server_root() {
    validates_directive_value_for("root");
    current_server_config.root = current_token.value;
    validates_extra_arguments_in("root");
}

ServerConfig ConfigParser::parse_server() {
    init_default_server_config();
    advance();
    validates_char_block_at("server");
    while (current_token.type != RIGHT_BRACE) {
        if (current_token.type == EOF_TOKEN)
            throw InvalidSyntaxException(current_token.source_position);
        current_token.directive_type = ParsingUtils::get_server_directive_type(current_token.value);
        switch (current_token.directive_type) {
            case SERVER_ROOT: parse_server_root(); break;
            case SERVER_LISTEN: parse_server_listen(); break;
            case SERVER_NAMES: parse_server_names(); break;
            case SERVER_CLIENT_MAX_BODY_SIZE: parse_client_max_body_size(); break;
            case SERVER_ERROR_PAGE: parse_error_page(); break;
            case SERVER_LOCATION: current_server_config.locations.push_back(parse_location()); continue;
            default: log_token_alert("server", current_token); break;
        }
        advance();
    }
    validates_char_block_at("server");

    return current_server_config;
}

void ConfigParser::parse_location_value() {
    std::vector<std::string> parts = std::vector<std::string>();
    while (current_token.type == WORD) {
        parts.push_back(current_token.value);
        advance();
    }
    int parts_size = static_cast<int>(parts.size());
    if (parts_size > 2)
        throw_unexpected_token_error("Too many arguments for 'location' directive: " + current_token.value);
    if (parts_size == 0)
        throw_unexpected_token_error("location directive requires a path");
    current_location_config.path = parts[parts_size - 1];
    if (current_location_config.path[0] != '/' && current_location_config.path[0] != '@')
        throw_unexpected_token_error("location path must start with '/' or '@': " + current_location_config.path);
    if (parts_size > 1) {
        current_location_config.modifier = parts[0];
        if (current_location_config.modifier != "=" 
            && current_location_config.modifier != "~" 
            && current_location_config.modifier != "~*")
            throw_unexpected_token_error("Invalid location modifier: " + current_location_config.modifier);
    }
}

void ConfigParser::parse_location_index() {
    validates_directive_value_for("index");
    current_location_config.index = current_token.value;
    validates_extra_arguments_in("index");
}

void ConfigParser::parse_location_autoindex() {
    validates_directive_value_for("autoindex");
    if (current_token.value == "on") {
        current_location_config.autoindex = ON;
    } else if (current_token.value == "off") {
        current_location_config.autoindex = OFF;
    } else {
        throw_unexpected_token_error("Invalid value for 'autoindex' directive: " + current_token.value);
    }
    validates_extra_arguments_in("autoindex");
}

void ConfigParser::parse_location_methods() {
    validates_directive_value_for("limit_except");
    while (current_token.type == WORD) {
        HttpMethod method;
        if (current_token.value == "GET") method = GET;
        else if (current_token.value == "POST") method = POST;
        else if (current_token.value == "DELETE") method = DELETE;
        else if (current_token.value == "PUT") method = PUT;
        else if (current_token.value == "HEAD") method = HEAD;
        else if (current_token.value == "CONNECT") method = CONNECT;
        else if (current_token.value == "OPTIONS") method = OPTIONS;
        else if (current_token.value == "TRACE") method = TRACE;
        else if (current_token.value == "PATCH") method = PATCH;
        else throw_unexpected_token_error("Invalid HTTP method in 'limit_except' directive: " + current_token.value);
        current_location_config.limit_except.push_back(method);
        advance();
    }
    if (current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'limit_except' directive value: " + current_token.value);
}

void ConfigParser::parse_location_upload_store() {
    validates_directive_value_for("upload_store");
    current_location_config.upload_store = current_token.value;
    validates_extra_arguments_in("upload_store");
}

void ConfigParser::parse_location_cgi_pass() {
    validates_directive_value_for("cgi_pass");
    current_location_config.cgi_pass = current_token.value;
    validates_extra_arguments_in("cgi_pass");
}

void ConfigParser::parse_location_redirect() {
    validates_directive_value_for("return");
        if (current_location_config.return_code != 0) {
        throw_unexpected_token_error(
            "Multiple 'return' directives in the same location block are not allowed");
    }
    std::vector<std::string> parts = std::vector<std::string>();
    while (current_token.type == WORD) {
        parts.push_back(current_token.value);
        advance();
    }
    if (parts.size() != 2)
        throw_unexpected_token_error("'return' requires exactly 2 arguments: code and URL");
    std::stringstream ss(parts[0]);
    int return_code;
    if (!(ss >> return_code))
        throw_unexpected_token_error("Invalid HTTP status code: " + parts[0]);
    current_location_config.return_code = return_code;
    current_location_config.return_url = parts[1];
    validates_extra_arguments_in("return");
}

void ConfigParser::parse_location_root() {
    validates_directive_value_for("root");
    current_location_config.root = current_token.value;
    validates_extra_arguments_in("root");
}

LocationConfig ConfigParser::parse_location() {
    std::string directive("location");
    init_default_location_config();
    validates_directive_value_for(directive);
    parse_location_value();
    validates_char_block_at(directive);
    while (current_token.type != RIGHT_BRACE) {
        if (current_token.type == EOF_TOKEN)
            throw InvalidSyntaxException(current_token.source_position);
        current_token.directive_type  = ParsingUtils::get_location_directive_type(current_token.value);
        switch (current_token.directive_type) {
            case LOCATION_ROOT: parse_location_root(); break;
            case LOCATION_INDEX: parse_location_index(); break;
            case LOCATION_LIMITS_EXCEPT: parse_location_methods(); break;
            case LOCATION_AUTOINDEX: parse_location_autoindex(); break;
            case LOCATION_UPLOAD_STORE: parse_location_upload_store(); break;
            case LOCATION_CGI_PASS: parse_location_cgi_pass(); break;
            case LOCATION_REDIRECT: parse_location_redirect(); break;
            default: log_token_alert(directive, current_token); break;
        }
        advance();
    }
    validates_char_block_at(directive);

    return current_location_config;
}

Config ConfigParser::parse() {
    while (current_token.type != EOF_TOKEN) {
        current_token.directive_type  = ParsingUtils::get_root_directive_type(current_token.value);
        switch (current_token.directive_type) {
            case ROOT_SERVER: config.servers.push_back(parse_server()); break;
            default: throw InvalidSyntaxException(current_token.source_position);
        }
        advance();
    }

    return config;
}