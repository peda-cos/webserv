#include "ParsingUtils.hpp"
#include "Enums.hpp"
#include <map>
#include <sstream>

static std::map<std::string, ConfigDirectiveType> createRootDirectives() {
    std::map<std::string, ConfigDirectiveType> map;
    map["server"] = ROOT_SERVER;
    map["http"] = ROOT_HTTP;
    return map;
}

static std::map<std::string, ConfigDirectiveType> createServerDirectives() {
    std::map<std::string, ConfigDirectiveType> map;
    map["listen"] = SERVER_LISTEN;
    map["location"] = SERVER_LOCATION;
    map["server_name"] = SERVER_NAMES;
    map["client_max_body_size"] = SERVER_CLIENT_MAX_BODY_SIZE;
    map["error_page"] = SERVER_ERROR_PAGE;
    map["root"] = SERVER_ROOT;
    return map;
}

static std::map<std::string, ConfigDirectiveType> createLocationDirectives() {
    std::map<std::string, ConfigDirectiveType> map;
    map["root"] = LOCATION_ROOT;
    map["index"] = LOCATION_INDEX;
    map["methods"] = LOCATION_METHODS;
    map["limit_except"] = LOCATION_LIMITS_EXCEPT;
    map["autoindex"] = LOCATION_AUTOINDEX;
    map["upload_store"] = LOCATION_UPLOAD_STORE;
    map["cgi_pass"] = LOCATION_CGI_PASS;
    map["return"] = LOCATION_REDIRECT;
    return map;
}

static std::map<std::string, ConfigDirectiveType> valid_root_directives = createRootDirectives();
static std::map<std::string, ConfigDirectiveType> valid_server_directives = createServerDirectives();
static std::map<std::string, ConfigDirectiveType> valid_location_directives = createLocationDirectives();

static ConfigDirectiveType get_directive(const std::string& word, const std::map<std::string, ConfigDirectiveType>& directives_map) {
    std::map<std::string, ConfigDirectiveType>::const_iterator it = directives_map.find(word);
    return (it != directives_map.end()) ? it->second : UNKNOWN;
}

namespace ParsingUtils {
bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

ConfigDirectiveType get_root_directive_type(const std::string& word) {
    return get_directive(word, valid_root_directives);
}

ConfigDirectiveType get_server_directive_type(const std::string& word) {
    return get_directive(word, valid_server_directives);
}

ConfigDirectiveType get_location_directive_type(const std::string& word) {
    return get_directive(word, valid_location_directives);
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}
    }