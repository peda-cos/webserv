#pragma once

#include "ConfigToken.hpp"

class ConfigUtils {
    public:
        static std::string get_config_content(const std::string& path);
        static ConfigTokenType getConfigDelimiterType(char c);
        static bool is_valid_char_for_config_word(char c);
        static bool is_delimiter(char c);
};