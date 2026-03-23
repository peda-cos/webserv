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
        void build_query_string_env(const StringMap& params);
        void build_envs_for_post_request(const HttpRequest& request);
        void build_fundamental_envs(const HttpRequest& request);
    
    public:
        CgiEnvBuilder(const HttpRequest& request);
        ~CgiEnvBuilder();

        char** getEnvp() const;
};

#endif