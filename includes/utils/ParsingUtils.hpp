#ifndef PARSING_UTILS_HPP
#define PARSING_UTILS_HPP

#include <string>
#include <vector>
#include "Enums.hpp"

namespace ParsingUtils {
    bool is_whitespace(char c);
    std::vector<std::string> split(const std::string& str, char delimiter);
    ConfigDirectiveType get_root_directive_type(const std::string& word);
    ConfigDirectiveType get_server_directive_type(const std::string& word);
    ConfigDirectiveType get_location_directive_type(const std::string& word);
}

#endif