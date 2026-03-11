#ifndef CONFIG_PARSE_SYNTAX_EXCEPTION_HPP
#define CONFIG_PARSE_SYNTAX_EXCEPTION_HPP

#include <string>
#include <vector>
#include <exception>
#include <SourcePosition.hpp>

namespace ConfigParse {
    enum ParserContext { 
        CONFIG_LEXER, CONFIG_PARSER, 
        CONFIG_PARSER_SERVER, CONFIG_PARSER_LOCATION 
    };
    class SyntaxException : public std::exception {
        private:
            ParserContext context;
            std::string custom_message;
            SourcePosition source_position;
        public:
            SyntaxException(ParserContext ctx, const SourcePosition& pos) 
                : context(ctx), source_position(pos) {}
            SyntaxException(const std::string& msg, ParserContext ctx, const SourcePosition& pos) 
                : context(ctx), custom_message(msg), source_position(pos) {}
            virtual ~SyntaxException() throw() {}
            virtual const char* what() const throw();

            std::string get_context_string() const;
    };
};

#endif