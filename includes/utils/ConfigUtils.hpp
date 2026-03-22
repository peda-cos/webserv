#ifndef CONFIG_UTILS_HPP
#define CONFIG_UTILS_HPP

#include "ConfigToken.hpp"

namespace ConfigUtils {
    std::string get_config_content(const std::string& path);
    ConfigTokenType getConfigDelimiterType(char c);
    bool is_valid_char_for_config_word(char c);
    bool is_delimiter(char c);
}

#endif