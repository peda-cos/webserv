#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include <string>
#include <ChunkedDecoder.hpp>
#include <HttpRequest.hpp>

class HttpRequestParser {
    public:
        enum ParserState {
            REQUEST_LINE,
            HEADERS,
            BODY,
            CHUNKED_BODY,
            COMPLETE,
            ERROR
        };

        HttpRequestParser();
        ~HttpRequestParser();

        void feed(const std::string& data);
        bool isComplete() const;
        HttpRequest getRequest() const;
        void setMaxBodySize(std::size_t size);

    private:
        std::string _buffer;
        ParserState _state;
        std::size_t _maxBodySize;
        HttpRequest _request;
        std::size_t _contentLength;
        std::string _bodyBuffer;
        ChunkedDecoder _chunkedDecoder;

        void _parseRequestLine();
        void _parseHeaders();
        void _initiateBodyReading();
        void _parseContentLength();
};

#endif
