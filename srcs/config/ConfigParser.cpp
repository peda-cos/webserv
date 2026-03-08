#include "ConfigParser.hpp"
#include "ParsingUtils.hpp"
#include <Logger.hpp>
#include "Enums.hpp"
#include <sstream>

ConfigParser::ConfigParser(std::vector<ConfigToken> &config_tokens) : 
    config(), tokens(config_tokens), current_token_index(0), 
    total_tokens(config_tokens.size()), current_token(tokens[0]) {}

const char* ConfigParser::SourceInvalidSyntaxException::what() const throw() {
    static std::string message;
    std::stringstream ss;
    ss << "'ConfigParser': Invalid directive in configuration file at "
        << "line " << source_position.line  << ", column " << source_position.column;
    message = ss.str();
    return message.c_str();
}

void ConfigParser::advance() {
    if (current_token_index < total_tokens - 1) {
        current_token_index++;
        current_token = tokens[current_token_index];
    }
}

void ConfigParser::validates_directive_value_for(const std::string& directive_name) {
    advance();
    if (current_token.type != WORD) {
        std::string message = "Expected a value for '" + directive_name + "' directive";
        throw SourceInvalidSyntaxException(current_token.source_position, message);
    }
}

void ConfigParser::validates_extra_arguments_in(const std::string& directive_name) {
    advance();
    if (current_token.type != SEMICOLON) {
        std::string message = "Unexpected extra arguments for '" + directive_name + "' directive";
        throw SourceInvalidSyntaxException(current_token.source_position, message);
    }
}

ServerConfig ConfigParser::parse_server() {
    ServerConfig server_config = { // DEFAULT_VALUES
        .host = "localhost", .port = "8080",
        .server_names = std::vector<std::string>(),
        .error_pages = std::map<int, std::string>(),
        .client_max_body_size = 1024 * 1024 * 10, .autoindex = OFF,
        .limit_except = std::vector<HttpMethod>(), .locations = std::vector<LocationConfig>(),
        .root = "/var/www/html", .index = std::vector<std::string>(),
    };

    advance();
    if (current_token.type != LEFT_BRACE) {
        throw SourceInvalidSyntaxException(current_token.source_position);
    }
    advance();
    while (current_token.type != RIGHT_BRACE) {
        if (current_token.type == EOF_TOKEN) {
            throw SourceInvalidSyntaxException(current_token.source_position);
        }
        current_token.directive_type  = ParsingUtils::get_server_directive_type(current_token.value);
        switch (current_token.directive_type) {
            // TODO: Extrair para metodos privados de parseamento de cada diretiva
            case SERVER_LISTEN: {
                validates_directive_value_for("listen");
                std::vector<std::string> parts = ParsingUtils::split(current_token.value, ':');
                if (parts.size() == 0 || parts.size() > 2) {
                    std::string message = "Invalid value for 'listen' directive: " + current_token.value;
                    throw SourceInvalidSyntaxException(current_token.source_position, message);
                }
                if (parts.size() == 1) {
                    server_config.port = parts[0];
                } else {
                    server_config.host = parts[0];
                    server_config.port = parts[1];
                }
                validates_extra_arguments_in("listen");
                break;
            }
            case SERVER_NAME: {
                validates_directive_value_for("server_name");
                while (current_token.type == WORD) {
                    server_config.server_names.push_back(current_token.value);
                    advance();
                }
                validates_extra_arguments_in("server_name");
                break;
            }
            case CLIENT_MAX_BODY_SIZE: {
                validates_directive_value_for("client_max_body_size");
                std::stringstream ss(current_token.value);
                size_t value;
                if (!(ss >> value)) {
                    std::string message = "Invalid value for 'client_max_body_size' directive: " + current_token.value;
                    throw SourceInvalidSyntaxException(current_token.source_position, message);
                }
                validates_extra_arguments_in("client_max_body_size");
                break;
            }
            case ERROR_PAGE: {
                validates_directive_value_for("error_page");
                std::vector<std::string> parts = std::vector<std::string>();
                while (current_token.type == WORD) {
                    parts.push_back(current_token.value);
                    advance();
                }
                std::string path = parts[parts.size() - 1];
                if (path.empty()) {
                    std::string message = "Invalid value for 'error_page' directive: " + path;
                    throw SourceInvalidSyntaxException(current_token.source_position, message);
                }
                if (path.empty()) {
                    std::string message = "error_page requires a non-empty path: ";
                    throw SourceInvalidSyntaxException(current_token.source_position, message);
                }
                if (parts.size() < 2) {
                    std::string message = "error_page directive requires at least one status code and a path";
                    throw SourceInvalidSyntaxException(current_token.source_position, message);
                }
                if (path[0] != '/' && path[0] != '@' && path.substr(0, 4) != "http") {
                    std::string message = "error_page path must start with '/', '@', or 'http': " + path;
                    throw SourceInvalidSyntaxException(current_token.source_position, message);
                }
                for (size_t i = 0; i < parts.size() - 1; ++i) {
                    std::stringstream ss(parts[i]);
                    int error_code;
                    if (!(ss >> error_code)) {
                        std::string message = "Invalid error code: " + parts[i];
                        throw SourceInvalidSyntaxException(current_token.source_position, message);
                    }
                    server_config.error_pages[error_code] = path;
                }
                break;
            }
            default: {
                std::stringstream ss;
                ss << "Unknown directive '" << current_token.value << "' in server block at line " 
                    << current_token.source_position.line << ", column " 
                    << current_token.source_position.column;
                Logger::warn(ss.str());
                break;
            }
        }
        advance();
    }

    return server_config;
}

Config ConfigParser::parse() {
    while (current_token.type != EOF_TOKEN) {
        current_token.directive_type  = ParsingUtils::get_root_directive_type(current_token.value);
        switch (current_token.directive_type) {
            case ROOT_SERVER: config.servers.push_back(parse_server()); break;
            default: throw SourceInvalidSyntaxException(current_token.source_position);
        }
        advance();
    }
    return config;
}