#ifndef SEMANTIC_VALIDATOR_HPP
#define SEMANTIC_VALIDATOR_HPP

#include <vector>
#include <string>
#include <set>
#include <map>
#include "../../includes/config/ConfigNode.hpp"
#include "../../includes/config/ValidationError.hpp"
#include "../../includes/types/ServerConfig.hpp"

class SemanticValidator {
public:
    SemanticValidator();
    bool validate(ConfigNode* root, std::vector<ServerConfig>& configs,
                  std::vector<ValidationError>& errors);

private:
    std::vector<ValidationError> errors_;
    std::set<std::string> singleInstanceDirectives_;
    std::set<std::string> validMethods_;

    // Pass 1: Local validation
    void validateLocal(ConfigNode* node, const std::string& context);
    void validateDirective(const std::string& name, const std::vector<std::string>& args,
                          size_t line, const std::string& context);
    
    // Pass 2: Symbol table and references
    void validateReferences(ConfigNode* node);
    
    // Pass 3: Build configs and apply inheritance
    void buildConfigs(ConfigNode* node, std::vector<ServerConfig>& configs);
    void buildServerConfig(ConfigNode* node, ServerConfig& config);
    void buildLocationConfig(ConfigNode* node, LocationConfig& loc);
    void applyInheritance(ServerConfig& server);
    
    // Helpers
    bool isValidPort(const std::string& port);
    bool isValidMethod(const std::string& method);
    bool isValidRedirectCode(int code);
    bool isSingleInstanceDirective(const std::string& name);
    size_t parseSize(const std::string& s);
    int stringToInt(const std::string& s);
};

#endif
