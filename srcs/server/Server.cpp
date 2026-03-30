#include <Server.hpp>
#include <Logger.hpp>
#include <HttpRequest.hpp>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <cstring>
#include <sstream>
#include <iostream>

#define READ_BUFFER_SIZE        4096
#define MAX_HEADER_BUFFER_SIZE  READ_BUFFER_SIZE
#define CONNECTION_TIMEOUT_SEC  5


Server::Server(const Config& config) : _config(config) {
    signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    for (size_t i = 0; i < _listening_fds.size(); ++i) {
        close(_listening_fds[i]);
    }
    for (std::map<int, Connection>::iterator it = _connections.begin();
         it != _connections.end(); ++it) {
        close(it->second.fd);
    }
}

void Server::_setup_listening_sockets() {
    for (size_t i = 0; i < _config.servers.size(); ++i) {
        const ServerConfig& srv = _config.servers[i];
        int fd = _create_listening_socket(srv);
        if (fd < 0) {
            continue;
        }
        _listening_fds.push_back(fd);
        _fd_to_server_config[fd] = &srv;
        _register_fd(fd, POLLIN);

        std::stringstream ss;
        ss << "Listening on " << (srv.host.empty() ? "0.0.0.0" : srv.host)
           << ":" << srv.port;
        Logger::info(ss.str());
    }
}

int Server::_create_listening_socket(const ServerConfig& srv) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        Logger::error("socket() failed for port " + srv.port);
        return -1;
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::error("setsockopt() failed for port " + srv.port);
        close(fd);
        return -1;
    }


    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        Logger::error("fcntl() O_NONBLOCK failed for port " + srv.port);
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    std::stringstream ss(srv.port);
    int port_num;
    ss >> port_num;
    addr.sin_port = htons(static_cast<uint16_t>(port_num));


    if (srv.host.empty() || srv.host == "localhost" || srv.host == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        addr.sin_addr.s_addr = inet_addr(srv.host.c_str());
        if (addr.sin_addr.s_addr == (in_addr_t)(-1)) {
            Logger::error("inet_addr() failed for host: " + srv.host);
            close(fd);
            return -1;
        }
    }

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::error("bind() failed for port " + srv.port + " (port already in use?)");
        close(fd);
        return -1;
    }

    if (listen(fd, SOMAXCONN) < 0) {
        Logger::error("listen() failed for port " + srv.port);
        close(fd);
        return -1;
    }

    return fd; 
}

void Server::_register_fd(int fd, short events) {
    struct pollfd pfd;
    pfd.fd      = fd;
    pfd.events  = events;
    pfd.revents = 0;
    _poll_fds.push_back(pfd);
}

void Server::run() {
    _setup_listening_sockets();

    if (_listening_fds.empty()) {
        Logger::error("No listening sockets could be created. Aborting.");
        return;
    }

    Logger::info("Server started. Waiting for connections...");

    while (true) {
        if (_poll_fds.empty())
            break;

        int ready = poll(&_poll_fds[0], _poll_fds.size(), CONNECTION_TIMEOUT_SEC * 1000);

        if (ready < 0) {
            Logger::error("poll() failed — stopping server");
            break;
        }

        if (ready == 0) {
            _check_timeouts();
            continue;
        }

        size_t current_size = _poll_fds.size();
        for (size_t i = 0; i < current_size && i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].revents == 0)
                continue;

            int fd = _poll_fds[i].fd;

            if (_is_listening_fd(fd)) {
                _accept_new_connection(fd);
            } else {
                if (_poll_fds[i].revents & POLLHUP) {
                    _close_connection(fd);
                    if (i < current_size) { current_size--; i--; }
                } else if (_poll_fds[i].revents & POLLIN) {
                    _read_from_client(fd);
                } else if (_poll_fds[i].revents & POLLOUT) {
                    _write_to_client(fd);
                }
            }
        }

        _check_timeouts();
    }
}

bool Server::_is_listening_fd(int fd) const {
    for (size_t i = 0; i < _listening_fds.size(); ++i) {
        if (_listening_fds[i] == fd)
            return true;
    }
    return false;
}

void Server::_accept_new_connection(int listening_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(listening_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        return;
    }

    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
        Logger::error("fcntl() O_NONBLOCK failed for new client");
        close(client_fd);
        return;
    }

    const ServerConfig* srv_cfg = _fd_to_server_config[listening_fd];
    _connections[client_fd] = Connection(client_fd, srv_cfg);

    _connections[client_fd].parser.setMaxBodySize(srv_cfg->client_max_body_size);

    _register_fd(client_fd, POLLIN);

    Logger::info("New connection accepted");
}

void Server::_read_from_client(int fd) {
    char buf[READ_BUFFER_SIZE];

    ssize_t n = recv(fd, buf, sizeof(buf), 0);

    if (n == 0) {
        _close_connection(fd);
        return;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        _close_connection(fd);
        return;
    }

    Connection& conn = _connections[fd];
    conn.last_activity = time(NULL);
    std::string data(buf, static_cast<size_t>(n));

    conn.parser.feed(data);

    if (!conn.parser.isComplete()) {
        return;
    }

    HttpRequest req = conn.parser.getRequest();

    if (req.errorCode == 413) {
        const std::string response413 =
            "HTTP/1.1 413 Payload Too Large\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 17\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Payload Too Large";
        conn.write_buffer = response413;
        conn.keep_alive = false;
        _set_pollout(fd, true);
        return;
    }

    conn.keep_alive = (conn.read_buffer.find("Connection: close") == std::string::npos);
    const std::string conn_header = conn.keep_alive ? "keep-alive" : "close";

    const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 18\r\n"
        "Connection: " + conn_header + "\r\n"
        "\r\n"
        "Hello, World test!";

    conn.write_buffer = response;
    _set_pollout(fd, true);
}

void Server::_write_to_client(int fd) {
    Connection& conn = _connections[fd];

    if (conn.write_buffer.empty()) {
        _set_pollout(fd, false);
        return;
    }

    ssize_t n = send(fd, conn.write_buffer.c_str(), conn.write_buffer.size(), 0);

    if (n > 0) {
        conn.write_buffer.erase(0, static_cast<size_t>(n));
    } else if (n < 0) {
        _close_connection(fd);
        return;
    }

    if (conn.write_buffer.empty()) {
        _set_pollout(fd, false);
        if (conn.keep_alive) {
            conn.read_buffer.clear();
            conn.last_activity = time(NULL);
        } else {
            _close_connection(fd);
        }
    }
}

void Server::_close_connection(int fd) {
    shutdown(fd, SHUT_RDWR);
    close(fd);
    _connections.erase(fd);

    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        if (_poll_fds[i].fd == fd) {
            _poll_fds.erase(_poll_fds.begin() + static_cast<long>(i));
            break;
        }
    }

    Logger::info("Connection closed");
}

void Server::_set_pollout(int fd, bool enable) {
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        if (_poll_fds[i].fd == fd) {
            if (enable)
                _poll_fds[i].events |= POLLOUT;
            else
                _poll_fds[i].events &= ~POLLOUT;
            return;
        }
    }
}

void Server::_check_timeouts() {
    time_t now = time(NULL);
    std::vector<int> to_close;

    for (std::map<int, Connection>::iterator it = _connections.begin();
         it != _connections.end(); ++it) {
        if (now - it->second.last_activity > CONNECTION_TIMEOUT_SEC) {
            to_close.push_back(it->first);
        }
    }

    for (size_t i = 0; i < to_close.size(); ++i) {
        Logger::info("Closing idle connection (timeout)");
        _close_connection(to_close[i]);
    }
}