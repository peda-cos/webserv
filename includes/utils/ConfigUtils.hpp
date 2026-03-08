#pragma once

#include "ConfigToken.hpp"

class ConfigUtils {
    public:
        static ConfigTokenType getConfigDelimiterType(char c);
        static bool is_valid_char_for_config_word(char c);
        static bool is_delimiter(char c);
};