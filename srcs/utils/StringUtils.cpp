#include <StringUtils.hpp>

#include <sstream>
#include <cctype>

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

std::string StringUtils::to_lower(const std::string& value) {
    std::string lower = value;
    for (size_t i = 0; i < lower.length(); ++i) {
        lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
    }
    return lower;
}

std::string StringUtils::trim_left(const std::string& value) {
    std::string::size_type first = value.find_first_not_of(" \t");
    return (first == std::string::npos) ? "" : value.substr(first);
}