#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>

struct ServerConfig;

struct Connection {
    int                  fd;
    std::string          read_buffer;
    std::string          write_buffer;
    const ServerConfig*  server_config;  // config do bloco server{} que aceitou esta conexão

    Connection() : fd(-1), server_config(NULL) {}
    Connection(int fd, const ServerConfig* cfg) : fd(fd), server_config(cfg) {}
};

#endif