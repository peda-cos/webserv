#include "ConfigUtils.hpp"
#include "ParsingUtils.hpp"

ConfigTokenType ConfigUtils::getConfigDelimiterType(char c) {
    switch (c) {
        case '{': return LEFT_BRACE;
        case '}': return RIGHT_BRACE;
        case ';': return SEMICOLON;
        default: return UNKNOWN;
    }
}

bool ConfigUtils::isValidCharForConfigWord(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '.' || c == '/' || c == ':' ||
           c == '_' || c == '-' || c == '*';
}
