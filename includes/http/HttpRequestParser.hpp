#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include <string>
#include <HttpRequest.hpp>

class HttpRequestParser {
    public:
        HttpRequestParser();
        ~HttpRequestParser();

        void feed(const std::string& data);
        bool isComplete() const;
        HttpRequest getRequest() const;
        void setMaxBodySize(std::size_t size);

    private:
        std::string _buffer;
        bool _complete;
        std::size_t _maxBodySize;
        HttpRequest _request;

        void _parseRequestLine();
};

#endif
