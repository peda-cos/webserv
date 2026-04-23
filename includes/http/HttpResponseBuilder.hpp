#ifndef HTTP_RESPONSE_BUILDER_HPP
#define HTTP_RESPONSE_BUILDER_HPP

#include <string>
#include <vector>

class HttpResponseBuilder {
    public:
        HttpResponseBuilder();

        HttpResponseBuilder& setStatus(int code, const std::string& reason);
        HttpResponseBuilder& setStatus(int code);
        HttpResponseBuilder& setHeader(const std::string& key, const std::string& value);
        HttpResponseBuilder& setBody(const std::string& body);
        HttpResponseBuilder& setContentType(const std::string& type);
        HttpResponseBuilder& setConnection(const std::string& value);

        std::string build() const;

        static std::string buildErrorPage(int code, const std::string& customBody);
        static std::string buildRedirect(int code, const std::string& location);
        static std::string reasonPhraseFor(int code);

    private:
        int _statusCode;
        std::string _reasonPhrase;
        std::string _body;
        std::string _contentType;
        std::string _connection;
        std::vector< std::pair<std::string, std::string> > _headers;

        static std::string _defaultErrorHtml(int code, const std::string& reason);
};

#endif
