#include <RequestRouter.hpp>
#include <CgiUtils.hpp>
#include <HttpUtils.hpp>
#include <Logger.hpp>

#include <sys/stat.h>
#include <cstdlib>

const LocationConfig* RequestRouter::matchLocation(const ServerConfig& server, const std::string& uriPath) {
    const LocationConfig* best_match = NULL;
    size_t best_match_length = 0;

    for (size_t i = 0; i < server.locations.size(); ++i) {
        const std::string& loc_path = server.locations[i].path;

        if (CgiUtils::starts_with(uriPath, loc_path) && loc_path.length() > best_match_length) {
            best_match = &(server.locations[i]);
            best_match_length = loc_path.length();
        }
    }

    return best_match;
}

bool RequestRouter::isMethodAllowed(const std::string& method, const LocationConfig& loc) {
    if (loc.limit_except.empty()) {
        return true;
    }

    for (size_t i = 0; i < loc.limit_except.size(); ++i) {
        if (HttpUtils::method_to_string(loc.limit_except[i]) == method) {
            return true;
        }
    }

    return false;
}

std::string RequestRouter::resolvePhysicalPath(const LocationConfig& loc, const std::string& uriPath, const ServerConfig& server) {
    std::string root = loc.root.empty() ? server.root : loc.root;
    if (root.empty()) {
        root = "www";
    }

    std::string relative = uriPath;
    if (!loc.path.empty() && CgiUtils::starts_with(uriPath, loc.path)) {
        relative = uriPath.substr(loc.path.length());
        if (relative.empty()) {
            relative = "/";
        }
        if (relative[0] != '/') {
            relative = "/" + relative;
        }
    }

    return CgiUtils::join_paths(root, relative);
}

bool RequestRouter::isPathTraversal(const std::string& path) {
    std::string normalized;
    size_t start = 0;

    while (start < path.length()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) {
            end = path.length();
        }

        std::string component = path.substr(start, end - start);
        if (component == "..") {
            return true;
        }

        start = end + 1;
    }

    return false;
}

bool RequestRouter::isCgiRequest(const std::string& physicalPath, const LocationConfig& loc) {
    std::string ext;
    size_t slash_pos = 0;
    return CgiUtils::extract_extension(physicalPath, ext, slash_pos) &&
           loc.cgi_handlers.find(ext) != loc.cgi_handlers.end();
}

RequestRouter::TargetType RequestRouter::classifyRequest(const std::string& physicalPath, const LocationConfig& loc) {
    if (isCgiRequest(physicalPath, loc)) {
        return TARGET_CGI;
    }

    struct stat st;
    if (stat(physicalPath.c_str(), &st) != 0) {
        return TARGET_NOT_FOUND;
    }

    if (S_ISDIR(st.st_mode)) {
        return TARGET_DIRECTORY;
    }

    if (S_ISREG(st.st_mode)) {
        return TARGET_FILE;
    }

    return TARGET_NOT_FOUND;
}
