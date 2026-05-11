#ifndef CHUNKED_DECODER_HPP
#define CHUNKED_DECODER_HPP

#include <string>
#include <stdexcept>

class ChunkedDecodeException : public std::runtime_error {
public:
    explicit ChunkedDecodeException(const std::string& msg)
        : std::runtime_error(msg) {}
};

class ChunkedBodyTooLargeException : public ChunkedDecodeException {
public:
    explicit ChunkedBodyTooLargeException(const std::string& msg)
        : ChunkedDecodeException(msg) {}
};

// State machine: READING_SIZE → READING_DATA → READING_TRAILER_CRLF → DONE
// Zero-length terminator → DONE
class ChunkedDecoder {
public:
    enum DecoderState {
        READING_SIZE,
        READING_DATA,
        READING_TRAILER_CRLF,
        READING_TRAILERS,
        DONE
    };

    ChunkedDecoder();

    ~ChunkedDecoder();

    /**
     * Feed data to the decoder. May be called multiple times with
     * arbitrary data fragments.
     *
     * @param data The data fragment to process
     * @throw ChunkedDecodeException if malformed data is encountered
     */
    void feed(const std::string& data);
    void setMaxBodySize(std::size_t size);

    /**
     * Check if the complete body has been decoded.
     *
     * @return true if the zero-length terminator chunk has been received
     */
    bool isComplete() const;

    std::string getBody() const;

    bool hasError() const;

    /**
     * Get any unconsumed data after the chunked body is complete.
     *
     * @return Trailing bytes after the chunked terminator, or empty string if not done
     */
    std::string getRemainder() const;

private:
    DecoderState _state;
    std::string _buffer;
    std::string _body;
    std::size_t _currentChunkSize;
    std::size_t _bytesRead;
    std::size_t _maxBodySize;      // 0 = unlimited
    bool _error;

    /**
     * Parse a hex string to size_t.
     *
     * @param hex The hex string to parse (case-insensitive)
     * @return The parsed size
     * @throw ChunkedDecodeException if invalid hex characters
     */
    std::size_t _parseHexSize(const std::string& hex) const;

    void _processBuffer();

    /**
     * Handle READING_SIZE state - find CRLF and parse hex size.
     */
    void _processReadingSize();

    void _processReadingData();

    void _processReadingTrailerCrlf();
    void _processReadingTrailers();
    void _appendToBody(const std::string& data);
};

#endif
