#include "../../includes/config/Parser.hpp"

Parser::Parser(Lexer& lexer) : lexer_(lexer) {}

ConfigNode* Parser::parse() {
    ConfigNode* root = new ConfigNode(NULL);
    parseBlock(root);
    return errors_.empty() ? root : NULL;
}

void Parser::parseBlock(ConfigNode* parent) {
    while (true) {
        Token t = lexer_.nextToken();

        if (t.type == TK_EOF || t.type == TK_CURLY_CLOSE)
            return;

        if (t.type == TK_WORD) {
            ConfigNode* node = new ConfigNode(parent);
            node->name = t.content;
            node->line = t.line;

            // Collect arguments
            while (true) {
                Token next = lexer_.nextToken();
                if (next.type == TK_WORD) {
                    node->args.push_back(next.content);
                } else if (next.type == TK_SEMICOLON) {
                    parent->addChild(node);
                    break;
                } else if (next.type == TK_CURLY_OPEN) {
                    parent->addChild(node);
                    parseBlock(node);  // nested block
                    break;
                } else {
                    reportError("Expected ; or {", next.line);
                    delete node;
                    return;
                }
            }
        } else if (t.type == TK_ERROR) {
            reportError("Lexer error", t.line);
            return;
        } else {
            reportError("Unexpected token", t.line);
            return;
        }
    }
}

void Parser::reportError(const std::string& msg, size_t line) {
    std::stringstream ss;
    ss << "Parse error at line " << line << ": " << msg;
    errors_.push_back(ss.str());
}

const std::vector<std::string>& Parser::getErrors() const {
    return errors_;
}
