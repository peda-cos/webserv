#ifndef HTTP_UTILS_HPP
#define HTTP_UTILS_HPP

#include <string>
#include <HttpRequest.hpp>

class HttpUtils {
    public:
        static std::string method_to_string(HttpMethod method);
};

#endif