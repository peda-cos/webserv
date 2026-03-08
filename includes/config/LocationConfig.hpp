#pragma once

#include <string>
#include <vector>
#include "enums.hpp"

struct LocationConfig {
    std::string path;
    std::string root;
    std::string index;
    AutoIndex autoindex;
    std::vector<HttpMethod> limit_except;
    std::string upload_store;
    std::string cgi_pass;
    std::string return_url;
    int return_code;
};