#include <CgiHandler.hpp>

#include <StringUtils.hpp>
#include <Logger.hpp>
#include <HttpUtils.hpp>
#include <CgiUtils.hpp>

#include <sys/stat.h>
#include <cstdlib>

namespace {
    static const std::string& request_target(const HttpRequest& req) {
        return req.path.empty() ? req.uri : req.path;
    }

    static const LocationConfig* find_best_matching_location(const HttpRequest& req,
        const ServerConfig& server_config)
    {
        const LocationConfig* best_match = NULL;
        size_t best_match_length = 0;

        for (size_t i = 0; i < server_config.locations.size(); ++i) {
            const std::string& loc_path = server_config.locations[i].path;

            if (req.path.find(loc_path) == 0 && loc_path.length() > best_match_length) {
                best_match = &(server_config.locations[i]);
                best_match_length = loc_path.length();
            }
        }

        return best_match;
    }

    static bool find_body_separator(const std::string& raw_output,
        size_t& separator_pos,
        size_t& separator_len)
    {
        separator_pos = raw_output.find("\r\n\r\n");
        if (separator_pos != std::string::npos) {
            separator_len = 4;
            return true;
        }
        separator_pos = raw_output.find("\n\n");
        if (separator_pos != std::string::npos) {
            separator_len = 2;
            return true;
        }
        return false;
    }

    static void parse_header_lines(const std::string& header_block, CgiParsedOutput& parsed) {
        for (std::string::size_type start = 0; start <= header_block.length(); ) {
            std::string::size_type end = header_block.find('\n', start);
            if (end == std::string::npos) end = header_block.length();
            
            std::string line = header_block.substr(start, end - start);
            start = end + 1;
            
            if (!line.empty() && line[line.length() - 1] == '\r') line.erase(line.length() - 1);
            if (line.empty() || line.find(':') == std::string::npos) continue;
            
            size_t colon = line.find(':');
            parsed.headers.push_back(std::make_pair(line.substr(0, colon), StringUtils::trim_left(line.substr(colon + 1))));
        }
    }

    static void extract_status_and_location(CgiParsedOutput& parsed) {
        parsed.status_code = 200;
        bool has_location = false;

        for (size_t i = 0; i < parsed.headers.size(); ++i) {
            std::string key = StringUtils::to_lower(parsed.headers[i].first);
            if (key == "status") {
                parsed.status_code = std::atoi(parsed.headers[i].second.c_str());
                if (parsed.status_code < 100 || parsed.status_code >= 600) {
                    parsed.status_code = 200;
                }
            }
            if (key == "location") {
                has_location = true;
            }
        }

        if (parsed.status_code == 200 && has_location) {
            parsed.status_code = 302;
        }
    }

}

CgiHandler::CgiHandler() : _executor() {}
CgiHandler::~CgiHandler() {}

bool CgiHandler::is_cgi_request(const HttpRequest& req, const ServerConfig& server_config) const {
    const LocationConfig* loc = find_best_matching_location(req, server_config);
    if (!loc) return false;
    std::string ext;
    size_t slash_pos = 0;
    return CgiUtils::extract_extension(request_target(req), ext, slash_pos) && 
           loc->cgi_handlers.find(ext) != loc->cgi_handlers.end();
}

int CgiHandler::validate_request(const HttpRequest& req, const ServerConfig& server_config, const LocationConfig& location_config) const {
    if (!location_config.limit_except.empty()) {
        bool allowed = false;
        for (size_t i = 0; i < location_config.limit_except.size(); ++i) {
            if (HttpUtils::method_to_string(location_config.limit_except[i]) == req.method) {
                allowed = true;
                break;
            }
        }
        if (!allowed) return 405;
    }
    if (req.body.size() > server_config.client_max_body_size) return 413;

    std::string ext;
    size_t slash_pos = 0;
    if (!CgiUtils::extract_extension(request_target(req), ext, slash_pos)) return 404;
    if (location_config.cgi_handlers.find(ext) == location_config.cgi_handlers.end()) return 403;

    std::string script_path = CgiUtils::absolute_path(CgiUtils::resolve_script_path(request_target(req).substr(0, slash_pos), location_config));
    struct stat st;
    return (stat(script_path.c_str(), &st) == 0) ? 0 : 404;
}

CgiParsedOutput CgiHandler::parse_cgi_output(const std::string& raw_output, CgiResult::Status execution_status) const {
    CgiParsedOutput parsed;

    if (execution_status == CgiResult::TIMEOUT) {
        parsed.status_code = 504;
        parsed.body = "CGI execution timed out";
        return parsed;
    }
    if (execution_status != CgiResult::SUCCESS) {
        parsed.status_code = 500;
        parsed.body = raw_output.empty() ? "CGI execution failed" : raw_output;
        return parsed;
    }

    size_t separator = 0;
    size_t separator_len = 0;
    if (!find_body_separator(raw_output, separator, separator_len)) {
        parsed.status_code = 200;
        parsed.body = raw_output;
        return parsed;
    }

    std::string header_block = raw_output.substr(0, separator);
    parsed.body = raw_output.substr(separator + separator_len);

    parse_header_lines(header_block, parsed);
    extract_status_and_location(parsed);

    return parsed;
}

int CgiHandler::start_cgi(const HttpRequest& req,
    const ServerConfig& server_config, CgiProcessInfo& out_info, int client_fd) const
{
    const LocationConfig* location_config = find_best_matching_location(req, server_config);
    if (!location_config) {
        return 404;
    }

    int validation_status = validate_request(req, server_config, *location_config);
    if (validation_status != 0) {
        return validation_status;
    }

    out_info = _executor.start_cgi(req, *location_config, server_config, client_fd);
    return 0;
}
