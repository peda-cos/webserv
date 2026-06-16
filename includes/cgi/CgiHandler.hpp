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

enum CgiExecutionStatus {
    CGI_EXEC_SUCCESS,
    CGI_EXEC_TIMEOUT,
    CGI_EXEC_ERROR
};

class CgiHandler {
    public:
        CgiHandler();
        ~CgiHandler();

        int start_cgi(const HttpRequest& req, const ServerConfig& server_config, CgiProcessInfo& out_info, int client_fd = -1) const;
        CgiParsedOutput parse_cgi_output(const std::string& raw_output, CgiExecutionStatus execution_status = CGI_EXEC_SUCCESS) const;

    private:
        const CgiExecutor _executor;
        int validate_request(const HttpRequest& req, const ServerConfig& server_config, const LocationConfig& location_config) const;
};

#endif
