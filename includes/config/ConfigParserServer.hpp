#ifndef CONFIG_PARSER_SERVER_HPP
#define CONFIG_PARSER_SERVER_HPP

#include <string>
#include <vector>
#include <exception>
#include <ConfigParser.hpp>
#include <SourcePosition.hpp>
#include <LocationConfig.hpp>

class ConfigParserServer {
    private:
        ConfigParser &parser;
        ServerConfig server_config;
        void parse_server_root();
        void parse_server_listen();
        void parse_server_names();
        void parse_client_max_body_size();
        void parse_error_page();
        void throw_unexpected_token_error(const std::string& message);
    public:
        ConfigParserServer(ConfigParser &parser);
        ServerConfig parse();
};

#endif