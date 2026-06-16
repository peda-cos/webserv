#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <vector>
#include <Config.hpp>
#include <ConfigToken.hpp>
#include <ConfigParseSyntaxException.hpp>
#include <ServerConfig.hpp>

class ConfigParser {
    private:
        std::vector<ConfigToken>& tokens;
        ConfigToken& current_token;
        size_t current_token_index;
        size_t total_tokens;
        Config config;

        void advance();
        void log_token_alert(const std::string& block_name);
        void validates_char_block_at(const std::string& block_name);
        void validates_directive_value_for(const std::string& directive_name);
        void validates_extra_arguments_in(const std::string& directive_name);
        void throw_unexpected_token_error(const std::string& message);
        void throw_unexpected_token_error(const std::string& message, ConfigParse::ParserContext ctx);

        ServerConfig parse_server_block();
        LocationConfig parse_location_block(const std::map<int, std::string>& server_error_pages);

        void parse_server_listen(ServerConfig& server_config);
        void parse_server_names(ServerConfig& server_config);
        void parse_client_max_body_size(ServerConfig& server_config);
        void parse_server_error_page(ServerConfig& server_config);
        void parse_server_root(ServerConfig& server_config);
        void parse_server_index(ServerConfig& server_config);

        void parse_location_value(LocationConfig& location_config);
        void parse_location_root(LocationConfig& location_config);
        void parse_location_index(LocationConfig& location_config);
        void parse_location_autoindex(LocationConfig& location_config);
        void parse_location_methods(LocationConfig& location_config);
        void parse_location_upload_store(LocationConfig& location_config);
        void parse_location_cgi_pass(LocationConfig& location_config);
        void parse_location_redirect(LocationConfig& location_config);
        void parse_location_error_page(LocationConfig& location_config);

    public:
        ConfigParser(std::vector<ConfigToken> &tokens);
        Config parse();
};

#endif
