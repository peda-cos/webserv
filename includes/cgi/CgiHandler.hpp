#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <HttpRequest.hpp>
#include <CgiExecutor.hpp>
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
        CgiParsedOutput handle_request(const HttpRequest& req,
            const ServerConfig& server_config) const;
        
        bool is_cgi_request(const HttpRequest& req, const ServerConfig& server_config) const;

    private:
        const CgiExecutor _executor;
        CgiParsedOutput build_error_output(int status_code, const std::string& error_msg) const;
        CgiParsedOutput parse_cgi_output(const std::string& raw_output, CgiResult::Status execution_status) const;
        int validate_request(const HttpRequest& req, const ServerConfig& server_config, const LocationConfig& location_config) const;
        bool is_method_allowed(const std::string& method, const LocationConfig& location_config) const;
};

#endif