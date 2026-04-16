#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <vector>
#include <Enums.hpp>

class HttpResponse {
    private:
        int status_code;
        std::string body;
        std::vector< std::pair<std::string, std::string> > headers;

    public:
        HttpResponse();
        HttpResponse &setStatusCode(HttpStatusCode code);
        HttpResponse &setStatusCode(int code);
        HttpResponse &setBody(const std::string &body);
        HttpResponse &addHeader(const std::string& key, const std::string& value);
        HttpResponse &setHeaders(const std::vector< std::pair<std::string, std::string> >& new_headers);
        std::string toString() const;

};

#endif