#pragma once

#include <vector>
#include "Config.hpp"
#include "ConfigToken.hpp"
#include "ServerConfig.hpp"

class ConfigParser {
    private:
        Config config;
        std::vector<ConfigToken> tokens;
        size_t current_token_index;
        size_t total_tokens;
        ConfigToken current_token;
        

        void advance();
        ServerConfig parse_server();
        LocationConfig parse_location();

        std::string get_server_listen();
        std::string get_server_name();

        void validates_directive_value_for(const std::string& directive_name);
        void validates_extra_arguments_in(const std::string& directive_name);
    public:
        ConfigParser(std::vector<ConfigToken> &tokens);

        Config parse();

        class SourceInvalidSyntaxException : public std::exception {
            private:
                SourcePosition source_position;
                std::string custom_message;
            public:
                SourceInvalidSyntaxException(const SourcePosition& pos) : source_position(pos) {}
                SourceInvalidSyntaxException(const SourcePosition& pos, const std::string& msg) 
                    : source_position(pos), custom_message(msg) {}
                virtual ~SourceInvalidSyntaxException() throw() {}
                virtual const char* what() const throw();
        };
};