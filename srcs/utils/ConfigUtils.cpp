#include "ConfigUtils.hpp"
#include "ParsingUtils.hpp"

#include <fstream>
#include <sstream>
#include <Logger.hpp>

ConfigTokenType ConfigUtils::getConfigDelimiterType(char c) {
    switch (c) {
        case '{': return LEFT_BRACE;
        case '}': return RIGHT_BRACE;
        default: return SEMICOLON;
    }
}

bool ConfigUtils::is_valid_char_for_config_word(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '.' || c == '/' || c == ':' ||
           c == '_' || c == '-' || c == '*' ||
           c == '~' || c == '=' ||
           c == '[' || c == ']';
}

bool ConfigUtils::is_delimiter(char c) {
    return c == '{' || c == '}' || c == ';';
}

std::string ConfigUtils::get_config_content(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        Logger::error("Could not open configuration file: " + path);
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
