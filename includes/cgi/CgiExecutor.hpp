#ifndef CGI_EXECUTOR_HPP
#define CGI_EXECUTOR_HPP

#include <sys/types.h>
#include <HttpRequest.hpp>
#include <LocationConfig.hpp>
#include <ServerConfig.hpp>

struct CgiProcessInfo {
    pid_t pid;
    int   stdin_fd;
    int   stdout_fd;
};

class CgiExecutor {
    public:
        CgiProcessInfo start_cgi(const HttpRequest& request, const LocationConfig& location_config,
                                 const ServerConfig& server_config, int client_fd) const;
};

#endif