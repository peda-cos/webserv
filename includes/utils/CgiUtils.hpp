#ifndef CGI_UTILS_HPP
#define CGI_UTILS_HPP

#include <string>
#include <LocationConfig.hpp>

class CgiUtils {
    public:
        static std::string env_normalize(const std::string& str);
        static bool starts_with(const std::string& value, const std::string& prefix);
        static std::string join_paths(const std::string& left, const std::string& right);
        static std::string resolve_script_path(const std::string& request_script_path,
            const LocationConfig& location_config);
        static std::string absolute_path(const std::string& path);
};

#endif