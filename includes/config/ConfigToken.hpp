#pragma once

#include <string>
#include "Enums.hpp"
#include "SourcePosition.hpp"

struct ConfigToken {
    ConfigTokenType   type;
    std::string value;
    SourcePosition source_position;
    ConfigDirectiveType directive_type;

    ConfigToken(const std::string& value, ConfigTokenType type, SourcePosition position)
        : type(type), value(value), source_position(position), directive_type(UNKNOWN) {}
};