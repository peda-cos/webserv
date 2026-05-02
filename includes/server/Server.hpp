#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <string>
#include <poll.h>

#include <Config.hpp>
#include <ServerConfig.hpp>
#include <LocationConfig.hpp>
#include <Connection.hpp>
#include <CgiHandler.hpp>
#include <RequestRouter.hpp>

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
        std::map<int, int> _cgi_fd_to_client_fd;

        std::vector<struct pollfd> _poll_fds;

        void _setup_listening_sockets();
        int  _create_listening_socket(const ServerConfig& server_config);
        void _register_fd(int fd, short events);

        bool _is_listening_fd(int fd) const;
        void _accept_new_connection(int listening_fd);
        void _read_from_client(int fd);
        void _write_to_client(int fd);
        void _handle_cgi_input(int cgi_fd);
        void _handle_cgi_output(int cgi_fd);
        void _finalize_cgi_response(int client_fd, int status, pid_t pid_res);
        void _cleanup_cgi(int client_fd);
        void _prepare_for_next_request(int fd);
        void _close_connection(int fd);
        void _remove_from_poll(int fd);
        void _set_pollout(int fd, bool enable);
        void _check_timeouts();
        void _check_cgi_exits();
        bool _queue_parsed_request_response(int fd);
        std::string _serve_static_response(const HttpRequest& req, const std::string& physicalPath, const LocationConfig& loc, const std::string& conn_header) const;
        std::string _generate_directory_listing(const std::string& physicalPath) const;


};


#endif
