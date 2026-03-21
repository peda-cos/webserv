#include <HttpUtils.hpp>

std::string HttpUtils::method_to_string(HttpMethod method) {
    switch (method) {
        case GET: return "GET";
        case POST: return "POST";
        case DELETE: return "DELETE";
        case PUT: return "PUT";
        case HEAD: return "HEAD";
        case CONNECT: return "CONNECT";
        case OPTIONS: return "OPTIONS";
        case TRACE: return "TRACE";
        case PATCH: return "PATCH";
        default: return "UNKNOWN";
    }
}