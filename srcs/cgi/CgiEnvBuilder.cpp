#include <CgiEnvBuilder.hpp>
#include <TypeDefs.hpp>
#include <CgiUtils.hpp>
#include <HttpUtils.hpp>
#include <sstream>

void CgiEnvBuilder::build_fundamental_envs(const HttpRequest& request) {
    env_map["SERVER_PROTOCOL"] = request.version;
    env_map["SERVER_SOFTWARE"] = "Webserv/1.0"; // TODO: tornar dinâmico com base no nome do servidor e versão, como ?
    env_map["GATEWAY_INTERFACE"] = "CGI/1.1";
    env_map["SCRIPT_NAME"] = request.uri_path;
    env_map["REQUEST_METHOD"] = HttpUtils::method_to_string(request.method);
    env_map["REQUEST_URI"] = request.uri_path;
    env_map["PATH_INFO"] = "PENDENTE"; // TODO: Entender melhor essa variável e como preenchê-la corretamente
    env_map["REMOTE_ADDR"] = "PENDENTE"; // TODO: Obter o endereço IP do cliente a partir do socket de conexão
}


void CgiEnvBuilder::build_query_string_env(const StringMap& params) {
    std::string query_string = "";
    for (StringMapIterator it = params.begin(); it != params.end(); ++it) {
        if (!query_string.empty()) {
            query_string += "&";
        }
        query_string += it->first + "=" + it->second;
    }
    env_map["QUERY_STRING"] = query_string;
}

void CgiEnvBuilder::build_envs_for_post_request(const HttpRequest& request) {
    std::stringstream content_length_stream;
    content_length_stream << request.body.size();
    env_map["CONTENT_LENGTH"] = content_length_stream.str();
    StringMapIterator content_type_it = request.headers.find("Content-Type");
    if (request.headers.find("Content-Type") != request.headers.end()) {
        env_map["CONTENT_TYPE"] = content_type_it->second;
    }
}

void CgiEnvBuilder::build_headers_envs(const HttpRequest& request) {
    const StringMap &headers = request.headers;
    for (StringMapIterator it = headers.begin(); it != headers.end(); ++it) {
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

CgiEnvBuilder::CgiEnvBuilder(const HttpRequest& request) : envp(NULL) {
    build_fundamental_envs(request);
    build_query_string_env(request.query_parameters);
    if (request.method == POST) {
        build_envs_for_post_request(request);
    }
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
    if (envp) return envp;
    return NULL;
}