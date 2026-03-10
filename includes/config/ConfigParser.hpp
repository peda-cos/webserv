#pragma once

#include <vector>
#include "Config.hpp"
#include "ConfigToken.hpp"
#include "ServerConfig.hpp"

class ConfigParser {
    private:
        Config config;

        // Token management
        std::vector<ConfigToken> tokens;
        size_t current_token_index;
        size_t total_tokens;
        ConfigToken current_token;

        // Current parsing context
        ServerConfig current_server_config;
        LocationConfig current_location_config;

        void advance();
        ServerConfig parse_server();
        LocationConfig parse_location();

        void validates_char_block_at(const std::string& block_name);
        void validates_directive_value_for(const std::string& directive_name);
        void validates_extra_arguments_in(const std::string& directive_name);

        void init_default_server_config();
        void init_default_location_config();

        // Server Properties Block
        void parse_server_root();
        void parse_server_listen();
        void parse_server_names();
        void parse_client_max_body_size();
        void parse_error_page();

        // Location Properties Block
        void parse_location_value();
        void parse_location_root();
        void parse_location_index();
        void parse_location_autoindex();
        void parse_location_methods();
        void parse_location_upload_store();
        void parse_location_cgi_pass();
        void parse_location_redirect();

        void throw_unexpected_token_error(const std::string& message);

    public:
        ConfigParser(std::vector<ConfigToken> &tokens);

        Config parse();

        class InvalidSyntaxException : public std::exception {
            private:
                SourcePosition source_position;
                std::string custom_message;
            public:
                InvalidSyntaxException(const SourcePosition& pos) : source_position(pos) {}
                InvalidSyntaxException(const SourcePosition& pos, const std::string& msg) 
                    : source_position(pos), custom_message(msg) {}
                virtual ~InvalidSyntaxException() throw() {}
                virtual const char* what() const throw();
        };
};