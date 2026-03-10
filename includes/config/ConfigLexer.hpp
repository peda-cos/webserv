#pragma once

#include <string>
#include <vector>
#include "ConfigToken.hpp"
#include "SourcePosition.hpp"

class ConfigLexer {
    private:
        std::string source;
        size_t position;
        size_t source_length;
        SourcePosition source_position;
    
        void advance();
        void skip_comment();
        ConfigToken build_token();
    public:
        ConfigLexer(const std::string& source);

        ConfigToken get_next_token();
        std::vector<ConfigToken> tokenize();

        class SourceInvalidSyntaxException : public std::exception {
            private:
                SourcePosition source_position;
            public:
                SourceInvalidSyntaxException(const SourcePosition& pos) : source_position(pos) {}
                virtual const char* what() const throw();
        };
};