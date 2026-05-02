#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <ctime>

#include <HttpRequestParser.hpp>

struct ServerConfig;

enum CgiState {
    CGI_NONE,
    CGI_WRITING_IN,
    CGI_READING_OUT,
    CGI_WAITING_FOR_EXIT,
    CGI_COMPLETE
};

struct Connection {
    int                  fd;
    std::string          read_buffer;
    std::string          write_buffer;
    const ServerConfig*  server_config;
    time_t               last_activity;
    bool                 keep_alive;
    HttpRequestParser    parser;

    pid_t                cgi_pid;
    int                  cgi_in_fd;
    int                  cgi_out_fd;
    CgiState             cgi_state;
    size_t               cgi_body_bytes_sent;
    std::string          cgi_response_raw;
    HttpRequest          pending_req;
    time_t               cgi_start_time;

    Connection() : fd(-1), server_config(NULL), last_activity(0), keep_alive(false), 
                  cgi_pid(-1), cgi_in_fd(-1), cgi_out_fd(-1), cgi_state(CGI_NONE), 
                  cgi_body_bytes_sent(0), cgi_start_time(0) {}
    Connection(int fd, const ServerConfig* cfg)
        : fd(fd), server_config(cfg), last_activity(time(NULL)), keep_alive(false),
          cgi_pid(-1), cgi_in_fd(-1), cgi_out_fd(-1), cgi_state(CGI_NONE), 
          cgi_body_bytes_sent(0), cgi_start_time(0) {}
};

#endif