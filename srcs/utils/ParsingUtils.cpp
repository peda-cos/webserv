#include "ParsingUtils.hpp"

bool ParsingUtils::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool ParsingUtils::isConfigDelimiter(char c) {
    return c == '{' || c == '}' || c == ';';
}
