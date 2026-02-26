#include "../../includes/config/SemanticValidator.hpp"
#include <sstream>
#include <cstdlib>

SemanticValidator::SemanticValidator() {
    // Directives that can only appear once per context
    singleInstanceDirectives_.insert("listen");
    singleInstanceDirectives_.insert("server_name");
    singleInstanceDirectives_.insert("root");
    singleInstanceDirectives_.insert("client_max_body_size");
    singleInstanceDirectives_.insert("autoindex");
    singleInstanceDirectives_.insert("index");
    singleInstanceDirectives_.insert("upload_enabled");
    singleInstanceDirectives_.insert("upload_dir");
    
    // Valid HTTP methods
    validMethods_.insert("GET");
    validMethods_.insert("POST");
    validMethods_.insert("DELETE");
    validMethods_.insert("PUT");
    validMethods_.insert("HEAD");
}

bool SemanticValidator::validate(ConfigNode* root, std::vector<ServerConfig>& configs,
                                  std::vector<ValidationError>& errors) {
    errors_.clear();
    
    if (!root) {
        errors_.push_back(ValidationError(STAGE_VALIDATOR, SEVERITY_ERROR, 0, 
                                          "Root node is NULL", ""));
        errors = errors_;
        return false;
    }
    
    // Pass 1: Local validation of each directive
    for (size_t i = 0; i < root->children.size(); ++i) {
        validateLocal(root->children[i], "root");
    }
    
    // Pass 2: Validate references (if needed)
    for (size_t i = 0; i < root->children.size(); ++i) {
        validateReferences(root->children[i]);
    }
    
    // Pass 3: Build configs and apply inheritance
    buildConfigs(root, configs);
    
    errors = errors_;
    return errors_.empty();
}

void SemanticValidator::validateLocal(ConfigNode* node, const std::string& context) {
    if (!node) return;
    
    // Track seen directives for duplicate detection
    std::map<std::string, size_t> seenDirectives;
    
    for (size_t i = 0; i < node->children.size(); ++i) {
        ConfigNode* child = node->children[i];
        if (!child) continue;
        
        // Check for duplicates
        if (isSingleInstanceDirective(child->name)) {
            if (seenDirectives.find(child->name) != seenDirectives.end()) {
                errors_.push_back(ValidationError(STAGE_VALIDATOR, SEVERITY_ERROR, child->line,
                                                  "Duplicate directive: " + child->name, ""));
            } else {
                seenDirectives[child->name] = child->line;
            }
        }
        
        // Validate directive
        validateDirective(child->name, child->args, child->line, context);
        
        // Recursively validate children (for blocks)
        if (!child->children.empty()) {
            validateLocal(child, child->name);
        }
    }
}

void SemanticValidator::validateDirective(const std::string& name, 
                                          const std::vector<std::string>& args,
                                          size_t line, const std::string& /*context*/) {
    if (name == "listen") {
        if (args.empty()) {
            errors_.push_back(ValidationError(STAGE_VALIDATOR, SEVERITY_ERROR, line,
                                              "listen requires a port", ""));
        } else if (!isValidPort(args[0])) {
            errors_.push_back(ValidationError(STAGE_VALIDATOR, SEVERITY_ERROR, line,
                                              "Invalid port: " + args[0], ""));
        }
    } else if (name == "allowed_methods") {
        for (size_t i = 0; i < args.size(); ++i) {
            if (!isValidMethod(args[i])) {
                errors_.push_back(ValidationError(STAGE_VALIDATOR, SEVERITY_ERROR, line,
                                                  "Invalid HTTP method: " + args[i], ""));
            }
        }
    } else if (name == "return") {
        if (args.empty()) {
            errors_.push_back(ValidationError(STAGE_VALIDATOR, SEVERITY_ERROR, line,
                                              "return requires a code", ""));
        } else {
            int code = stringToInt(args[0]);
            if (!isValidRedirectCode(code)) {
                errors_.push_back(ValidationError(STAGE_VALIDATOR, SEVERITY_ERROR, line,
                                                  "Invalid redirect code: " + args[0], ""));
            }
        }
    } else if (name == "client_max_body_size") {
        if (args.empty()) {
            errors_.push_back(ValidationError(STAGE_VALIDATOR, SEVERITY_ERROR, line,
                                              "client_max_body_size requires a value", ""));
        } else {
            // Try to parse size
            size_t size = parseSize(args[0]);
            if (size == 0 && args[0] != "0") {
                errors_.push_back(ValidationError(STAGE_VALIDATOR, SEVERITY_ERROR, line,
                                                  "Invalid size: " + args[0], ""));
            }
        }
    }
    // Add more directive validations as needed
}

void SemanticValidator::validateReferences(ConfigNode* node) {
    // Currently no reference validation needed for basic implementation
    (void)node;
}

void SemanticValidator::buildConfigs(ConfigNode* node, std::vector<ServerConfig>& configs) {
    if (!node) return;
    
    for (size_t i = 0; i < node->children.size(); ++i) {
        ConfigNode* child = node->children[i];
        if (!child) continue;
        
        if (child->name == "server") {
            ServerConfig config;
            buildServerConfig(child, config);
            applyInheritance(config);
            configs.push_back(config);
        }
    }
}

void SemanticValidator::buildServerConfig(ConfigNode* node, ServerConfig& config) {
    if (!node) return;
    
    for (size_t i = 0; i < node->children.size(); ++i) {
        ConfigNode* child = node->children[i];
        if (!child) continue;
        
        if (child->name == "listen") {
            if (!child->args.empty()) {
                Listen l;
                // Parse "host:port" or just "port"
                size_t colonPos = child->args[0].find(':');
                if (colonPos != std::string::npos) {
                    l.host = child->args[0].substr(0, colonPos);
                    l.port = stringToInt(child->args[0].substr(colonPos + 1));
                } else {
                    l.port = stringToInt(child->args[0]);
                }
                config.listens.push_back(l);
            }
        } else if (child->name == "server_name") {
            if (!child->args.empty()) {
                config.server_name = child->args[0];
            }
        } else if (child->name == "root") {
            if (!child->args.empty()) {
                config.root = child->args[0];
            }
        } else if (child->name == "client_max_body_size") {
            if (!child->args.empty()) {
                config.client_max_body_size = parseSize(child->args[0]);
            }
        } else if (child->name == "location") {
            LocationConfig loc;
            if (!child->args.empty()) {
                loc.path = child->args[0];
            }
            buildLocationConfig(child, loc);
            config.locations.push_back(loc);
        }
    }
}

void SemanticValidator::buildLocationConfig(ConfigNode* node, LocationConfig& loc) {
    if (!node) return;
    
    for (size_t i = 0; i < node->children.size(); ++i) {
        ConfigNode* child = node->children[i];
        if (!child) continue;
        
        if (child->name == "allowed_methods") {
            for (size_t j = 0; j < child->args.size(); ++j) {
                loc.methods.insert(child->args[j]);
            }
        } else if (child->name == "root") {
            if (!child->args.empty()) {
                loc.root = child->args[0];
            }
        } else if (child->name == "index") {
            if (!child->args.empty()) {
                loc.index = child->args[0];
            }
        } else if (child->name == "autoindex") {
            if (!child->args.empty()) {
                loc.autoindex = (child->args[0] == "on");
            }
        } else if (child->name == "upload_enabled") {
            if (!child->args.empty()) {
                loc.upload_enabled = (child->args[0] == "on");
            }
        } else if (child->name == "upload_dir") {
            if (!child->args.empty()) {
                loc.upload_dir = child->args[0];
            }
        } else if (child->name == "client_max_body_size") {
            if (!child->args.empty()) {
                loc.client_max_body_size = parseSize(child->args[0]);
            }
        } else if (child->name == "return") {
            if (child->args.size() >= 2) {
                loc.has_redirect = true;
                loc.redirect_code = stringToInt(child->args[0]);
                loc.redirect_target = child->args[1];
            }
        } else if (child->name == "cgi_map") {
            if (child->args.size() >= 2) {
                loc.cgi_map[child->args[0]] = child->args[1];
            }
        }
    }
}

void SemanticValidator::applyInheritance(ServerConfig& server) {
    // Apply default to server if not set
    if (server.client_max_body_size == 0) {
        server.client_max_body_size = ServerConfig::DEFAULT_MAX_BODY_SIZE;
    }
    
    // Inherit to locations
    for (size_t i = 0; i < server.locations.size(); ++i) {
        if (server.locations[i].client_max_body_size == LocationConfig::UNSET_SIZE) {
            server.locations[i].client_max_body_size = server.client_max_body_size;
        }
    }
}

bool SemanticValidator::isValidPort(const std::string& port) {
    int p = stringToInt(port);
    return p > 0 && p <= 65535;
}

bool SemanticValidator::isValidMethod(const std::string& method) {
    return validMethods_.find(method) != validMethods_.end();
}

bool SemanticValidator::isValidRedirectCode(int code) {
    return code >= 300 && code <= 399;
}

bool SemanticValidator::isSingleInstanceDirective(const std::string& name) {
    return singleInstanceDirectives_.find(name) != singleInstanceDirectives_.end();
}

size_t SemanticValidator::parseSize(const std::string& s) {
    if (s.empty()) return 0;
    
    size_t pos = 0;
    long long value = 0;
    
    // Parse number
    while (pos < s.length() && isdigit(s[pos])) {
        value = value * 10 + (s[pos] - '0');
        pos++;
    }
    
    // Parse unit
    if (pos < s.length()) {
        char unit = s[pos];
        switch (unit) {
            case 'k':
            case 'K':
                value *= 1024;
                break;
            case 'm':
            case 'M':
                value *= 1024 * 1024;
                break;
            case 'g':
            case 'G':
                value *= 1024 * 1024 * 1024;
                break;
        }
    }
    
    return static_cast<size_t>(value);
}

int SemanticValidator::stringToInt(const std::string& s) {
    std::stringstream ss(s);
    int result = 0;
    ss >> result;
    return result;
}
