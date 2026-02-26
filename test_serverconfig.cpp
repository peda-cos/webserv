#include "types/ServerConfig.hpp"
#include <iostream>
#include <cassert>

int main() {
    // Test Listen struct
    Listen listen;
    assert(listen.host == "0.0.0.0");
    assert(listen.port == 80);
    
    // Test LocationConfig struct
    LocationConfig loc;
    assert(loc.autoindex == false);
    assert(loc.upload_enabled == false);
    assert(loc.has_redirect == false);
    assert(loc.redirect_code == 0);
    assert(loc.client_max_body_size == LocationConfig::UNSET_SIZE);
    
    // Test ServerConfig struct
    ServerConfig srv;
    assert(srv.client_max_body_size == ServerConfig::DEFAULT_MAX_BODY_SIZE);
    assert(srv.client_max_body_size == 1048576);
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
