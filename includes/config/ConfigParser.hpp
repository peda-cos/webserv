#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <vector>
#include <Config.hpp>
#include <ConfigToken.hpp>
#include <ServerConfig.hpp>

class ConfigParser {
    friend class ConfigParserServer;
    friend class ConfigParserLocation;

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
    public:
        ConfigParser(std::vector<ConfigToken> &tokens);
        Config parse();
};

#endif