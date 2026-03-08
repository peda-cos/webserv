#pragma once

#include <string>
#include <vector>
#include "enums.hpp"
#include "LocationConfig.hpp"

struct ServerConfig {
    std::string host;
    std::string port;
    std::size_t client_max_body_size;
    AutoIndex autoindex;
    std::vector<HttpMethod> limit_except;
    std::vector<LocationConfig> locations;
    // Fallback/Default values
    std::string root;
    std::vector<std::string> index;
};