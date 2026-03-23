#include <CgiUtils.hpp>

std::string CgiUtils::env_normalize(const std::string& str) {
    std::string normalized = str;
    size_t size = normalized.size();
    for (size_t i = 0; i < size; ++i) {
        if (normalized[i] == '-') {
            normalized[i] = '_';
        } else {
            normalized[i] = toupper(normalized[i]);
        }
    }
    return normalized;
}