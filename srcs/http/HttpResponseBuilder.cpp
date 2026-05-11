#include <HttpResponseBuilder.hpp>
#include <HttpResponse.hpp>
#include <Constants.hpp>
#include <StringUtils.hpp>

#include <sstream>

HttpResponseBuilder::HttpResponseBuilder()
    : _statusCode(200)
    , _reasonPhrase("OK")
    , _body()
    , _contentType("text/html")
    , _connection("close")
    , _headers()
{}


HttpResponseBuilder& HttpResponseBuilder::setStatus(int code, const std::string& reason)
{
    _statusCode   = code;
    _reasonPhrase = reason;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::setStatus(int code)
{
    _statusCode   = code;
    _reasonPhrase = reasonPhraseFor(code);
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::setHeader(const std::string& key, const std::string& value)
{
    _headers.push_back(std::make_pair(key, value));
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::setBody(const std::string& body)
{
    _body = body;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::setContentType(const std::string& type)
{
    _contentType = type;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::setConnection(const std::string& value)
{
    _connection = value;
    return *this;
}


std::string HttpResponseBuilder::build() const
{
    HttpResponse response;
    response.setStatusCode(_statusCode);
    response.setBody(_body);

    response.addHeader("Server", "webserv");
    response.addHeader("Content-Type", _contentType);

    for (std::size_t i = 0; i < _headers.size(); ++i) {
        response.addHeader(_headers[i].first, _headers[i].second);
    }

    response.addHeader("Connection", _connection);


    return response.toString();
}


std::string HttpResponseBuilder::reasonPhraseFor(int code)
{
    switch (code) {
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
        default:  return "Internal Server Error";
    }
}


std::string HttpResponseBuilder::buildErrorBody(int code, const std::string& message)
{
    std::string reason = reasonPhraseFor(code);
    std::string codeStr = StringUtils::to_string(code);
    std::string html;
    html += "<html>\r\n";
    html += "<head><title>" + codeStr + " " + reason + "</title></head>\r\n";
    html += "<body>\r\n";
    html += "<center><h1>" + codeStr + " " + reason + "</h1></center>\r\n";
    if (!message.empty()) {
        html += "<center><p>" + message + "</p></center>\r\n";
    }
    html += "<hr><center>webserv</center>\r\n";
    html += "</body>\r\n";
    html += "</html>\r\n";
    return html;
}


std::string HttpResponseBuilder::buildErrorPage(int code, const std::string& customBody)
{
    return HttpResponseBuilder()
        .setStatus(code)
        .setContentType("text/html")
        .setBody(buildErrorBody(code, customBody))
        .setConnection("close")
        .build();
}

std::string HttpResponseBuilder::buildRedirect(int code, const std::string& location)
{
    return HttpResponseBuilder()
        .setStatus(code)
        .setHeader("Location", location)
        .setContentType("text/html")
        .setBody(buildErrorBody(code, ""))
        .setConnection("close")
        .build();
}
