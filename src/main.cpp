#include <iostream>
#include <vector>
#include "../includes/core/EventLoop.hpp"
#include "../includes/core/Listener.hpp"
#include "../includes/config/ConfigLoader.hpp"

int main(int argc, char** argv) {
    std::string configPath = (argc > 1) ? argv[1] : "config/webserv.conf";
    
    std::vector<ServerConfig> configs;
    std::string error;
    
    if (!ConfigLoader::load(configPath, configs, error)) {
        std::cerr << "Configuration error:\n" << error << std::endl;
        return 1;
    }
    
    EventLoop loop;
    std::vector<Listener*> listeners;
    size_t validListeners = 0;
    
    for (size_t i = 0; i < configs.size(); ++i) {
        for (size_t j = 0; j < configs[i].listens.size(); ++j) {
            Listener* listener = new Listener(
                configs[i].listens[j].host,
                configs[i].listens[j].port
            );
            if (listener->is_valid()) {
                loop.add_listener(listener);
                validListeners++;
            }
            listeners.push_back(listener);
        }
    }
    
    if (validListeners == 0) {
        std::cerr << "No valid listeners configured" << std::endl;
        for (size_t i = 0; i < listeners.size(); ++i) {
            delete listeners[i];
        }
        return 1;
    }
    
    loop.run();
    
    // Cleanup listeners
    for (size_t i = 0; i < listeners.size(); ++i) {
        delete listeners[i];
    }
    
    return 0;
}
