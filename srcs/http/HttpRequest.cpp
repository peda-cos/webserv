#include <HttpRequest.hpp>

HttpRequest::HttpRequest()
    : method()
    , uri()
    , path()
    , queryString()
    , httpVersion()
    , headers()
    , body()
    , errorCode(0)
{}

HttpRequest& HttpRequest::setBody(std::string body) {
    this->body = body;
    return *this;
}

HttpRequest& HttpRequest::setUri(std::string uri) {
    this->uri = uri;
    return *this;
}

HttpRequest& HttpRequest::setMethod(std::string method) {
    this->method = method;
    return *this;
}

HttpRequest& HttpRequest::setPath(std::string path) {
    this->path = path;
    return *this;
}

HttpRequest& HttpRequest::setQueryString(std::string queryString) {
    this->queryString = queryString;
    return *this;
}

HttpRequest& HttpRequest::setHttpVersion(std::string httpVersion) {
    this->httpVersion = httpVersion;
    return *this;
}

HttpRequest& HttpRequest::addHeader(std::string key, std::string value) {
    this->headers[key] = value;
    return *this;
}

void HttpRequest::setErrorCode(int code) {
    this->errorCode = code;
}