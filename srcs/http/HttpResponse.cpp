#include <HttpResponse.hpp>
#include <Constants.hpp>
#include <StringUtils.hpp>

#include <cctype>
#include <ctime>

namespace {
    static std::string reason_phrase(int status_code)
    {
        switch (status_code) {
            case 200: return "OK";
            case 201: return "Created";
            case 204: return "No Content";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 400: return "Bad Request";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 413: return "Payload Too Large";
            case 431: return "Request Header Fields Too Large";
            case 500: return "Internal Server Error";
            case 501: return "Not Implemented";
            case 504: return "Gateway Timeout";
            case 505: return "HTTP Version Not Supported";
            default: return "Internal Server Error";
        }
    }
}

static std::string format_http_date() {
    std::time_t now = std::time(NULL);
    std::tm* gmt = std::gmtime(&now);
    char buf[30];
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buf);
}

HttpResponse::HttpResponse() : status_code(200), body(), headers() {}

HttpResponse &HttpResponse::setStatusCode(HttpStatusCode code)
{
    this->status_code = static_cast<int>(code);
    return *this;
}

HttpResponse &HttpResponse::setStatusCode(int code)
{
    this->status_code = code;
    return *this;
}

HttpResponse &HttpResponse::setBody(const std::string &body)
{
    this->body = body;
    return *this;
}

HttpResponse &HttpResponse::addHeader(const std::string& key, const std::string& value)
{
    headers.push_back(std::make_pair(key, value));
    return *this;
}

HttpResponse &HttpResponse::setHeaders(const std::vector< std::pair<std::string, std::string> >& new_headers)
{
    headers = new_headers;
    return *this;
}

std::string HttpResponse::toString() const
{
    std::string response = "HTTP/1.1 " + StringUtils::to_string(status_code) + " " + reason_phrase(status_code);
    response += CARRIAGE_RETURN_LINE_FEED;

    bool has_content_length = false;
    bool has_date = false;
    for (std::size_t i = 0; i < headers.size(); ++i) {
        std::string lower_key = StringUtils::to_lower(headers[i].first);
        if (lower_key == "content-length") {
            has_content_length = true;
        }
        if (lower_key == "date") {
            has_date = true;
        }
        response += headers[i].first + ": " + headers[i].second + CARRIAGE_RETURN_LINE_FEED;
    }

    if (!has_date) {
        response += "Date: " + format_http_date() + CARRIAGE_RETURN_LINE_FEED;
    }

    if (!has_content_length) {
        response += "Content-Length: " + StringUtils::to_string(body.size()) + CARRIAGE_RETURN_LINE_FEED;
    }

    response += CARRIAGE_RETURN_LINE_FEED;
    response += body;
    return response;
}