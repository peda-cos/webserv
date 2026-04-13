#include <CgiUtils.hpp>

#include <unistd.h>

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

bool CgiUtils::starts_with(const std::string& value, const std::string& prefix) {
    return value.length() >= prefix.length() && value.compare(0, prefix.length(), prefix) == 0;
}

std::string CgiUtils::join_paths(const std::string& left, const std::string& right) {
    if (left.empty()) return right;
    if (right.empty()) return left;

    if (left[left.length() - 1] == '/' && right[0] == '/') {
        return left + right.substr(1);
    }
    if (left[left.length() - 1] != '/' && right[0] != '/') {
        return left + "/" + right;
    }
    return left + right;
}

std::string CgiUtils::resolve_script_path(const std::string& request_script_path,
    const LocationConfig& location_config)
{
    std::string relative = request_script_path;
    if (!location_config.path.empty() && starts_with(request_script_path, location_config.path)) {
        relative = request_script_path.substr(location_config.path.length());
        if (relative.empty()) {
            relative = "/";
        }
        if (relative[0] != '/') {
            relative = "/" + relative;
        }
    }
    return join_paths(location_config.root, relative);
}

std::string CgiUtils::absolute_path(const std::string& path) {
    if (!path.empty() && path[0] == '/') {
        return path;
    }

    char cwd_buffer[4096];
    if (getcwd(cwd_buffer, sizeof(cwd_buffer)) == NULL) {
        return path;
    }

    return join_paths(std::string(cwd_buffer), path);
}