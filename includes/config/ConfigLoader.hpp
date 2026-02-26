#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include <string>
#include <vector>
#include "../../includes/types/ServerConfig.hpp"

class ConfigLoader {
public:
    static bool load(const std::string& filepath,
                     std::vector<ServerConfig>& configs,
                     std::string& error);
};

#endif
