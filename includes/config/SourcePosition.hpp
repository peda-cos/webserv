#ifndef SOURCE_POSITION_HPP
#define SOURCE_POSITION_HPP

#include <string.h>

struct SourcePosition {
    size_t line;
    size_t column;

    SourcePosition() : line(1), column(1) {}
};

#endif