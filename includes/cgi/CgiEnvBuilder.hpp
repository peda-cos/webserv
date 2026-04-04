#ifndef CGI_ENV_BUILDER_HPP
#define CGI_ENV_BUILDER_HPP

#include <map>
#include <string>
#include <HttpRequest.hpp>
#include <LocationConfig.hpp>
#include <TypeDefs.hpp>

struct UriPathParts {
    std::string script_name;
    std::string path_info;
};

class CgiEnvBuilder {
    private:
        std::map<std::string, std::string> env_map;
        char **envp;

        void build_envp();
        void build_headers_envs(const HttpRequest& request);
        void build_query_string_env(const HttpRequest& request);
        void build_envs_for_post_request(const HttpRequest& request);
        void build_fundamental_envs(const HttpRequest& request, const LocationConfig& location);
        
        UriPathParts extract_path_parts(const HttpRequest& request, const LocationConfig& location);
    
    public:
        CgiEnvBuilder(const HttpRequest& request, const LocationConfig& location);
        ~CgiEnvBuilder();

        char** getEnvp() const;
};

#endif