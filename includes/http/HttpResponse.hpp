#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <Enums.hpp>

class HttpResponse {
    private:
        HttpStatusCode status_code;
        std::string body;
    public:
        HttpResponse &setStatusCode(HttpStatusCode code);
        HttpResponse &setBody(const std::string &body);
        std::string toString() const;

};

#endif