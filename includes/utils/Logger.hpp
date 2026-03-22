#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include "Constants.hpp"

namespace Logger {
    void info(const std::string& message);
    void warn(const std::string& message);
    void debug(const std::string& message);
    void error(const std::string& message);
}

#endif