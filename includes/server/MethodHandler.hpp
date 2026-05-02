#ifndef METHOD_HANDLER_HPP
#define METHOD_HANDLER_HPP

#include <string>
#include <map>
#include <LocationConfig.hpp>
#include <ServerConfig.hpp>

class MethodHandler {
public:
    struct Result {
        int statusCode;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    static Result handleGet(const std::string& path, const LocationConfig& loc, const ServerConfig& server);
    static Result handlePost(const std::string& path, const std::string& body, const LocationConfig& loc, const ServerConfig& server);
    static Result handleDelete(const std::string& path, const LocationConfig& loc, const ServerConfig& server);
    static bool isMethodAllowed(const std::string& method, const LocationConfig& loc);
};

#endif
