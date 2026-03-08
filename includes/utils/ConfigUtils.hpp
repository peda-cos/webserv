#pragma once

#include "ConfigToken.hpp"

class ConfigUtils {
    public:
        static ConfigTokenType getConfigDelimiterType(char c);
        static bool isValidCharForConfigWord(char c);
};