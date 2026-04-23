#include <MethodHandler.hpp>
#include <RequestRouter.hpp>
#include <CgiUtils.hpp>
#include <StringUtils.hpp>
#include <MimeTypes.hpp>

#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>

static std::string generate_directory_listing(const std::string& physicalPath) {
    DIR* dir = opendir(physicalPath.c_str());
    if (!dir) {
        return "";
    }

    std::ostringstream html;
    html << "<html><head><title>Index of " << physicalPath << "</title></head><body>\n";
    html << "<h1>Index of " << physicalPath << "</h1><hr><pre>\n";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }

        std::string full_path = physicalPath + "/" + name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            name += "/";
        }

        html << "<a href=\"" << name << "\">" << name << "</a>\n";
    }

    closedir(dir);
    html << "</pre><hr></body></html>";
    return html.str();
}

bool MethodHandler::isMethodAllowed(const std::string& method, const LocationConfig& loc) {
    return RequestRouter::isMethodAllowed(method, loc);
}

MethodHandler::Result MethodHandler::handleGet(const std::string& path, const LocationConfig& loc, const ServerConfig& server) {
    Result res;
    res.statusCode = 404;

    std::string physicalPath = RequestRouter::resolvePhysicalPath(loc, path, server);
    if (RequestRouter::isPathTraversal(physicalPath)) {
        res.statusCode = 403;
        return res;
    }

    struct stat st;
    if (stat(physicalPath.c_str(), &st) != 0) {
        res.statusCode = 404;
        return res;
    }

    if (S_ISDIR(st.st_mode)) {
        if (!loc.index.empty()) {
            std::string indexPath = CgiUtils::join_paths(physicalPath, loc.index);
            std::ifstream indexFile(indexPath.c_str(), std::ios::binary);
            if (indexFile.is_open()) {
                std::ostringstream ss;
                ss << indexFile.rdbuf();
                res.statusCode = 200;
                res.body = ss.str();
                return res;
            }
        }

        if (loc.autoindex == ON) {
            std::string listing = generate_directory_listing(physicalPath);
            if (!listing.empty()) {
                res.statusCode = 200;
                res.body = listing;
                return res;
            }
        }

        res.statusCode = 403;
        return res;
    }

    if (S_ISREG(st.st_mode)) {
        std::ifstream file(physicalPath.c_str(), std::ios::binary);
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            res.statusCode = 200;
            res.body = ss.str();
            return res;
        }
    }

    res.statusCode = 404;
    return res;
}

MethodHandler::Result MethodHandler::handlePost(const std::string& path, const std::string& body, const LocationConfig& loc, const ServerConfig& server) {
    (void)path;
    (void)server;
    Result res;
    res.statusCode = 403;

    if (loc.upload_store.empty()) {
        res.statusCode = 403;
        return res;
    }

    struct stat st;
    if (stat(loc.upload_store.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        res.statusCode = 500;
        return res;
    }

    std::string filename = "upload_" + StringUtils::to_string(static_cast<int>(time(NULL))) + ".txt";
    std::string uploadPath = CgiUtils::join_paths(loc.upload_store, filename);

    std::ofstream out(uploadPath.c_str(), std::ios::binary);
    if (!out.is_open()) {
        res.statusCode = 500;
        return res;
    }
    out.write(body.c_str(), static_cast<std::streamsize>(body.length()));
    out.close();

    res.statusCode = 201;
    res.body = "Created";
    return res;
}

MethodHandler::Result MethodHandler::handleDelete(const std::string& path, const LocationConfig& loc, const ServerConfig& server) {
    Result res;
    res.statusCode = 404;

    std::string physicalPath = RequestRouter::resolvePhysicalPath(loc, path, server);
    if (RequestRouter::isPathTraversal(physicalPath)) {
        res.statusCode = 403;
        return res;
    }

    if (!isMethodAllowed("DELETE", loc)) {
        res.statusCode = 405;
        return res;
    }

    struct stat st;
    if (stat(physicalPath.c_str(), &st) != 0) {
        res.statusCode = 404;
        return res;
    }

    if (!S_ISREG(st.st_mode)) {
        res.statusCode = 404;
        return res;
    }

    if (std::remove(physicalPath.c_str()) != 0) {
        res.statusCode = 500;
        return res;
    }

    res.statusCode = 204;
    return res;
}
