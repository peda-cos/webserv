#ifndef PARSING_UTILS_HPP
#define PARSING_UTILS_HPP

#include <string>
#include <vector>
#include "Enums.hpp"

class ParsingUtils {
    public:
        static bool is_whitespace(char c);
        static std::vector<std::string> split(const std::string& str, char delimiter);
        static ConfigDirectiveType get_root_directive_type(const std::string& word);
        static ConfigDirectiveType get_server_directive_type(const std::string& word);
        static ConfigDirectiveType get_location_directive_type(const std::string& word);
};

#endif