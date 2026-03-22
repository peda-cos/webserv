#ifndef CGI_EXECUTOR_HPP
#define CGI_EXECUTOR_HPP

#include <string>
#include <sys/types.h>
#include <HttpRequest.hpp>
#include <LocationConfig.hpp>

class CgiExecutor {
    private:
        pid_t pid;
    public:
        std::string execute(const HttpRequest& request, LocationConfig& location_config);
};

#endif