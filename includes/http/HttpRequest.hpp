#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <map>
#include <string>
#include <Enums.hpp>

class HttpRequest {
    public:
        std::string body;
        HttpMethod method;
        std::string version;
        std::string uri_path;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> query_parameters;

        HttpRequest& setBody(std::string body);
        HttpRequest& setMethod(HttpMethod method);
        HttpRequest& setUriPath(std::string uri_path);
        HttpRequest& setVersion(std::string version);
        HttpRequest& addHeader(std::string key, std::string value);
        HttpRequest& addQueryParameter(std::string key, std::string value);
};

#endif