#include <HttpRequestParser.hpp>
#include <algorithm>
#include <cctype>
#include <climits>
#include <map>
#include <sstream>

static const std::string CRLF = "\r\n";
static const std::string DOUBLE_CRLF = "\r\n\r\n";
static const std::string HTTP_VERSION_PREFIX = "HTTP/";
static const std::string HEADER_HOST = "host";
static const std::string HEADER_TRANSFER_ENCODING = "transfer-encoding";
static const std::string HEADER_CONTENT_LENGTH = "content-length";

HttpRequestParser::HttpRequestParser()
    : _buffer()
    , _state(REQUEST_LINE)
    , _maxBodySize(0)
    , _request()
    , _contentLength(0)
    , _bodyBuffer()
    , _remainder()
{
    _chunkedDecoder.setMaxBodySize(_maxBodySize);
}

HttpRequestParser::~HttpRequestParser() {}

void HttpRequestParser::reset() {
    _buffer.clear();
    _state = REQUEST_LINE;
    _request = HttpRequest();
    _contentLength = 0;
    _bodyBuffer.clear();
    _chunkedDecoder = ChunkedDecoder();
    _chunkedDecoder.setMaxBodySize(_maxBodySize);
    _remainder.clear();
}

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
        std::size_t endPos = _buffer.find(DOUBLE_CRLF);
        if (endPos != std::string::npos) {
            _state = HEADERS;
            _parseRequestLine();
            if (_request.errorCode == 0) {
                _parseHeaders();
                if (_request.errorCode == 0) {
                    std::size_t bodyStart = endPos + DOUBLE_CRLF.size();
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
        _feedChunkedDecoder(data);
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
    _chunkedDecoder.setMaxBodySize(size);
}

void HttpRequestParser::_parseRequestLine() {
    std::size_t lineEnd = _buffer.find(CRLF);
    if (lineEnd == std::string::npos) {
        _request.setErrorCode(400);
        return;
    }

    std::string requestLine = _buffer.substr(0, lineEnd);

    if (requestLine.empty()) {
        _request.setErrorCode(400);
        return;
    }

    // Request line must be exactly METHOD SP URI SP HTTP/version
    if (requestLine.find('\t') != std::string::npos ||
        requestLine[0] == ' ' ||
        requestLine[requestLine.length() - 1] == ' ' ||
        requestLine.find("  ") != std::string::npos) {
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
    if (versionToken.length() <= HTTP_VERSION_PREFIX.length() ||
        versionToken.substr(0, HTTP_VERSION_PREFIX.length()) != HTTP_VERSION_PREFIX) {
        _request.setErrorCode(400);
        return;
    }

    std::string httpVersion = versionToken.substr(HTTP_VERSION_PREFIX.length());
    if (!_isSupportedHttpVersion(httpVersion)) {
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
    std::size_t pos = _buffer.find(CRLF);
    if (pos == std::string::npos) {
        _request.setErrorCode(400);
        return;
    }
    pos += CRLF.size();

    std::size_t endPos = _buffer.find(DOUBLE_CRLF);
    if (endPos == std::string::npos) {
        _request.setErrorCode(400);
        return;
    }
    std::size_t headerEnd = endPos + CRLF.size();

    // Parse each header line
    while (pos < headerEnd) {
        // Find end of this header line
        std::size_t lineEnd = _buffer.find(CRLF, pos);
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

        // Extract field name: validate characters and convert to lowercase in a single pass
        std::string fieldName = headerLine.substr(0, colonPos);
        for (std::size_t i = 0; i < fieldName.length(); ++i) {
            unsigned char ch = static_cast<unsigned char>(fieldName[i]);
            if (std::iscntrl(ch) || ch == ' ' || ch == '\t') {
                _request.setErrorCode(400);
                return;
            }
            fieldName[i] = static_cast<char>(std::tolower(ch));
        }

        // Extract field value (after colon) and trim OWS (optional whitespace)
        std::string fieldValue;
        if (colonPos + 1 < headerLine.length()) {
            fieldValue = _trimOws(headerLine.substr(colonPos + 1));
        }

        // Handle duplicate headers: concatenate with ", "
        std::map<std::string, std::string>::iterator existing = _request.headers.find(fieldName);
        if (existing != _request.headers.end()) {
            existing->second += ", " + fieldValue;
        } else {
            _request.addHeader(fieldName, fieldValue);
        }

        // Move to next header line
        pos = lineEnd + CRLF.size();
    }

    // Validate Host header for HTTP/1.1
    if (_request.httpVersion == "1.1") {
        std::map<std::string, std::string>::iterator hostIt = _request.headers.find(HEADER_HOST);
        if (hostIt == _request.headers.end() || hostIt->second.empty() || hostIt->second.find(',') != std::string::npos) {
            _request.setErrorCode(400);
            return;
        }
    }

    std::map<std::string, std::string>::iterator transferEncodingIt = _request.headers.find(HEADER_TRANSFER_ENCODING);
    if (transferEncodingIt != _request.headers.end() && !_isChunkedTransferEncoding(transferEncodingIt->second)) {
        _request.setErrorCode(400);
        return;
    }

    std::map<std::string, std::string>::iterator contentLengthIt = _request.headers.find(HEADER_CONTENT_LENGTH);
    if (contentLengthIt != _request.headers.end() && _hasConflictingContentLength(contentLengthIt->second)) {
        _request.setErrorCode(400);
    }
}

void HttpRequestParser::_initiateBodyReading(const std::string& afterHeaders) {
    std::map<std::string, std::string>::iterator transferEncodingIt = _request.headers.find(HEADER_TRANSFER_ENCODING);
    if (transferEncodingIt != _request.headers.end()) {
        if (_isChunkedTransferEncoding(transferEncodingIt->second)) {
            if (!afterHeaders.empty()) {
                _feedChunkedDecoder(afterHeaders);
                if (_state == COMPLETE || _state == ERROR) {
                    return;
                }
            }
            _state = CHUNKED_BODY;
            return;
        }

        _request.setErrorCode(400);
        _state = ERROR;
        return;
    }

    // Check for content-length header
    std::map<std::string, std::string>::iterator contentLengthIt = _request.headers.find(HEADER_CONTENT_LENGTH);
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

void HttpRequestParser::_feedChunkedDecoder(const std::string& data) {
    try {
        _chunkedDecoder.feed(data);
        if (_chunkedDecoder.isComplete()) {
            _finalizeChunkedBody();
        }
    } catch (const ChunkedBodyTooLargeException&) {
        _request.setErrorCode(413);
        _state = ERROR;
    } catch (const ChunkedDecodeException&) {
        _request.setErrorCode(400);
        _state = ERROR;
    }
}

void HttpRequestParser::_finalizeChunkedBody() {
    _request.setBody(_chunkedDecoder.getBody());
    std::ostringstream oss;
    oss << _request.body.size();
    _request.headers[HEADER_CONTENT_LENGTH] = oss.str();
    _remainder = _chunkedDecoder.getRemainder();
    _state = COMPLETE;
}

void HttpRequestParser::_parseContentLength() {
    std::map<std::string, std::string>::iterator it = _request.headers.find(HEADER_CONTENT_LENGTH);
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

bool HttpRequestParser::_isSupportedHttpVersion(const std::string& version) const {
    return version == "1.1" || version == "1.0";
}

std::string HttpRequestParser::_trimOws(const std::string& str) {
    std::size_t start = 0;
    while (start < str.length() && (str[start] == ' ' || str[start] == '\t')) {
        ++start;
    }
    std::size_t end = str.length();
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t')) {
        --end;
    }
    return str.substr(start, end - start);
}

bool HttpRequestParser::_isChunkedTransferEncoding(const std::string& value) const {
    std::size_t start = 0;

    while (start < value.length()) {
        std::size_t end = value.find(',', start);
        std::string raw = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::string token = _trimOws(raw);

        for (std::size_t i = 0; i < token.length(); ++i) {
            token[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(token[i])));
        }

        if (token.empty()) {
            return false;
        }
        if (token == "chunked") {
            return end == std::string::npos || end + 1 >= value.length();
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    return false;
}

bool HttpRequestParser::_hasConflictingContentLength(const std::string& value) {
    std::size_t start = 0;
    std::string canonical;

    while (start < value.length()) {
        std::size_t end = value.find(',', start);
        std::string raw = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::string token = _trimOws(raw);

        if (token.empty()) {
            return true;
        }

        for (std::size_t i = 0; i < token.length(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(token[i]))) {
                return true;
            }
        }

        if (canonical.empty()) {
            canonical = token;
        } else if (canonical != token) {
            return true;
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    if (!canonical.empty()) {
        _request.headers[HEADER_CONTENT_LENGTH] = canonical;
    }
    return false;
}
