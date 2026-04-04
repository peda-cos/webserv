#ifndef CGI_ENV_BUILDER_HPP
#define CGI_ENV_BUILDER_HPP

#include <map>
#include <string>
#include <HttpRequest.hpp>
#include <LocationConfig.hpp>
#include <TypeDefs.hpp>

class CgiEnvBuilder {
    private:
        std::map<std::string, std::string> env_map;
        char **envp;

        void build_envp();
        void build_headers_envs(const HttpRequest& request);
        void build_query_string_env(const HttpRequest& request);
        void build_envs_for_post_request(const HttpRequest& request);
        void build_fundamental_envs(const HttpRequest& request, const LocationConfig& location);
        
        // Helper: Extract PATH_INFO from uri_path by finding CGI script extension
        // E.g., uri_path="/app.py/extra/path" with .py handler → PATH_INFO="/extra/path"
        std::string extract_path_info(const std::string& uri_path, const LocationConfig& location);
    
    public:
        CgiEnvBuilder(const HttpRequest& request, const LocationConfig& location);
        ~CgiEnvBuilder();

        char** getEnvp() const;
};

#endif