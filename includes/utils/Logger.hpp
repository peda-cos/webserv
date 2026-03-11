#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include "Constants.hpp"

class Logger {
    public:
        static void info(const std::string& message);
        static void warn(const std::string& message);
        static void debug(const std::string& message);
        static void error(const std::string& message);
};

#endif