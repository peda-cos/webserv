#pragma once

#include <string.h>

struct SourcePosition {
    size_t line;
    size_t column;

    SourcePosition() : line(1), column(1) {}
};