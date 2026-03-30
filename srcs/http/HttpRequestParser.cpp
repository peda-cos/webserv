#include <HttpRequestParser.hpp>
#include <algorithm>
#include <cctype>
#include <map>

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
        if (_request.errorCode == 0) {
            _parseHeaders();
        }
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

void HttpRequestParser::_parseHeaders() {
    // Skip request line - find first \r\n
    std::size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos) {
        _request.setErrorCode(400);
        return;
    }
    pos += 2; // Move past \r\n

    // Find end of headers marker
    std::size_t endPos = _buffer.find("\r\n\r\n", pos);
    if (endPos == std::string::npos) {
        _request.setErrorCode(400);
        return;
    }

    // Parse each header line
    while (pos < endPos) {
        // Find end of this header line
        std::size_t lineEnd = _buffer.find("\r\n", pos);
        if (lineEnd == std::string::npos || lineEnd > endPos) {
            break;
        }

        // Extract the header line
        std::string headerLine = _buffer.substr(pos, lineEnd - pos);

        // Find the colon separator
        std::size_t colonPos = headerLine.find(':');
        if (colonPos == std::string::npos) {
            // No colon - malformed header
            _request.setErrorCode(400);
            return;
        }

        if (colonPos == 0) {
            // Empty field name
            _request.setErrorCode(400);
            return;
        }

        // Extract field name and convert to lowercase
        std::string fieldName = headerLine.substr(0, colonPos);
        for (std::size_t i = 0; i < fieldName.length(); ++i) {
            fieldName[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(fieldName[i])));
        }

        // Extract field value (after colon) and trim whitespace
        std::string fieldValue;
        if (colonPos + 1 < headerLine.length()) {
            fieldValue = headerLine.substr(colonPos + 1);

            // Trim leading whitespace (SP and HTAB)
            std::size_t valueStart = 0;
            while (valueStart < fieldValue.length() &&
                   (fieldValue[valueStart] == ' ' || fieldValue[valueStart] == '\t')) {
                ++valueStart;
            }

            // Trim trailing whitespace (SP and HTAB)
            std::size_t valueEnd = fieldValue.length();
            while (valueEnd > valueStart &&
                   (fieldValue[valueEnd - 1] == ' ' || fieldValue[valueEnd - 1] == '\t')) {
                --valueEnd;
            }

            fieldValue = fieldValue.substr(valueStart, valueEnd - valueStart);
        }

        // Handle duplicate headers: concatenate with ", "
        std::map<std::string, std::string>::iterator existing = _request.headers.find(fieldName);
        if (existing != _request.headers.end()) {
            existing->second += ", " + fieldValue;
        } else {
            _request.addHeader(fieldName, fieldValue);
        }

        // Move to next header line
        pos = lineEnd + 2; // Move past \r\n
    }

    // Validate Host header for HTTP/1.1
    if (_request.httpVersion == "1.1") {
        if (_request.headers.find("host") == _request.headers.end()) {
            _request.setErrorCode(400);
        }
    }
}
