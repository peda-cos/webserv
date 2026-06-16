#include <sstream>
#include <set>
#include <Enums.hpp>
#include <Logger.hpp>
#include <ConfigParser.hpp>
#include <ParsingUtils.hpp>
#include <ParserDirectiveUtils.hpp>
#include <ConfigParseSyntaxException.hpp>

ConfigParser::ConfigParser(std::vector<ConfigToken> &config_tokens) :
    tokens(config_tokens), current_token(tokens[0]), current_token_index(0), total_tokens(config_tokens.size()) {}

void ConfigParser::throw_unexpected_token_error(const std::string& message) {
    throw ConfigParse::SyntaxException(message,
        ConfigParse::CONFIG_PARSER, current_token.source_position);
}

void ConfigParser::throw_unexpected_token_error(const std::string& message, ConfigParse::ParserContext ctx) {
    throw ConfigParse::SyntaxException(message, ctx, current_token.source_position);
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

void ConfigParser::parse_server_listen(ServerConfig& server_config) {
    validates_directive_value_for("listen");
    std::vector<std::string> parts = ParsingUtils::split(current_token.value, ':');
    if (parts.size() == 0 || parts.size() > 2)
        throw_unexpected_token_error("Invalid format for 'listen' directive: " + current_token.value, ConfigParse::CONFIG_PARSER_SERVER);
    int parsed_port = 0;
    if (parts.size() == 1) {
        if (!ParserDirectiveUtils::parse_port(parts[0], parsed_port))
            throw_unexpected_token_error("Invalid port in 'listen' directive: " + parts[0], ConfigParse::CONFIG_PARSER_SERVER);
        server_config.port = parts[0];
    } else {
        if (!ParserDirectiveUtils::is_valid_listen_host(parts[0]))
            throw_unexpected_token_error("Invalid host in 'listen' directive: " + parts[0], ConfigParse::CONFIG_PARSER_SERVER);
        if (!ParserDirectiveUtils::parse_port(parts[1], parsed_port))
            throw_unexpected_token_error("Invalid port in 'listen' directive: " + parts[1], ConfigParse::CONFIG_PARSER_SERVER);
        server_config.host = parts[0];
        server_config.port = parts[1];
    }
    validates_extra_arguments_in("listen");
}

void ConfigParser::parse_server_names(ServerConfig& server_config) {
    validates_directive_value_for("server_name");
    while (current_token.type == WORD) {
        if (!ParserDirectiveUtils::is_valid_hostname(current_token.value)) {
            throw_unexpected_token_error(
                "Invalid server name in 'server_name' directive: " + current_token.value, ConfigParse::CONFIG_PARSER_SERVER);
        }
        server_config.server_names.push_back(current_token.value);
        advance();
    }
    if (current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'server_name' directive value: " + current_token.value, ConfigParse::CONFIG_PARSER_SERVER);
}

void ConfigParser::parse_client_max_body_size(ServerConfig& server_config) {
    validates_directive_value_for("client_max_body_size");
    size_t parsed_value = 0;
    if (!ParserDirectiveUtils::parse_body_size(current_token.value, parsed_value))
        throw_unexpected_token_error("Invalid value for 'client_max_body_size' directive: " + current_token.value, ConfigParse::CONFIG_PARSER_SERVER);
    server_config.client_max_body_size = parsed_value;
    validates_extra_arguments_in("client_max_body_size");
}

void ConfigParser::parse_server_error_page(ServerConfig& server_config) {
    validates_directive_value_for("error_page");
    std::vector<std::string> parts;
    while (current_token.type == WORD) {
        parts.push_back(current_token.value);
        advance();
    }
    if (current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'error_page' directive value: " + current_token.value, ConfigParse::CONFIG_PARSER_SERVER);
    if (parts.size() < 2)
        throw_unexpected_token_error("error_page directive requires at least one status code and a path", ConfigParse::CONFIG_PARSER_SERVER);

    std::string path = parts[parts.size() - 1];
    if (!ParserDirectiveUtils::is_valid_error_page_path(path))
        throw_unexpected_token_error("error_page path must start with '/', '@', or 'http': " + path, ConfigParse::CONFIG_PARSER_SERVER);

    for (size_t i = 0; i < parts.size() - 1; ++i) {
        std::stringstream ss(parts[i]);
        int error_code = 0;
        if (!(ss >> error_code) || !ss.eof())
            throw_unexpected_token_error("Invalid error code: " + parts[i], ConfigParse::CONFIG_PARSER_SERVER);
        if (!ParserDirectiveUtils::is_valid_error_status_code(error_code))
            throw_unexpected_token_error("Unsupported error code in 'error_page': " + parts[i], ConfigParse::CONFIG_PARSER_SERVER);
        server_config.error_pages[error_code] = path;
    }
}

void ConfigParser::parse_server_root(ServerConfig& server_config) {
    validates_directive_value_for("root");
    server_config.root = current_token.value;
    validates_extra_arguments_in("root");
}

void ConfigParser::parse_server_index(ServerConfig& server_config) {
    validates_directive_value_for("index");
    while (current_token.type == WORD) {
        server_config.index.push_back(current_token.value);
        advance();
    }
    if (server_config.index.empty())
        throw_unexpected_token_error("index directive requires at least one value", ConfigParse::CONFIG_PARSER_SERVER);
    if (current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'index' directive value: " + current_token.value, ConfigParse::CONFIG_PARSER_SERVER);
}

ServerConfig ConfigParser::parse_server_block() {
    ServerConfig server_config;
    server_config.host = "localhost";
    server_config.port = "8080";
    server_config.client_max_body_size = 1024 * 1024 * 10;
    server_config.autoindex = OFF;
    server_config.root = "www";

    advance();
    validates_char_block_at("server");
    while (current_token.type != RIGHT_BRACE) {
        if (current_token.type == EOF_TOKEN)
            throw_unexpected_token_error("Unexpected end of file while parsing server block", ConfigParse::CONFIG_PARSER_SERVER);
        current_token.directive_type = ParsingUtils::get_server_directive_type(current_token.value);
        switch (current_token.directive_type) {
            case SERVER_ROOT: parse_server_root(server_config); break;
            case SERVER_LISTEN: parse_server_listen(server_config); break;
            case SERVER_NAMES: parse_server_names(server_config); break;
            case SERVER_CLIENT_MAX_BODY_SIZE: parse_client_max_body_size(server_config); break;
            case SERVER_ERROR_PAGE: parse_server_error_page(server_config); break;
            case SERVER_INDEX: parse_server_index(server_config); break;
            case SERVER_LOCATION: {
                LocationConfig location_config = parse_location_block(server_config.error_pages);
                if (location_config.index.empty() && !server_config.index.empty()) {
                    location_config.index = server_config.index[0];
                }
                server_config.locations.push_back(location_config);
                continue;
            }
            default: log_token_alert("server"); break;
        }
        advance();
    }
    validates_char_block_at("server");

    return server_config;
}

void ConfigParser::parse_location_value(LocationConfig& location_config) {
    std::vector<std::string> parts;
    while (current_token.type == WORD) {
        parts.push_back(current_token.value);
        advance();
    }
    int parts_size = static_cast<int>(parts.size());
    if (parts_size > 2)
        throw_unexpected_token_error("Too many arguments for 'location' directive: " + current_token.value, ConfigParse::CONFIG_PARSER_LOCATION);
    if (parts_size == 0)
        throw_unexpected_token_error("location directive requires a path", ConfigParse::CONFIG_PARSER_LOCATION);
    location_config.path = parts[parts_size - 1];
    if (location_config.path[0] != '/' && location_config.path[0] != '@')
        throw_unexpected_token_error("location path must start with '/' or '@': " + location_config.path, ConfigParse::CONFIG_PARSER_LOCATION);
    if (parts_size > 1) {
        location_config.modifier = parts[0];
        if (location_config.modifier != "="
            && location_config.modifier != "~"
            && location_config.modifier != "~*")
            throw_unexpected_token_error("Invalid location modifier: " + location_config.modifier, ConfigParse::CONFIG_PARSER_LOCATION);
    }
}

void ConfigParser::parse_location_index(LocationConfig& location_config) {
    validates_directive_value_for("index");
    location_config.index = current_token.value;
    validates_extra_arguments_in("index");
}

void ConfigParser::parse_location_autoindex(LocationConfig& location_config) {
    validates_directive_value_for("autoindex");
    if (current_token.value == "on") {
        location_config.autoindex = ON;
    } else if (current_token.value == "off") {
        location_config.autoindex = OFF;
    } else {
        throw_unexpected_token_error("Invalid value for 'autoindex' directive: " + current_token.value, ConfigParse::CONFIG_PARSER_LOCATION);
    }
    validates_extra_arguments_in("autoindex");
}

void ConfigParser::parse_location_methods(LocationConfig& location_config) {
    validates_directive_value_for("limit_except");
    bool has_at_least_one_method = false;
    while (current_token.type == WORD) {
        HttpMethod method = GET;
        if (current_token.value == "GET") method = GET;
        else if (current_token.value == "POST") method = POST;
        else if (current_token.value == "DELETE") method = DELETE;
        else if (current_token.value == "PUT") method = PUT;
        else if (current_token.value == "HEAD") method = HEAD;
        else if (current_token.value == "CONNECT") method = CONNECT;
        else if (current_token.value == "OPTIONS") method = OPTIONS;
        else if (current_token.value == "TRACE") method = TRACE;
        else if (current_token.value == "PATCH") method = PATCH;
        else throw_unexpected_token_error("Invalid HTTP method in 'limit_except' directive: " + current_token.value, ConfigParse::CONFIG_PARSER_LOCATION);

        if (ParserDirectiveUtils::has_method(location_config.limit_except, method)) {
            throw_unexpected_token_error(
                "Duplicated HTTP method in 'limit_except' directive: " + current_token.value, ConfigParse::CONFIG_PARSER_LOCATION);
        }
        location_config.limit_except.push_back(method);
        has_at_least_one_method = true;
        advance();
    }
    if (!has_at_least_one_method)
        throw_unexpected_token_error("'limit_except' directive requires at least one HTTP method", ConfigParse::CONFIG_PARSER_LOCATION);
    if (current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'limit_except' directive value: " + current_token.value, ConfigParse::CONFIG_PARSER_LOCATION);
    if (ParserDirectiveUtils::has_method(location_config.limit_except, GET)) {
        if (!ParserDirectiveUtils::has_method(location_config.limit_except, HEAD)) {
            location_config.limit_except.push_back(HEAD);
        }
    }
}

void ConfigParser::parse_location_upload_store(LocationConfig& location_config) {
    validates_directive_value_for("upload_store");
    location_config.upload_store = current_token.value;
    validates_extra_arguments_in("upload_store");
}

void ConfigParser::parse_location_cgi_pass(LocationConfig& location_config) {
    validates_directive_value_for("cgi_pass");
    std::string extension = current_token.value;

    if (!ParserDirectiveUtils::is_valid_cgi_extension(extension)) {
        throw_unexpected_token_error("Invalid CGI extension format (must start with '.'): " + extension, ConfigParse::CONFIG_PARSER_LOCATION);
    }

    advance();

    if (current_token.type != WORD) {
        throw_unexpected_token_error("cgi_pass directive requires extension and handler path", ConfigParse::CONFIG_PARSER_LOCATION);
    }

    std::string handler_path = current_token.value;

    if (!ParserDirectiveUtils::is_valid_cgi_handler_path(handler_path)) {
        throw_unexpected_token_error("Invalid CGI handler path (must start with '/' for absolute path): " + handler_path, ConfigParse::CONFIG_PARSER_LOCATION);
    }

    if (location_config.cgi_handlers.find(extension) != location_config.cgi_handlers.end()) {
        throw_unexpected_token_error("Duplicate cgi_pass extension: " + extension, ConfigParse::CONFIG_PARSER_LOCATION);
    }

    location_config.cgi_handlers[extension] = handler_path;
    validates_extra_arguments_in("cgi_pass");
}

void ConfigParser::parse_location_redirect(LocationConfig& location_config) {
    validates_directive_value_for("return");
    if (location_config.return_code != 0) {
        throw_unexpected_token_error(
            "Multiple 'return' directives in the same location block are not allowed", ConfigParse::CONFIG_PARSER_LOCATION);
    }
    std::vector<std::string> parts;
    while (current_token.type == WORD) {
        parts.push_back(current_token.value);
        advance();
    }
    if (current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'return' directive value: " + current_token.value, ConfigParse::CONFIG_PARSER_LOCATION);
    if (parts.size() != 2)
        throw_unexpected_token_error("'return' requires exactly 2 arguments: code and URL", ConfigParse::CONFIG_PARSER_LOCATION);
    std::stringstream ss(parts[0]);
    int return_code = 0;
    if (!(ss >> return_code) || !ss.eof())
        throw_unexpected_token_error("Invalid HTTP status code: " + parts[0], ConfigParse::CONFIG_PARSER_LOCATION);
    if (!ParserDirectiveUtils::is_valid_redirect_status_code(return_code))
        throw_unexpected_token_error("'return' directive only supports redirect status codes (3xx): " + parts[0], ConfigParse::CONFIG_PARSER_LOCATION);
    if (!ParserDirectiveUtils::is_valid_redirect_target(parts[1]))
        throw_unexpected_token_error("Invalid URL in 'return' directive: " + parts[1], ConfigParse::CONFIG_PARSER_LOCATION);

    location_config.return_code = return_code;
    location_config.return_url = parts[1];
}

void ConfigParser::parse_location_root(LocationConfig& location_config) {
    validates_directive_value_for("root");
    location_config.root = current_token.value;
    validates_extra_arguments_in("root");
}

void ConfigParser::parse_location_error_page(LocationConfig& location_config) {
    validates_directive_value_for("error_page");
    std::vector<std::string> parts;
    while (current_token.type == WORD) {
        parts.push_back(current_token.value);
        advance();
    }
    if (current_token.type != SEMICOLON)
        throw_unexpected_token_error("Unexpected token after 'error_page' directive value: " + current_token.value, ConfigParse::CONFIG_PARSER_LOCATION);
    if (parts.size() < 2)
        throw_unexpected_token_error("error_page directive requires at least one status code and a path", ConfigParse::CONFIG_PARSER_LOCATION);

    std::string path = parts[parts.size() - 1];
    if (!ParserDirectiveUtils::is_valid_error_page_path(path))
        throw_unexpected_token_error("error_page path must start with '/', '@', or 'http': " + path, ConfigParse::CONFIG_PARSER_LOCATION);

    for (size_t i = 0; i < parts.size() - 1; ++i) {
        std::stringstream ss(parts[i]);
        int error_code = 0;
        if (!(ss >> error_code) || !ss.eof())
            throw_unexpected_token_error("Invalid error code: " + parts[i], ConfigParse::CONFIG_PARSER_LOCATION);
        if (!ParserDirectiveUtils::is_valid_error_status_code(error_code))
            throw_unexpected_token_error("Unsupported error code in 'error_page': " + parts[i], ConfigParse::CONFIG_PARSER_LOCATION);
        location_config.error_pages[error_code] = path;
    }
}

LocationConfig ConfigParser::parse_location_block(const std::map<int, std::string>& server_error_pages) {
    LocationConfig location_config;
    location_config.path = "/";
    location_config.root = "";
    location_config.index = "";
    location_config.autoindex = OFF;
    location_config.upload_store = "";
    location_config.return_url = "";
    location_config.return_code = 0;
    location_config.error_pages = server_error_pages;

    std::string directive("location");
    validates_directive_value_for(directive);
    parse_location_value(location_config);
    validates_char_block_at(directive);
    while (current_token.type != RIGHT_BRACE) {
        if (current_token.type == EOF_TOKEN)
            throw_unexpected_token_error("Unexpected end of file while parsing location block", ConfigParse::CONFIG_PARSER_LOCATION);
        current_token.directive_type = ParsingUtils::get_location_directive_type(current_token.value);
        switch (current_token.directive_type) {
            case LOCATION_ROOT: parse_location_root(location_config); break;
            case LOCATION_INDEX: parse_location_index(location_config); break;
            case LOCATION_LIMITS_EXCEPT: parse_location_methods(location_config); break;
            case LOCATION_AUTOINDEX: parse_location_autoindex(location_config); break;
            case LOCATION_UPLOAD_STORE: parse_location_upload_store(location_config); break;
            case LOCATION_CGI_PASS: parse_location_cgi_pass(location_config); break;
            case LOCATION_REDIRECT: parse_location_redirect(location_config); break;
            case LOCATION_ERROR_PAGE: parse_location_error_page(location_config); break;
            default: log_token_alert(directive); break;
        }
        advance();
    }
    validates_char_block_at(directive);

    return location_config;
}

Config ConfigParser::parse() {
    std::map<std::string, std::vector<ServerConfig*> > port_to_servers;
    while (current_token.type != EOF_TOKEN) {
        current_token.directive_type = ParsingUtils::get_root_directive_type(current_token.value);
        switch (current_token.directive_type) {
            case ROOT_SERVER: {
                ServerConfig server_config = parse_server_block();
                config.servers.push_back(server_config);
                port_to_servers[server_config.port].push_back(&config.servers.back());
                continue;
            }
            default: {
                std::string detail_message("Unexpected token at root level: " + current_token.value);
                throw_unexpected_token_error(detail_message);
            }
        }
        advance();
    }

    for (std::map<std::string, std::vector<ServerConfig*> >::iterator it = port_to_servers.begin();
         it != port_to_servers.end(); ++it) {
        if (it->second.size() > 1) {
            for (size_t i = 0; i < it->second.size(); ++i) {
                if (it->second[i]->server_names.empty()) {
                    throw ConfigParse::SyntaxException(
                        "Duplicate listen port without server_name not allowed: " + it->first,
                        ConfigParse::CONFIG_PARSER_SERVER,
                        it->second[i]->server_names.empty() ? SourcePosition() : SourcePosition());
                }
            }
        }
    }

    return config;
}
