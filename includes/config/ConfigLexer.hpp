#pragma once

#include <string>
#include <vector>
#include "ConfigToken.hpp"
#include "SourcePosition.hpp"

class ConfigLexer {
    private:
        std::string source;
        size_t position;
        size_t sourceLength;
        SourcePosition sourcePosition;
    
        void advance();
        void skipComment();
        ConfigToken buildToken();
    public:
        ConfigLexer(const std::string& source);

        ConfigToken getNextToken();
        std::vector<ConfigToken> tokenize();
};