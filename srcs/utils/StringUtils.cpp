#include <StringUtils.hpp>

#include <sstream>

std::string StringUtils::to_string(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::string StringUtils::to_string(size_t value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::string StringUtils::to_string(double value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::string StringUtils::to_string(bool value) {
    return value ? "true" : "false";
}