#include <HttpRequestParser.hpp>
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>

HttpRequestParser::HttpRequestParser()
    : _buffer()
    , _state(REQUEST_LINE)
    , _maxBodySize(0)
    , _request()
    , _contentLength(0)
    , _bodyBuffer()
    , _remainder()
{}

HttpRequestParser::~HttpRequestParser() {}

void HttpRequestParser::feed(const std::string& data) {
    if (_state == COMPLETE || _state == ERROR) {
        return;
    }

    // Only append to _buffer during header parsing phase
    if (_state == REQUEST_LINE || _state == HEADERS) {
        _buffer += data;
    }

    // Process based on current state
    if (_state == REQUEST_LINE || _state == HEADERS) {
        // Check for end of headers marker \r\n\r\n
        std::size_t endPos = _buffer.find("\r\n\r\n");
        if (endPos != std::string::npos) {
            _state = HEADERS;
            _parseRequestLine();
            if (_request.errorCode == 0) {
                _parseHeaders();
                if (_request.errorCode == 0) {
                    // Capture any data after headers before _buffer is cleared
                    std::size_t bodyStart = endPos + 4;
                    std::string afterHeaders;
                    if (bodyStart < _buffer.size()) {
                        afterHeaders = _buffer.substr(bodyStart);
                    }
                    _initiateBodyReading(afterHeaders);
                    // Clear _buffer after successful header parsing to prevent
                    // body duplication on subsequent feed() calls
                    if (_state != ERROR) {
                        _buffer.clear();
                    }
                } else {
                    _state = ERROR;
                }
            } else {
                _state = ERROR;
            }
        }
    } else if (_state == BODY) {
        // Accumulate body data
        _bodyBuffer += data;
        if (_bodyBuffer.size() >= _contentLength) {
            _request.setBody(_bodyBuffer.substr(0, _contentLength));
            // Capture remainder (excess bytes beyond Content-Length)
            _remainder = _bodyBuffer.substr(_contentLength);
            _state = COMPLETE;
        }
    } else if (_state == CHUNKED_BODY) {
        // Delegate to chunked decoder
        try {
            _chunkedDecoder.feed(data);

            // Check max body size against accumulated body
            if (_maxBodySize > 0 && _chunkedDecoder.getBody().size() > _maxBodySize) {
                _request.setErrorCode(413);
                _state = ERROR;
                return;
            }

            // Check if complete
            if (_chunkedDecoder.isComplete()) {
                _request.setBody(_chunkedDecoder.getBody());
                // Update content-length header for downstream handlers
                std::ostringstream oss;
                oss << _request.body.size();
                _request.headers["content-length"] = oss.str();
                // Capture remainder from chunked decoder
                _remainder = _chunkedDecoder.getRemainder();
                _state = COMPLETE;
            }
        } catch (const ChunkedDecodeException&) {
            _request.setErrorCode(400);
            _state = ERROR;
        }
    }
}

bool HttpRequestParser::isComplete() const {
    return _state == COMPLETE || _state == ERROR;
}

HttpRequest HttpRequestParser::getRequest() const {
    return _request;
}

std::string HttpRequestParser::getRemainder() const {
    return _remainder;
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

    // Split URI into path and query string
    std::size_t queryPos = uri.find('?');
    if (queryPos != std::string::npos) {
        _request.setPath(uri.substr(0, queryPos));
        _request.setQueryString(uri.substr(queryPos + 1));
    } else {
        _request.setPath(uri);
    }

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

void HttpRequestParser::_initiateBodyReading(const std::string& afterHeaders) {
    // Check for Transfer-Encoding: chunked first (RFC 2616 §4.4)
    std::map<std::string, std::string>::iterator transferEncodingIt = _request.headers.find("transfer-encoding");
    if (transferEncodingIt != _request.headers.end()) {
        // Chunked encoding takes precedence
        if (transferEncodingIt->second == "chunked") {
            // Feed any data already after headers to the chunked decoder
            if (!afterHeaders.empty()) {
                try {
                    _chunkedDecoder.feed(afterHeaders);
                    
                    // If we got the complete body in one go
                    if (_chunkedDecoder.isComplete()) {
                        // Check max body size before accepting
                        if (_maxBodySize > 0 && _chunkedDecoder.getBody().size() > _maxBodySize) {
                            _request.setErrorCode(413);
                            _state = ERROR;
                            return;
                        }
                        _request.setBody(_chunkedDecoder.getBody());
                        _request.headers["content-length"] = _request.body;
                        std::ostringstream oss;
                        oss << _request.body.size();
                        _request.headers["content-length"] = oss.str();
                        // Capture remainder from chunked decoder
                        _remainder = _chunkedDecoder.getRemainder();
                        _state = COMPLETE;
                        return;
                    }
                } catch (const ChunkedDecodeException&) {
                    _request.setErrorCode(400);
                    _state = ERROR;
                    return;
                }
            }
            _state = CHUNKED_BODY;
            return;
        }
    }

    // Check for content-length header
    std::map<std::string, std::string>::iterator contentLengthIt = _request.headers.find("content-length");
    if (contentLengthIt == _request.headers.end()) {
        // No body expected - any afterHeaders is the remainder (pipelined request)
        _remainder = afterHeaders;
        _state = COMPLETE;
        return;
    }

    _parseContentLength();

    if (_state == ERROR) {
        return;
    }

    // Content-Length of 0 means complete immediately
    if (_contentLength == 0) {
        _remainder = afterHeaders;
        _state = COMPLETE;
        return;
    }

    // Use afterHeaders as initial body data
    _bodyBuffer = afterHeaders;

    // Check if we have enough body data already
    if (_bodyBuffer.size() >= _contentLength) {
        _request.setBody(_bodyBuffer.substr(0, _contentLength));
        // Capture remainder (excess bytes beyond Content-Length)
        _remainder = _bodyBuffer.substr(_contentLength);
        _state = COMPLETE;
    } else {
        _state = BODY;
    }
}

void HttpRequestParser::_parseContentLength() {
    std::map<std::string, std::string>::iterator it = _request.headers.find("content-length");
    if (it == _request.headers.end()) {
        _contentLength = 0;
        return;
    }

    const std::string& value = it->second;

    // Validate: must be digits only
    if (value.empty()) {
        _request.setErrorCode(400);
        _state = ERROR;
        return;
    }

    for (std::size_t i = 0; i < value.length(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(value[i]))) {
            _request.setErrorCode(400);
            _state = ERROR;
            return;
        }
    }

    // Parse the value
    _contentLength = 0;
    for (std::size_t i = 0; i < value.length(); ++i) {
        int digit = value[i] - '0';
        // Check for overflow
        if (_contentLength > (static_cast<std::size_t>(-1) - digit) / 10) {
            _request.setErrorCode(400);
            _state = ERROR;
            return;
        }
        _contentLength = _contentLength * 10 + digit;
    }

    // Check against max body size
    if (_maxBodySize > 0 && _contentLength > _maxBodySize) {
        _request.setErrorCode(413);
        _state = ERROR;
    }
}
