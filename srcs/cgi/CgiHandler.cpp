#include <CgiHandler.hpp>

#include <StringUtils.hpp>
#include <Logger.hpp>
#include <HttpUtils.hpp>
#include <CgiException.hpp>
#include <CgiUtils.hpp>

#include <sys/stat.h>
#include <cstdlib>
#include <cctype>

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

    static bool extract_extension(const std::string& request_path,
        std::string& extension,
        size_t& slash_pos)
    {
        size_t dot_pos = request_path.rfind('.');
        if (dot_pos == std::string::npos) {
            return false;
        }

        slash_pos = request_path.find('/', dot_pos);
        if (slash_pos == std::string::npos) {
            slash_pos = request_path.length();
        }

        extension = request_path.substr(dot_pos, slash_pos - dot_pos);
        return true;
    }

    static std::string to_lower_copy(const std::string& value) {
        std::string lower = value;
        for (size_t i = 0; i < lower.length(); ++i) {
            lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
        }
        return lower;
    }

    static std::string trim_left_copy(const std::string& value) {
        std::string::size_type first = value.find_first_not_of(" \t");
        if (first == std::string::npos) {
            return "";
        }
        return value.substr(first);
    }

    static std::string reason_phrase(int status_code) {
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
            case 500: return "Internal Server Error";
            case 504: return "Gateway Timeout";
            default: return "Internal Server Error";
        }
    }
}

CgiHandler::CgiHandler() : _executor() {}

CgiHandler::~CgiHandler() {}

CgiParsedOutput CgiHandler::build_error_output(int status_code, const std::string& error_msg) const {
    CgiParsedOutput output;
    output.status_code = status_code;
    output.body = error_msg.empty() ? reason_phrase(status_code) : error_msg;
    output.headers.push_back(std::make_pair("Content-Type", "text/html"));
    return output;
}

bool CgiHandler::is_cgi_request(const HttpRequest& req, const ServerConfig& server_config) const {
    const LocationConfig* location_config = find_best_matching_location(req, server_config);
    if (!location_config) {
        return false;
    }

    const std::string& request_path = request_target(req);
    std::string extension;
    size_t slash_pos = 0;
    if (!extract_extension(request_path, extension, slash_pos)) {
        return false;
    }

    return location_config->cgi_handlers.find(extension) != location_config->cgi_handlers.end();
}

bool CgiHandler::is_method_allowed(const std::string& method, const LocationConfig& location_config) const {
    if (location_config.limit_except.empty()) {
        return true;
    }

    for (size_t i = 0; i < location_config.limit_except.size(); ++i) {
        if (HttpUtils::method_to_string(location_config.limit_except[i]) == method) {
            return true;
        }
    }
    return false;
}

int CgiHandler::validate_request(const HttpRequest& req, const ServerConfig& server_config, const LocationConfig& location_config) const {
    if (!is_method_allowed(req.method, location_config)) {
        return 405;
    }

    if (req.body.size() > server_config.client_max_body_size) {
        return 413;
    }

    const std::string& request_path = request_target(req);
    std::string extension;
    size_t slash_pos = 0;
    if (!extract_extension(request_path, extension, slash_pos)) {
        return 404;
    }

    if (location_config.cgi_handlers.find(extension) == location_config.cgi_handlers.end()) {
        return 403;
    }

    std::string request_script_path = request_path.substr(0, slash_pos);
    std::string script_path = CgiUtils::absolute_path(CgiUtils::resolve_script_path(request_script_path, location_config));
    struct stat st;
    if (stat(script_path.c_str(), &st) != 0) {
        return 404;
    }

    return 0;
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

    std::string header_block;
    std::string body_block;

    size_t separator = raw_output.find("\r\n\r\n");
    size_t separator_len = 4;
    if (separator == std::string::npos) {
        separator = raw_output.find("\n\n");
        separator_len = 2;
    }

    if (separator == std::string::npos) {
        parsed.status_code = 200;
        parsed.body = raw_output;
        return parsed;
    }

    header_block = raw_output.substr(0, separator);
    body_block = raw_output.substr(separator + separator_len);
    parsed.body = body_block;

    std::string::size_type start = 0;
    while (start <= header_block.length()) {
        std::string::size_type end = header_block.find('\n', start);
        std::string line;
        if (end == std::string::npos) {
            line = header_block.substr(start);
            start = header_block.length() + 1;
        } else {
            line = header_block.substr(start, end - start);
            start = end + 1;
        }

        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        if (line.empty()) {
            continue;
        }

        std::string::size_type colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, colon);
        std::string value = trim_left_copy(line.substr(colon + 1));

        parsed.headers.push_back(std::make_pair(key, value));
    }

    parsed.status_code = 200;
    bool has_location = false;
    for (size_t i = 0; i < parsed.headers.size(); ++i) {
        std::string key = to_lower_copy(parsed.headers[i].first);
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

    return parsed;
}

CgiParsedOutput CgiHandler::handle_request(const HttpRequest& req,
    const ServerConfig& server_config) const
{
    if (req.errorCode != 0) {
        Logger::warn("CGI request rejected due to parser error: " + StringUtils::to_string(req.errorCode));
        return build_error_output(req.errorCode, "");
    }

    const std::string& request_path = request_target(req);
    Logger::info("CGI request started: " + req.method + " " + request_path);

    const LocationConfig* location_config = find_best_matching_location(req, server_config);
    if (!location_config) {
        Logger::warn("CGI location not found for path: " + request_path);
        return build_error_output(404, "Location not found");
    }

    int validation_status = validate_request(req, server_config, *location_config);
    if (validation_status != 0) {
        Logger::warn("CGI request validation failed (" + StringUtils::to_string(validation_status) + ") for path: " + request_path);
        return build_error_output(validation_status, "");
    }

    CgiResult result(CgiResult::EXECUTION_ERROR, "CGI execution failed");
    try {
        result = _executor.execute(req, *location_config);
    } catch (const CgiException& e) {
        Logger::error("CGI execution threw exception for path '" + request_path + "': " + std::string(e.what()));
        return build_error_output(500, e.what());
    }
    CgiParsedOutput parsed = parse_cgi_output(result.output, result.status);
    if (parsed.status_code >= 400 && parsed.body.empty()) {
        parsed.body = reason_phrase(parsed.status_code);
    }

    if (parsed.status_code >= 400) {
        Logger::warn("CGI request finished with status " + StringUtils::to_string(parsed.status_code) + " for path: " + request_path);
    } else {
        Logger::info("CGI request finished with status " + StringUtils::to_string(parsed.status_code) + " for path: " + request_path);
    }

    return parsed;
}