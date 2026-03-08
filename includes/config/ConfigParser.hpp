#pragma once

#include <string>

class ConfigParser {
    private:
        std::string source;
    public:
        ConfigParser(const std::string& source);

        void parse();
};