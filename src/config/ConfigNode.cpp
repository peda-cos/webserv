#include "../../includes/config/ConfigNode.hpp"

ConfigNode::~ConfigNode() {
    for (size_t i = 0; i < children.size(); ++i) {
        delete children[i];
    }
}

void ConfigNode::addChild(ConfigNode* child) {
    if (child) {
        child->parent = this;
        children.push_back(child);
    }
}
