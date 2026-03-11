#ifndef CONFIG_LEXER_HPP
#define CONFIG_LEXER_HPP

#include <string>
#include <vector>
#include <ConfigToken.hpp>
#include <SourcePosition.hpp>

class ConfigLexer {
    private:
        std::string source;
        size_t position;
        size_t source_length;
        SourcePosition source_position;
    
        void advance();
        void skip_comment();
        ConfigToken build_token();
        void throw_unexpected_char_error(char invalid_char);
    public:
        ConfigLexer(const std::string& source);

        ConfigToken get_next_token();
        std::vector<ConfigToken> tokenize();
};

#endif