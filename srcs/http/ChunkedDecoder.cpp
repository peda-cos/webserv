#include <ChunkedDecoder.hpp>

ChunkedDecoder::ChunkedDecoder()
    : _state(READING_SIZE),
      _currentChunkSize(0),
      _bytesRead(0),
      _error(false) {}

ChunkedDecoder::~ChunkedDecoder() {}

void ChunkedDecoder::feed(const std::string& data) {
    if (_error || _state == DONE) {
        return;
    }

    _buffer += data;
    _processBuffer();
}

bool ChunkedDecoder::isComplete() const {
    return _state == DONE;
}

std::string ChunkedDecoder::getBody() const {
    return _body;
}

bool ChunkedDecoder::hasError() const {
    return _error;
}

std::size_t ChunkedDecoder::_parseHexSize(const std::string& hex) const {
    std::size_t size = 0;

    for (std::size_t i = 0; i < hex.length(); ++i) {
        char c = hex[i];
        int digit = 0;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            digit = 10 + (c - 'A');
        } else {
            throw ChunkedDecodeException("Invalid hex character in chunk size");
        }

        // Check for overflow before multiplying
        if (size > (static_cast<std::size_t>(-1) - digit) / 16) {
            throw ChunkedDecodeException("Chunk size overflow");
        }

        size = size * 16 + digit;
    }

    return size;
}

void ChunkedDecoder::_processBuffer() {
    bool progress = true;

    while (progress && !_error && _state != DONE && !_buffer.empty()) {
        progress = false;

        switch (_state) {
            case READING_SIZE:
                _processReadingSize();
                progress = true; // Always make progress in state machine loop
                break;

            case READING_DATA:
                _processReadingData();
                progress = true;
                break;

            case READING_TRAILER_CRLF:
                _processReadingTrailerCrlf();
                progress = true;
                break;

            case DONE:
                progress = false;
                break;
        }
    }
}

void ChunkedDecoder::_processReadingSize() {
    // Look for CRLF ending the size line
    std::size_t crlfPos = _buffer.find("\r\n");

    if (crlfPos == std::string::npos) {
        // No complete line yet - wait for more data
        return;
    }

    // Extract the size line
    std::string sizeLine = _buffer.substr(0, crlfPos);

    // Remove the size line from buffer
    _buffer.erase(0, crlfPos + 2);

    // Parse hex size (ignore extensions after ;)
    std::size_t semicolonPos = sizeLine.find(';');
    std::string hexSize;
    if (semicolonPos != std::string::npos) {
        hexSize = sizeLine.substr(0, semicolonPos);
    } else {
        hexSize = sizeLine;
    }

    // Trim whitespace (shouldn't be any, but be safe)
    std::size_t start = 0;
    while (start < hexSize.length() && hexSize[start] == ' ') {
        ++start;
    }
    std::size_t end = hexSize.length();
    while (end > start && hexSize[end - 1] == ' ') {
        --end;
    }
    hexSize = hexSize.substr(start, end - start);

    if (hexSize.empty()) {
        throw ChunkedDecodeException("Empty chunk size");
    }

    try {
        _currentChunkSize = _parseHexSize(hexSize);
    } catch (const ChunkedDecodeException&) {
        _error = true;
        throw;
    }

    _bytesRead = 0;

    if (_currentChunkSize == 0) {
        // Last chunk - need to read final CRLF after size line
        _state = READING_TRAILER_CRLF;
    } else {
        _state = READING_DATA;
    }
}

void ChunkedDecoder::_processReadingData() {
    // How many bytes do we still need for this chunk?
    std::size_t remaining = _currentChunkSize - _bytesRead;

    if (_buffer.size() < remaining) {
        // Not enough data yet - consume all we have
        _body += _buffer;
        _bytesRead += _buffer.size();
        _buffer.clear();
        return;
    }

    // We have enough data - consume exactly 'remaining' bytes
    _body += _buffer.substr(0, remaining);
    _buffer.erase(0, remaining);
    _bytesRead += remaining;

    // Move to trailer CRLF state
    _state = READING_TRAILER_CRLF;
}

void ChunkedDecoder::_processReadingTrailerCrlf() {
    // Need at least 2 bytes for CRLF
    if (_buffer.size() < 2) {
        return;
    }

    // Check for CRLF
    if (_buffer[0] != '\r' || _buffer[1] != '\n') {
        _error = true;
        throw ChunkedDecodeException("Expected CRLF after chunk data");
    }

    // Consume the CRLF
    _buffer.erase(0, 2);

    // If we just finished a zero-size chunk, we're done
    if (_currentChunkSize == 0) {
        _state = DONE;
    } else {
        // Otherwise, move to next chunk
        _state = READING_SIZE;
    }
}
