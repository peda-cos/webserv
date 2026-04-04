#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <map>
#include <string>
#include <utils/Enums.hpp>

class HttpRequest {
    public:
        std::string method;
        std::string uri;
        std::string path;
        std::string queryString;
        std::string httpVersion;
        std::string client_ip;
        std::map<std::string, std::string> headers;
        std::string body;
        int errorCode;

        HttpRequest();
        HttpRequest& setBody(std::string body);
        HttpRequest& setMethod(std::string method);
        HttpRequest& setMethod(HttpMethod method);
        HttpRequest& setUri(std::string uri);
        HttpRequest& setUriPath(std::string path);
        HttpRequest& setPath(std::string path);
        HttpRequest& setQueryString(std::string queryString);
        HttpRequest& setHttpVersion(std::string httpVersion);
        HttpRequest& setVersion(std::string httpVersion);
        HttpRequest& setClientIp(std::string client_ip);
        HttpRequest& addHeader(std::string key, std::string value);
        HttpRequest& addQueryParameter(std::string key, std::string value);
        void setErrorCode(int code);
};

#endif
