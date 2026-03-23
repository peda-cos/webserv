#include <HttpRequest.hpp>

HttpRequest& HttpRequest::setBody(std::string body) {
    this->body = body;
    return *this;
}

HttpRequest& HttpRequest::setUriPath(std::string uri_path) {
    this->uri_path = uri_path;
    return *this;
}

HttpRequest& HttpRequest::setMethod(HttpMethod method) {
    this->method = method;
    return *this;
}

HttpRequest& HttpRequest::setVersion(std::string version) {
    this->version = version;
    return *this;
}

HttpRequest& HttpRequest::addHeader(std::string key, std::string value) {
    this->headers[key] = value;
    return *this;
}

HttpRequest& HttpRequest::addQueryParameter(std::string key, std::string value) {
    this->query_parameters[key] = value;
    return *this;
}