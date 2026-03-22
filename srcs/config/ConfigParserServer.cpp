#include <sstream>
#include <Logger.hpp>
#include <ParsingUtils.hpp>
#include <ParserDirectiveUtils.hpp>
#include <ConfigParserServer.hpp>
#include <ConfigParserLocation.hpp>
#include <ConfigParseSyntaxException.hpp>

ConfigParserServer::ConfigParserServer(ConfigParser &parser): parser(parser) {
    server_config.host = "localhost";
    server_config.port = "8080";
    server_config.server_names = std::vector<std::string>();
    server_config.error_pages = std::map<int, std::string>();
    server_config.client_max_body_size = 1024 * 1024 * 10;
    server_config.autoindex = OFF;
    server_config.limit_except = std::vector<HttpMethod>();
    server_config.locations = std::vector<LocationConfig>();
    server_config.root = "/var/www/html";
    server_config.index = std::vector<std::string>();
}

void ConfigParserServer::throw_unexpected_token_error(const std::string& message) {
    throw ConfigParse::SyntaxException(message,
        ConfigParse::CONFIG_PARSER_SERVER, parser.current_token.source_position);
}

void ConfigParserServer::parse_server_listen() {
    parser.validates_directive_value_for("listen");
    std::vector<std::string> parts = ParsingUtils::split(parser.current_token.value, ':');
    if (parts.size() == 0 || parts.size() > 2)
        throw_unexpected_token_error("Invalid format for 'listen' directive: " + parser.current_token.value);
    int parsed_port = 0;
    if (parts.size() == 1) {
        if (!ParserDirectiveUtils::parse_port(parts[0], parsed_port))
            throw_unexpected_token_error("Invalid port in 'listen' directive: " + parts[0]);
        server_config.port = parts[0];
    } else {
        if (!ParserDirectiveUtils::is_valid_listen_host(parts[0]))
            throw_unexpected_token_error("Invalid host in 'listen' directive: " + parts[0]);
        if (!ParserDirectiveUtils::parse_port(parts[1], parsed_port))
            throw_unexpected_token_error("Invalid port in 'listen' directive: " + parts[1]);
        server_config.host = parts[0];
        server_config.port = parts[1];
    }
    parser.validates_extra_arguments_in("listen");
}

void ConfigParserServer::parse_server_names() {
    parser.validates_directive_value_for("server_name");
    while (parser.current_token.type == WORD) {
        if (!ParserDirectiveUtils::is_valid_hostname(parser.current_token.value)) {
            throw_unexpected_token_error(
                "Invalid server name in 'server_name' directive: " + parser.current_token.value);
        }
        server_config.server_names.push_back(parser.current_token.value);
        parser.advance();
    }
    if (parser.current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'server_name' directive value: " + parser.current_token.value);
}

void ConfigParserServer::parse_client_max_body_size() {
    parser.validates_directive_value_for("client_max_body_size");
    size_t parsed_value = 0;
    if (!ParserDirectiveUtils::parse_body_size(parser.current_token.value, parsed_value))
        throw_unexpected_token_error("Invalid value for 'client_max_body_size' directive: " + parser.current_token.value);
    server_config.client_max_body_size = parsed_value;
    parser.validates_extra_arguments_in("client_max_body_size");
}

void ConfigParserServer::parse_error_page() {
    parser.validates_directive_value_for("error_page");
    std::vector<std::string> parts = std::vector<std::string>();
    while (parser.current_token.type == WORD) {
        parts.push_back(parser.current_token.value);
        parser.advance();
    }
    if (parser.current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'error_page' directive value: " + parser.current_token.value);
    if (parts.size() < 2)
        throw_unexpected_token_error("error_page directive requires at least one status code and a path");

    std::string path = parts[parts.size() - 1];
    if (!ParserDirectiveUtils::is_valid_error_page_path(path))
        throw_unexpected_token_error("error_page path must start with '/', '@', or 'http': " + path);

    for (size_t i = 0; i < parts.size() - 1; ++i) {
        std::stringstream ss(parts[i]);
        int error_code = 0;
        if (!(ss >> error_code) || !ss.eof())
            throw_unexpected_token_error("Invalid error code: " + parts[i]);
        if (!ParserDirectiveUtils::is_valid_error_status_code(error_code))
            throw_unexpected_token_error("Unsupported error code in 'error_page': " + parts[i]);
        server_config.error_pages[error_code] = path;
    }
}

void ConfigParserServer::parse_server_root() {
    parser.validates_directive_value_for("root");
    server_config.root = parser.current_token.value;
    parser.validates_extra_arguments_in("root");
}

ServerConfig ConfigParserServer::parse() {
    parser.advance();
    parser.validates_char_block_at("server");
    while (parser.current_token.type != RIGHT_BRACE) {
        if (parser.current_token.type == EOF_TOKEN)
            throw_unexpected_token_error("Unexpected end of file while parsing server block");
        parser.current_token.directive_type = ParsingUtils::get_server_directive_type(parser.current_token.value);
        switch (parser.current_token.directive_type) {
            case SERVER_ROOT: parse_server_root(); break;
            case SERVER_LISTEN: parse_server_listen(); break;
            case SERVER_NAMES: parse_server_names(); break;
            case SERVER_CLIENT_MAX_BODY_SIZE: parse_client_max_body_size(); break;
            case SERVER_ERROR_PAGE: parse_error_page(); break;
            case SERVER_LOCATION: {
                ConfigParserLocation location_parser(parser);
                location_parser.inherit_error_pages_from_server(server_config.error_pages);
                LocationConfig location_config = location_parser.parse();
                server_config.locations.push_back(location_config);
                continue;
            }
            default: parser.log_token_alert("server"); break;
        }
        parser.advance();
    }
    parser.validates_char_block_at("server");

    return server_config;
}