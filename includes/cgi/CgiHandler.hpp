#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <HttpRequest.hpp>
#include <CgiExecutor.hpp>
#include <CgiResult.hpp>
#include <ServerConfig.hpp>

struct CgiParsedOutput {
    int status_code;
    std::string body;
    std::vector< std::pair<std::string, std::string> > headers;

    CgiParsedOutput() : status_code(200), body(), headers() {}
};

class CgiHandler {
    public:
        CgiHandler();
        ~CgiHandler();

        bool is_cgi_request(const HttpRequest& req, const ServerConfig& server_config) const;
        int start_cgi(const HttpRequest& req, const ServerConfig& server_config, CgiProcessInfo& out_info) const;
        CgiParsedOutput parse_cgi_output(const std::string& raw_output, CgiResult::Status execution_status) const;

    private:
        const CgiExecutor _executor;
        int validate_request(const HttpRequest& req, const ServerConfig& server_config, const LocationConfig& location_config) const;
};

#endif