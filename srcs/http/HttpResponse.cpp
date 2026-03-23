#include <HttpResponse.hpp>
#include <Constants.hpp>
#include <StringUtils.hpp>

HttpResponse &HttpResponse::setStatusCode(HttpStatusCode code)
{
    this->status_code = code;
    return *this;
}

HttpResponse &HttpResponse::setBody(const std::string &body)
{
    this->body = body;
    return *this;
}

std::string HttpResponse::toString() const
{
    std::string response = "HTTP/1.1 " + StringUtils::to_string(status_code) + " ";
    switch (status_code) {
        case OK: response += "OK"; break;
        case BAD_REQUEST: response += "Bad Request"; break;
        case NOT_FOUND: response += "Not Found"; break;
        case INTERNAL_SERVER_ERROR: response += "Internal Server Error"; break;
        default: response += "Unknown Status"; break;
    }
    response += CARRIAGE_RETURN_LINE_FEED;
    response += "Content-Length: "
        + StringUtils::to_string(body.size())
        + CARRIAGE_RETURN_LINE_FEED;
    response += CARRIAGE_RETURN_LINE_FEED;
    response += body;
    return response;
}