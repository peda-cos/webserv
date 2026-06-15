#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <vector>
#include <ctime>

#include <HttpRequestParser.hpp>

struct ServerConfig;

enum CgiState {
    CGI_NONE,
    CGI_WRITING_IN,
    CGI_READING_OUT,
    CGI_WAITING_FOR_EXIT
};

struct Connection {
    int                  fd;
    std::string          read_buffer;
    std::string          write_buffer;
    std::vector<const ServerConfig*> server_configs;
    time_t               last_activity;
    bool                 keep_alive;
    bool                 headers_complete;
    HttpRequestParser    parser;

    pid_t                cgi_pid;
    int                  cgi_in_fd;
    int                  cgi_out_fd;
    CgiState             cgi_state;
    size_t               cgi_body_bytes_sent;
    std::string          cgi_response_raw;
    HttpRequest          pending_req;
    time_t               cgi_start_time;

    Connection() : fd(-1), last_activity(0), keep_alive(false), headers_complete(false),
                  cgi_pid(-1), cgi_in_fd(-1), cgi_out_fd(-1), cgi_state(CGI_NONE),
                  cgi_body_bytes_sent(0), cgi_start_time(0) {}
    Connection(int fd, const std::vector<const ServerConfig*>& cfgs)
        : fd(fd), server_configs(cfgs), last_activity(time(NULL)), keep_alive(false), headers_complete(false),
          cgi_pid(-1), cgi_in_fd(-1), cgi_out_fd(-1), cgi_state(CGI_NONE),
          cgi_body_bytes_sent(0), cgi_start_time(0) {}
};

#endif