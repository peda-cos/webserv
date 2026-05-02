#ifndef REQUEST_ROUTER_HPP
#define REQUEST_ROUTER_HPP

#include <string>
#include <ServerConfig.hpp>
#include <LocationConfig.hpp>

class RequestRouter {
public:
    enum TargetType {
        TARGET_CGI,
        TARGET_DIRECTORY,
        TARGET_FILE,
        TARGET_NOT_FOUND
    };

    static const LocationConfig* matchLocation(const ServerConfig& server, const std::string& uriPath);
    static bool isMethodAllowed(const std::string& method, const LocationConfig& loc);
    static std::string resolvePhysicalPath(const LocationConfig& loc, const std::string& uriPath, const ServerConfig& server);
    static bool isPathTraversal(const std::string& path);
    static bool isCgiRequest(const std::string& physicalPath, const LocationConfig& loc);
    static TargetType classifyRequest(const std::string& physicalPath, const LocationConfig& loc);
};

#endif
