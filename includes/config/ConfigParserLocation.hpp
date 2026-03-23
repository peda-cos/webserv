#ifndef CONFIG_PARSER_LOCATION_HPP
#define CONFIG_PARSER_LOCATION_HPP

#include <string>
#include <vector>
#include <ConfigParser.hpp>
#include <SourcePosition.hpp>
#include <LocationConfig.hpp>

class ConfigParserLocation {
    private:
        ConfigParser &parser;
        LocationConfig location_config;
        void parse_location_value();
        void parse_location_root();
        void parse_location_index();
        void parse_location_autoindex();
        void parse_location_methods();
        void parse_location_upload_store();
        void parse_location_cgi_pass();
        void parse_location_redirect();
        void parse_location_error_page();
        void throw_unexpected_token_error(const std::string& message);
    public:
        ConfigParserLocation(ConfigParser &parser);
        LocationConfig parse();
        void inherit_error_pages_from_server(const std::map<int, std::string>& server_error_pages);
};

#endif