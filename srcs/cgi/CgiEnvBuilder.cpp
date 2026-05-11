#include <CgiEnvBuilder.hpp>
#include <TypeDefs.hpp>
#include <CgiUtils.hpp>
#include <StringUtils.hpp>
#include <sstream>

UriPathParts CgiEnvBuilder::extract_path_parts(const HttpRequest& request, const LocationConfig& location) {
    UriPathParts parts;
    const std::string& request_path = request.path.empty() ? request.uri : request.path;

    std::size_t dotPos = request_path.rfind('.');
    if (dotPos == std::string::npos) {
        parts.script_name = request_path;
        parts.path_info = "";
        return parts;
    }

    std::size_t slash_pos = request_path.find('/', dotPos);
    if (slash_pos == std::string::npos) {
        slash_pos = request_path.length();
    }

    std::string extension = request_path.substr(dotPos, slash_pos - dotPos);

    if (location.cgi_handlers.find(extension) != location.cgi_handlers.end()) {
        parts.script_name = request_path.substr(0, slash_pos);
        parts.path_info = request_path.substr(slash_pos);
    } else {
        parts.script_name = request_path;
        parts.path_info = "";
    }

    return parts;
}

void CgiEnvBuilder::build_fundamental_envs(const HttpRequest& request, const LocationConfig& location,
    const std::string& script_filename, const std::string& server_name,
    const std::string& server_port, const std::string& remote_addr) {
    std::string server_protocol = "HTTP/" + request.httpVersion;
    env_map["SERVER_PROTOCOL"] = server_protocol;

    env_map["SERVER_SOFTWARE"] = "Webserv/1.0";
    env_map["GATEWAY_INTERFACE"] = "CGI/1.1";

    // RFC 3875 §4.1.1: AUTH_TYPE — empty if no authentication
    env_map["AUTH_TYPE"] = "";

    env_map["REQUEST_METHOD"] = request.method;
    env_map["REQUEST_URI"] = request.uri;
    env_map["CONTENT_LENGTH"] = "0";
    env_map["CONTENT_TYPE"] = "";

    UriPathParts path_parts = extract_path_parts(request, location);
    env_map["SCRIPT_NAME"] = path_parts.script_name;
    env_map["PATH_INFO"] = path_parts.path_info;

    // RFC 3875 §4.1.6: PATH_TRANSLATED derived from PATH_INFO
    if (!path_parts.path_info.empty()) {
        env_map["PATH_TRANSLATED"] = CgiUtils::absolute_path(
            CgiUtils::resolve_script_path(path_parts.path_info, location));
    }

    if (!script_filename.empty()) {
        env_map["SCRIPT_FILENAME"] = script_filename;
    }
    if (!server_name.empty()) {
        env_map["SERVER_NAME"] = server_name;
    }
    if (!server_port.empty()) {
        env_map["SERVER_PORT"] = server_port;
    }
    if (!remote_addr.empty()) {
        env_map["REMOTE_ADDR"] = remote_addr;
        // RFC 3875 §4.1.9: REMOTE_HOST — fallback to REMOTE_ADDR
        env_map["REMOTE_HOST"] = remote_addr;
    }
}


void CgiEnvBuilder::build_query_string_env(const HttpRequest& request) {
    env_map["QUERY_STRING"] = request.queryString;
}

void CgiEnvBuilder::build_envs_for_post_request(const HttpRequest& request) {
    std::stringstream content_length_stream;
    content_length_stream << request.body.size();
    env_map["CONTENT_LENGTH"] = content_length_stream.str();
    StringMapIterator it;
    for (it = request.headers.begin(); it != request.headers.end(); ++it) {
        if (CgiUtils::env_normalize(it->first) == "CONTENT_TYPE") break;
    }
    if (it != request.headers.end()) {
        env_map["CONTENT_TYPE"] = it->second;
    }
}

void CgiEnvBuilder::build_headers_envs(const HttpRequest& request) {
    const StringMap &headers = request.headers;
    // RFC 3875 §4.1.18: exclude headers already available as meta-variables
    // Content-Length, Content-Type, Authorization, Connection
    static const std::string excluded[] = {
        "content-length", "content-type", "authorization", "connection"
    };
    static const size_t excluded_count = sizeof(excluded) / sizeof(excluded[0]);

    for (StringMapIterator it = headers.begin(); it != headers.end(); ++it) {
        std::string lower = StringUtils::to_lower(it->first);
        bool skip = false;
        for (size_t i = 0; i < excluded_count; ++i) {
            if (lower == excluded[i]) { skip = true; break; }
        }
        if (skip) continue;
        std::string header_name = "HTTP_" + CgiUtils::env_normalize(it->first);
        env_map[header_name] = it->second;
    }
}

void CgiEnvBuilder::build_envp() {
    envp = new char*[env_map.size() + 1];
    size_t index = 0;
    for (StringMapIterator it = env_map.begin(); it != env_map.end(); ++it) {
        std::string env_entry = it->first + "=" + it->second;
        envp[index] = new char[env_entry.size() + 1];
        size_t len = env_entry.copy(envp[index], env_entry.size());
        envp[index][len] = '\0';
        ++index;
    }
    envp[index] = NULL;
}

CgiEnvBuilder::CgiEnvBuilder(const HttpRequest& request, const LocationConfig& location,
    const std::string& script_filename, const std::string& server_name,
    const std::string& server_port, const std::string& remote_addr) : envp(NULL) {
    build_fundamental_envs(request, location, script_filename, server_name, server_port, remote_addr);
    build_query_string_env(request);
    if (request.method == "POST") {
        build_envs_for_post_request(request);
    }
    build_headers_envs(request);
    build_envp();
}

CgiEnvBuilder::~CgiEnvBuilder() {
    if (envp) {
        for (size_t i = 0; envp[i] != NULL; ++i) {
            delete[] envp[i];
        }
        delete[] envp;
    }
}

char** CgiEnvBuilder::getEnvp() const {
    return envp;
}