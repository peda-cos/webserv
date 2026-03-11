#include <sstream>
#include <ConfigParseSyntaxException.hpp>

const char* ConfigParse::SyntaxException::what() const throw() {
    static std::string message;
    std::stringstream ss;
    ss  << "ConfigParser: \"" << get_context_string() << "\" context failed to parse configuration file. " 
        << std::endl << "Invalid syntax at line " << source_position.line
        << ", column " << source_position.column;
    if (!custom_message.empty()) {
        ss << ": " << custom_message;
    }
    ss << std::endl;
    message = ss.str();

    return message.c_str();
}

std::string ConfigParse::SyntaxException::get_context_string() const {
    switch (context) {
        case CONFIG_LEXER: return "lexer";
        case CONFIG_PARSER: return "parser";
        case CONFIG_PARSER_SERVER: return "server block";
        case CONFIG_PARSER_LOCATION: return "location block";
        default: return "unknown context";
    }
}