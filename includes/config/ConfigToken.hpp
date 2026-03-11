#ifndef CONFIG_TOKEN_HPP
#define CONFIG_TOKEN_HPP

#include <string>
#include <Enums.hpp>
#include <SourcePosition.hpp>

struct ConfigToken {
    std::string value;
    ConfigTokenType type;
    SourcePosition source_position;
    ConfigDirectiveType directive_type;

    ConfigToken(const std::string& value, ConfigTokenType type, SourcePosition position)
        : value(value), type(type), source_position(position), directive_type(UNKNOWN) {}
};

#endif