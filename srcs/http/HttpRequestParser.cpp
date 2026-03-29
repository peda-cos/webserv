#include <HttpRequestParser.hpp>
#include <algorithm>

HttpRequestParser::HttpRequestParser()
    : _buffer()
    , _complete(false)
    , _maxBodySize(0)
    , _request()
{}

HttpRequestParser::~HttpRequestParser() {}

void HttpRequestParser::feed(const std::string& data) {
    if (_complete) {
        return;
    }

    _buffer += data;

    // Check for end of headers marker \r\n\r\n
    std::size_t endPos = _buffer.find("\r\n\r\n");
    if (endPos != std::string::npos) {
        _complete = true;
        _parseRequestLine();
    }
}

bool HttpRequestParser::isComplete() const {
    return _complete;
}

HttpRequest HttpRequestParser::getRequest() const {
    return _request;
}

void HttpRequestParser::setMaxBodySize(std::size_t size) {
    _maxBodySize = size;
}

void HttpRequestParser::_parseRequestLine() {
    // Find the first line (up to first \r\n)
    std::size_t lineEnd = _buffer.find("\r\n");
    if (lineEnd == std::string::npos) {
        _request.setErrorCode(400);
        return;
    }

    std::string requestLine = _buffer.substr(0, lineEnd);

    // Check for empty request line
    if (requestLine.empty()) {
        _request.setErrorCode(400);
        return;
    }

    // Check for consecutive spaces (extra whitespace)
    if (requestLine.find("  ") != std::string::npos) {
        _request.setErrorCode(400);
        return;
    }

    // Find first space (after method)
    std::size_t firstSpace = requestLine.find(' ');
    if (firstSpace == std::string::npos) {
        _request.setErrorCode(400);
        return;
    }

    // Find last space (before HTTP version)
    std::size_t lastSpace = requestLine.rfind(' ');
    if (lastSpace == std::string::npos || lastSpace == firstSpace) {
        _request.setErrorCode(400);
        return;
    }

    // Extract method (everything before first space)
    std::string method = requestLine.substr(0, firstSpace);
    if (method.empty()) {
        _request.setErrorCode(400);
        return;
    }

    // Extract URI (everything between first and last space)
    std::string uri = requestLine.substr(firstSpace + 1, lastSpace - firstSpace - 1);
    if (uri.empty()) {
        _request.setErrorCode(400);
        return;
    }

    // Extract HTTP version token (everything after last space)
    std::string versionToken = requestLine.substr(lastSpace + 1);
    if (versionToken.empty()) {
        _request.setErrorCode(400);
        return;
    }

    // Validate HTTP/ prefix and extract version number
    if (versionToken.length() < 6 || versionToken.substr(0, 5) != "HTTP/") {
        _request.setErrorCode(400);
        return;
    }

    std::string httpVersion = versionToken.substr(5);
    if (httpVersion.empty()) {
        _request.setErrorCode(400);
        return;
    }

    // All validations passed - set the fields
    _request.setMethod(method);
    _request.setUri(uri);
    _request.setHttpVersion(httpVersion);
    _request.setErrorCode(0);
}
