#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <ctime>

struct ServerConfig;

struct Connection {
    int                  fd;
    std::string          read_buffer;
    std::string          write_buffer;
    const ServerConfig*  server_config;
    time_t               last_activity;
    bool                 keep_alive;

    Connection() : fd(-1), server_config(NULL), last_activity(0), keep_alive(false) {}
    Connection(int fd, const ServerConfig* cfg)
        : fd(fd), server_config(cfg), last_activity(time(NULL)), keep_alive(false) {}
};

#endif