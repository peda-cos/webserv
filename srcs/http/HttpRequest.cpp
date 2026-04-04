#include <HttpRequest.hpp>

static std::string httpMethodToString(HttpMethod method) {
    switch (method) {
        case GET:
            return "GET";
        case POST:
            return "POST";
        case DELETE:
            return "DELETE";
        case PUT:
            return "PUT";
        case HEAD:
            return "HEAD";
        case CONNECT:
            return "CONNECT";
        case OPTIONS:
            return "OPTIONS";
        case TRACE:
            return "TRACE";
        case PATCH:
            return "PATCH";
        default:
            return "";
    }
}

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

HttpRequest& HttpRequest::setMethod(HttpMethod method) {
    return setMethod(httpMethodToString(method));
}

HttpRequest& HttpRequest::setPath(std::string path) {
    this->path = path;
    return *this;
}

HttpRequest& HttpRequest::setUriPath(std::string path) {
    this->uri = path;
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

HttpRequest& HttpRequest::setVersion(std::string httpVersion) {
    if (httpVersion.length() > 5 && httpVersion.substr(0, 5) == "HTTP/") {
        return setHttpVersion(httpVersion.substr(5));
    }
    return setHttpVersion(httpVersion);
}

HttpRequest& HttpRequest::addHeader(std::string key, std::string value) {
    this->headers[key] = value;
    return *this;
}

HttpRequest& HttpRequest::addQueryParameter(std::string key, std::string value) {
    if (!this->queryString.empty()) {
        this->queryString += "&";
    }
    this->queryString += key + "=" + value;
    if (!this->path.empty()) {
        this->uri = this->path + "?" + this->queryString;
    }
    return *this;
}

void HttpRequest::setErrorCode(int code) {
    this->errorCode = code;
}
