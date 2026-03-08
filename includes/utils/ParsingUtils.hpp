#pragma once

#include <string>

class ParsingUtils {
    public:
        static bool isWhitespace(char c);
        static bool isConfigDelimiter(char c);
};