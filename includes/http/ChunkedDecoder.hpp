#ifndef CHUNKED_DECODER_HPP
#define CHUNKED_DECODER_HPP

#include <string>
#include <stdexcept>

/**
 * Exception thrown when chunked decoding encounters malformed data.
 */
class ChunkedDecodeException : public std::runtime_error {
public:
    explicit ChunkedDecodeException(const std::string& msg)
        : std::runtime_error(msg) {}
};

/**
 * Incremental decoder for HTTP chunked transfer-encoding.
 *
 * This class parses chunked-encoded data received in arbitrary fragments,
 * extracting hex chunk sizes, reading chunk data, and concatenating it
 * into a complete decoded body.
 *
 * State machine:
 *   READING_SIZE       - Accumulating hex digits until CRLF
 *   READING_DATA       - Consuming exactly N bytes of chunk data
 *   READING_TRAILER_CRLF - Consuming CRLF after each chunk
 *   DONE               - Zero-length terminator received
 */
class ChunkedDecoder {
public:
    enum DecoderState {
        READING_SIZE,
        READING_DATA,
        READING_TRAILER_CRLF,
        DONE
    };

    /**
     * Constructor - initializes decoder in READING_SIZE state.
     */
    ChunkedDecoder();

    /**
     * Destructor.
     */
    ~ChunkedDecoder();

    /**
     * Feed data to the decoder. May be called multiple times with
     * arbitrary data fragments.
     *
     * @param data The data fragment to process
     * @throw ChunkedDecodeException if malformed data is encountered
     */
    void feed(const std::string& data);

    /**
     * Check if the complete body has been decoded.
     *
     * @return true if the zero-length terminator chunk has been received
     */
    bool isComplete() const;

    /**
     * Get the decoded body accumulated so far.
     *
     * @return The decoded body content
     */
    std::string getBody() const;

    /**
     * Check if an error has occurred.
     *
     * @return true if the decoder is in an error state
     */
    bool hasError() const;

    /**
     * Get any unconsumed data after the chunked body is complete.
     *
     * @return Trailing bytes after the chunked terminator, or empty string if not done
     */
    std::string getRemainder() const;

private:
    DecoderState _state;
    std::string _buffer;           // Buffer for incomplete data
    std::string _body;             // Accumulated decoded body
    std::size_t _currentChunkSize; // Size of chunk being read
    std::size_t _bytesRead;        // Bytes read in current chunk
    bool _error;                   // Error state flag

    /**
     * Parse a hex string to size_t.
     *
     * @param hex The hex string to parse (case-insensitive)
     * @return The parsed size
     * @throw ChunkedDecodeException if invalid hex characters
     */
    std::size_t _parseHexSize(const std::string& hex) const;

    /**
     * Process accumulated data in the buffer based on current state.
     */
    void _processBuffer();

    /**
     * Handle READING_SIZE state - find CRLF and parse hex size.
     */
    void _processReadingSize();

    /**
     * Handle READING_DATA state - accumulate chunk data.
     */
    void _processReadingData();

    /**
     * Handle READING_TRAILER_CRLF state - consume CRLF after chunk data.
     */
    void _processReadingTrailerCrlf();
};

#endif
