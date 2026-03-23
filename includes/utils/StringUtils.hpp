#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP

#include <string>

class StringUtils {
    public:
        static std::string to_string(int value);
        static std::string to_string(size_t value);
        static std::string to_string(double value);
        static std::string to_string(bool value);
};

#endif