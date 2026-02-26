#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

struct Listen {
    std::string host;
    int port;
    Listen() : host("0.0.0.0"), port(80) {}
};

struct LocationConfig {
    std::string path;
    std::set<std::string> methods;
    bool autoindex;
    std::string root;
    std::string index;
    bool upload_enabled;
    std::string upload_dir;
    bool has_redirect;
    int redirect_code;
    std::string redirect_target;
    std::map<std::string, std::string> cgi_map;
    size_t client_max_body_size;
    static const size_t UNSET_SIZE = static_cast<size_t>(-1);

    LocationConfig()
        : autoindex(false), upload_enabled(false), has_redirect(false),
          redirect_code(0), client_max_body_size(UNSET_SIZE) {}
};

struct ServerConfig {
    std::vector<Listen> listens;
    std::string server_name;
    std::string root;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    std::vector<LocationConfig> locations;
    static const size_t DEFAULT_MAX_BODY_SIZE = 1048576; // 1MB

    ServerConfig() : client_max_body_size(DEFAULT_MAX_BODY_SIZE) {}
};

#endif
