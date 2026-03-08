#pragma once

#include <string>
#include "enums.hpp"
#include "SourcePosition.hpp"

struct ConfigToken {
    ConfigTokenType   type;
    std::string value;
    SourcePosition sourcePosition;

    ConfigToken(const std::string& value, ConfigTokenType type, SourcePosition position)
        : type(type), value(value), sourcePosition(position) {}
};