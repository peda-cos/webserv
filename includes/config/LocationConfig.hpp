#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <Enums.hpp>

struct LocationConfig {
    std::string modifier;
    std::string path;
    std::string root;
    std::string index;
    AutoIndex autoindex;
    std::vector<HttpMethod> limit_except;
    std::string upload_store;
    std::map<std::string, std::string> cgi_handlers;
    std::string return_url;
    int return_code;
    std::map<int, std::string> error_pages;
};

#endif