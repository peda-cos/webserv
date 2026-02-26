#ifndef CONFIG_NODE_HPP
#define CONFIG_NODE_HPP

#include <string>
#include <vector>

class ConfigNode {
public:
    std::string name;
    std::vector<std::string> args;
    std::vector<ConfigNode*> children;
    ConfigNode* parent;
    size_t line;

    ConfigNode(ConfigNode* p = NULL) : parent(p), line(0) {}
    ~ConfigNode();

    void addChild(ConfigNode* child);
    bool isBlock() const { return !children.empty(); }
    bool isDirective() const { return children.empty(); }
};

#endif
