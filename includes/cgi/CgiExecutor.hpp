#ifndef CGI_EXECUTOR_HPP
#define CGI_EXECUTOR_HPP

#include <sys/types.h>
#include <CgiResult.hpp>
#include <HttpRequest.hpp>
#include <LocationConfig.hpp>

class CgiExecutor {
    private:
        pid_t pid;
    public:
        CgiResult execute(const HttpRequest& request, const LocationConfig& location_config) const;
};

#endif