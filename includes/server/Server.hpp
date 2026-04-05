#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <string>
#include <poll.h>

#include <Config.hpp>
#include <ServerConfig.hpp>
#include <Connection.hpp>

class Server {
    public:
        explicit Server(const Config& config);
        ~Server();
        void run();

    private:
        const Config& _config;
        std::vector<int> _listening_fds;
        std::map<int, const ServerConfig*> _fd_to_server_config;
        std::map<int, Connection> _connections;

        std::vector<struct pollfd> _poll_fds;

        void _setup_listening_sockets();
        int  _create_listening_socket(const ServerConfig& server_config);
        void _register_fd(int fd, short events);

        bool _is_listening_fd(int fd) const;
        void _accept_new_connection(int listening_fd);
        void _read_from_client(int fd);
        void _write_to_client(int fd);
        void _close_connection(int fd);
        void _set_pollout(int fd, bool enable);
        void _check_timeouts();
        bool _queue_parsed_request_response(int fd);
        std::string _serve_static_with_cors(const HttpRequest& req, const std::string& conn_header) const;
        std::string _build_error_response(int statusCode, bool closeConnection) const;
        std::string _reason_phrase(int statusCode) const;


};


#endif
