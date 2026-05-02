#include <Server.hpp>
#include <Logger.hpp>
#include <HttpRequest.hpp>
#include <HttpResponse.hpp>
#include <StringUtils.hpp>
#include <HttpResponseBuilder.hpp>
#include <MimeTypes.hpp>
#include <RequestRouter.hpp>
#include <CgiUtils.hpp>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#include <sys/wait.h>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iostream>

#define READ_BUFFER_SIZE        4096
#define MAX_HEADER_BUFFER_SIZE  READ_BUFFER_SIZE
#define CONNECTION_TIMEOUT_SEC  5

namespace {
    void append_cors_headers(HttpResponseBuilder& builder) {
        builder.setHeader("Access-Control-Allow-Origin", "*");
        builder.setHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, HEAD, PUT, OPTIONS");
        builder.setHeader("Access-Control-Allow-Headers", "*");
    }

    std::string build_cgi_error(int code, const std::string& message, const std::string& extra = "") {
        HttpResponseBuilder builder;
        std::string reason = HttpResponseBuilder::reasonPhraseFor(code);
        std::string body = "<html><body style='font-family:sans-serif;'><h1>" + StringUtils::to_string(code) + " " + reason + "</h1>";
        if (!message.empty()) {
            body += "<p>" + message + "</p>";
        }
        if (!extra.empty()) {
            body += "<div style='background:#f8f9fa;padding:15px;border-left:5px solid #dc3545;margin-top:20px;'>";
            body += "<strong>Partial CGI Output:</strong><pre style='white-space:pre-wrap;'>" + extra + "</pre></div>";
        }
        body += "</body></html>";
        builder.setStatus(code)
               .setContentType("text/html")
               .setBody(body)
               .setConnection("close");
        append_cors_headers(builder);
        return builder.build();
    }
}


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

        int ready = poll(&_poll_fds[0], _poll_fds.size(), 10);

        if (ready < 0) {
            Logger::error("poll failed — stopping server");
            break;
        }

        _check_timeouts();
        _check_cgi_exits();

        if (ready == 0)
            continue;

        size_t current_size = _poll_fds.size();
        for (size_t i = 0; i < current_size && i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].revents == 0)
                continue;

            int fd = _poll_fds[i].fd;

            if (_is_listening_fd(fd)) {
                _accept_new_connection(fd);
            } else if (_cgi_fd_to_client_fd.count(fd)) {
                if (_poll_fds[i].revents & POLLIN) {
                    _handle_cgi_output(fd);
                } else if (_poll_fds[i].revents & POLLOUT) {
                    _handle_cgi_input(fd);
                } else if (_poll_fds[i].revents & (POLLHUP | POLLERR)) {
                    _handle_cgi_output(fd);
                }
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
    conn.read_buffer += data;

    if (conn.read_buffer.size() > MAX_HEADER_BUFFER_SIZE && !conn.parser.isComplete()) {
        _close_connection(fd);
        return;
    }

    conn.parser.feed(data);

    if (!conn.parser.isComplete()) {
        return;
    }

    if (!conn.write_buffer.empty()) {
        return;
    }

    if (conn.cgi_pid == -1) {
        _queue_parsed_request_response(fd);
    }
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
            conn.last_activity = time(NULL);
            if (!conn.parser.isComplete()) {
                return;
            }

            _queue_parsed_request_response(fd);
        } else {
            _close_connection(fd);
        }
    }
}

void Server::_cleanup_cgi(int fd) {
    Connection& conn = _connections[fd];
    if (conn.cgi_pid != -1) {
        kill(conn.cgi_pid, SIGKILL);
        waitpid(conn.cgi_pid, NULL, WNOHANG);
        conn.cgi_pid = -1;
    }
    if (conn.cgi_in_fd != -1) {
        _cgi_fd_to_client_fd.erase(conn.cgi_in_fd);
        _remove_from_poll(conn.cgi_in_fd);
        close(conn.cgi_in_fd);
        conn.cgi_in_fd = -1;
    }
    if (conn.cgi_out_fd != -1) {
        _remove_from_poll(conn.cgi_out_fd);
        _cgi_fd_to_client_fd.erase(conn.cgi_out_fd);
        close(conn.cgi_out_fd);
        conn.cgi_out_fd = -1;
    }
    conn.cgi_state = CGI_NONE;
    conn.cgi_response_raw = "";
    conn.cgi_body_bytes_sent = 0;
}

void Server::_prepare_for_next_request(int fd) {
    Connection& conn = _connections[fd];
    _cleanup_cgi(fd);
    conn.parser.reset();
    conn.pending_req = HttpRequest();
}

void Server::_remove_from_poll(int fd) {
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        if (_poll_fds[i].fd == fd) {
            _poll_fds.erase(_poll_fds.begin() + static_cast<long>(i));
            break;
        }
    }
}

void Server::_close_connection(int fd) {
    if (_connections.count(fd)) {
        _cleanup_cgi(fd);
    }
    shutdown(fd, SHUT_RDWR);
    close(fd);
    _connections.erase(fd);
    _remove_from_poll(fd);
    for (size_t i = 0; i < _poll_fds.size(); ) {
        if (_cgi_fd_to_client_fd.count(_poll_fds[i].fd) == 0 && 
            !_is_listening_fd(_poll_fds[i].fd) && 
            _connections.count(_poll_fds[i].fd) == 0) {
             _poll_fds.erase(_poll_fds.begin() + static_cast<long>(i));
        } else {
            i++;
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
        Connection& conn = it->second;
        if (conn.cgi_pid != -1) {
            if (now - conn.cgi_start_time > CGI_TIMEOUT_SECONDS) {
                to_close.push_back(it->first);
            }
        } else if (now - conn.last_activity > CONNECTION_TIMEOUT_SEC) {
            to_close.push_back(it->first);
        }
    }

    for (size_t i = 0; i < to_close.size(); ++i) {
        Connection& conn = _connections[to_close[i]];
        if (conn.cgi_pid != -1) {
            Logger::info("CGI execution timed out (504)");
            conn.write_buffer = build_cgi_error(504, "CGI execution timed out");
            _set_pollout(to_close[i], true);
            conn.keep_alive = false;
            _prepare_for_next_request(to_close[i]);
            conn.last_activity = now;
        } else {
            Logger::info("Closing idle connection (timeout)");
            _close_connection(to_close[i]);
        }
    }
}

bool Server::_queue_parsed_request_response(int fd) {
    Connection& conn = _connections[fd];
    HttpRequest req = conn.parser.getRequest();
    std::string remainder = conn.parser.getRemainder();
    bool shouldClose = false;

    if (req.httpVersion == "1.0") {
        shouldClose = true;
    }

    std::map<std::string, std::string>::const_iterator connectionIt = req.headers.find("connection");
    if (connectionIt != req.headers.end()) {
        std::string value = StringUtils::to_lower(connectionIt->second);
        if (value.find("close") != std::string::npos) {
            shouldClose = true;
        } else if (req.httpVersion == "1.0" && value.find("keep-alive") != std::string::npos) {
            shouldClose = false;
        }
    }

    conn.parser.reset();
    if (!remainder.empty()) {
        conn.parser.feed(remainder);
    }

    conn.keep_alive = !shouldClose;

    const std::string conn_header = conn.keep_alive ? "keep-alive" : "close";
    if (req.errorCode != 0) {
        conn.keep_alive = false;
        conn.write_buffer = HttpResponseBuilder::buildErrorPage(req.errorCode, "");
    } else if (!conn.server_config) {
        Logger::error("No server config for connection");
        conn.write_buffer = HttpResponseBuilder::buildErrorPage(500, "");
    } else {
        const LocationConfig* loc = RequestRouter::matchLocation(*conn.server_config, req.path);
        if (!loc) {
            conn.write_buffer = HttpResponseBuilder::buildErrorPage(404, "");
        } else if (!RequestRouter::isMethodAllowed(req.method, *loc)) {
            conn.write_buffer = HttpResponseBuilder::buildErrorPage(405, "");
        } else {
            std::string physicalPath = RequestRouter::resolvePhysicalPath(*loc, req.path, *conn.server_config);
            if (RequestRouter::isPathTraversal(physicalPath)) {
                conn.write_buffer = HttpResponseBuilder::buildErrorPage(403, "");
            } else {
                RequestRouter::TargetType target = RequestRouter::classifyRequest(physicalPath, *loc);
                if (target == RequestRouter::TARGET_CGI) {
                    Logger::info("CGI Request: " + req.method + " " + req.uri + " (Resolved: " + physicalPath + ")");
                    if (conn.cgi_pid != -1) {
                        _cleanup_cgi(fd);
                    }
                    CgiHandler cgi_handler;
                    CgiProcessInfo cgi_info;
                    int status_code = cgi_handler.start_cgi(req, *conn.server_config, cgi_info);
                    
                    if (status_code != 0) {
                        conn.write_buffer = build_cgi_error(status_code, "");
                    } else {
                        conn.cgi_pid = cgi_info.pid;
                        conn.cgi_in_fd = cgi_info.stdin_fd;
                        conn.cgi_out_fd = cgi_info.stdout_fd;
                        conn.pending_req = req;
                        conn.cgi_response_raw = "";
                        conn.cgi_body_bytes_sent = 0;
                        conn.cgi_start_time = time(NULL);

                        if (req.method == "POST" && !req.body.empty()) {
                            conn.cgi_state = CGI_WRITING_IN;
                            _register_fd(conn.cgi_in_fd, POLLOUT);
                            _cgi_fd_to_client_fd[conn.cgi_in_fd] = fd;
                        } else {
                            if (conn.cgi_in_fd != -1) {
                                close(conn.cgi_in_fd);
                                conn.cgi_in_fd = -1;
                            }
                            conn.cgi_state = CGI_READING_OUT;
                        }
                        
                        _register_fd(conn.cgi_out_fd, POLLIN);
                        _cgi_fd_to_client_fd[conn.cgi_out_fd] = fd;
                        
                        _set_pollout(fd, false);
                        return true; 
                    }
                } else {
                    conn.write_buffer = _serve_static_response(req, physicalPath, *loc, conn_header);
                }
            }
        }
    }

    conn.parser.reset();
    conn.parser.setMaxBodySize(conn.server_config->client_max_body_size);
    if (conn.keep_alive && !remainder.empty()) {
        conn.parser.feed(remainder);
    }

    _set_pollout(fd, true);
    return true;
}

std::string Server::_generate_directory_listing(const std::string& physicalPath) const {
    DIR* dir = opendir(physicalPath.c_str());
    if (!dir) {
        return "";
    }

    std::ostringstream html;
    html << "<html><head><title>Index of " << physicalPath << "</title></head><body>\n";
    html << "<h1>Index of " << physicalPath << "</h1><hr><pre>\n";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }

        std::string full_path = physicalPath + "/" + name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            name += "/";
        }

        html << "<a href=\"" << name << "\">" << name << "</a>\n";
    }

    closedir(dir);
    html << "</pre><hr></body></html>";
    return html.str();
}

std::string Server::_serve_static_response(const HttpRequest& req, const std::string& physicalPath, const LocationConfig& loc, const std::string& conn_header) const {
    if (req.method == "OPTIONS") {
        HttpResponseBuilder builder;
        builder.setStatus(204).setConnection(conn_header);
        append_cors_headers(builder);
        return builder.build();
    }

    if (req.method == "GET" || req.method == "HEAD") {
        struct stat st;
        if (stat(physicalPath.c_str(), &st) != 0) {
            return HttpResponseBuilder::buildErrorPage(404, "");
        }

        if (S_ISDIR(st.st_mode)) {
            if (!loc.index.empty()) {
                std::string indexPath = CgiUtils::join_paths(physicalPath, loc.index);
                std::ifstream indexFile(indexPath.c_str(), std::ios::binary);
                if (indexFile.is_open()) {
                    std::ostringstream ss;
                    ss << indexFile.rdbuf();
                    std::string content = ss.str();
                    HttpResponseBuilder builder;
                    builder.setStatus(200)
                           .setBody(content)
                           .setContentType(MimeTypes::getType(indexPath))
                           .setConnection(conn_header);
                    append_cors_headers(builder);
                    return builder.build();
                }
            }

            if (loc.autoindex == ON) {
                std::string listing = _generate_directory_listing(physicalPath);
                if (!listing.empty()) {
                    HttpResponseBuilder builder;
                    builder.setStatus(200)
                           .setBody(listing)
                           .setContentType("text/html")
                           .setConnection(conn_header);
                    append_cors_headers(builder);
                    return builder.build();
                }
            }

            return HttpResponseBuilder::buildErrorPage(403, "");
        }

        if (S_ISREG(st.st_mode)) {
            std::ifstream file(physicalPath.c_str(), std::ios::binary);
            if (file.is_open()) {
                std::ostringstream ss;
                ss << file.rdbuf();
                std::string content = ss.str();
                HttpResponseBuilder builder;
                builder.setStatus(200)
                       .setBody(content)
                       .setContentType(MimeTypes::getType(physicalPath))
                       .setConnection(conn_header);
                append_cors_headers(builder);
                return builder.build();
            }
        }

        return HttpResponseBuilder::buildErrorPage(404, "");
    }

    if (req.method == "POST") {
        if (loc.upload_store.empty()) {
            return HttpResponseBuilder::buildErrorPage(403, "");
        }

        struct stat st;
        if (stat(loc.upload_store.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
            return HttpResponseBuilder::buildErrorPage(500, "");
        }

        std::string filename = "upload_" + StringUtils::to_string(static_cast<int>(time(NULL))) + ".txt";
        std::string uploadPath = CgiUtils::join_paths(loc.upload_store, filename);

        std::ofstream out(uploadPath.c_str(), std::ios::binary);
        if (!out.is_open()) {
            return HttpResponseBuilder::buildErrorPage(500, "");
        }
        out.write(req.body.c_str(), static_cast<std::streamsize>(req.body.length()));
        out.close();

        HttpResponseBuilder builder;
        builder.setStatus(201)
               .setBody("Created")
               .setContentType("text/plain")
               .setConnection(conn_header);
        append_cors_headers(builder);
        return builder.build();
    }

    if (req.method == "DELETE") {
        struct stat st;
        if (stat(physicalPath.c_str(), &st) != 0) {
            return HttpResponseBuilder::buildErrorPage(404, "");
        }

        if (!S_ISREG(st.st_mode)) {
            return HttpResponseBuilder::buildErrorPage(404, "");
        }

        if (std::remove(physicalPath.c_str()) != 0) {
            return HttpResponseBuilder::buildErrorPage(500, "");
        }

        HttpResponseBuilder builder;
        builder.setStatus(204).setConnection(conn_header);
        append_cors_headers(builder);
        return builder.build();
    }

    return HttpResponseBuilder::buildErrorPage(405, "");
}


void Server::_handle_cgi_input(int cgi_fd) {
    int client_fd = _cgi_fd_to_client_fd[cgi_fd];
    Connection& conn = _connections[client_fd];

    const std::string& body = conn.pending_req.body;
    size_t to_send = body.size() - conn.cgi_body_bytes_sent;

    if (to_send > 0) {
        ssize_t n = write(cgi_fd, body.c_str() + conn.cgi_body_bytes_sent, to_send);
        if (n > 0) {
            conn.cgi_body_bytes_sent += static_cast<size_t>(n);
        } else if (n < 0) {
            Logger::error("Failed to write to CGI stdin");
            _close_connection(client_fd);
            return;
        }
    }

    if (conn.cgi_body_bytes_sent >= body.size()) {
        _cgi_fd_to_client_fd.erase(cgi_fd);
        _remove_from_poll(cgi_fd);
        close(cgi_fd);
        conn.cgi_in_fd = -1;
        conn.cgi_state = CGI_READING_OUT;
    }
}

void Server::_handle_cgi_output(int cgi_fd) {
    int client_fd = _cgi_fd_to_client_fd[cgi_fd];
    Connection& conn = _connections[client_fd];

    char buf[READ_BUFFER_SIZE];
    ssize_t n = read(cgi_fd, buf, sizeof(buf));

    if (n > 0) {
        conn.cgi_response_raw.append(buf, static_cast<size_t>(n));
    } else if (n == 0) {
        close(cgi_fd);
        _cgi_fd_to_client_fd.erase(cgi_fd);
        conn.cgi_out_fd = -1;
        _remove_from_poll(cgi_fd);
        conn.cgi_state = CGI_WAITING_FOR_EXIT;
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::error("Failed to read from CGI stdout");
            _close_connection(client_fd);
        }
    }
}

void Server::_finalize_cgi_response(int client_fd, int status, pid_t res) {
    Connection& conn = _connections[client_fd];
    
    if (res == 0) return;

    if (res > 0) {
        conn.cgi_pid = -1;
        if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) || WIFSIGNALED(status)) {
            std::string msg = "CGI Execution Failed";
            if (WIFEXITED(status))
                msg = "CGI script exited with code " + StringUtils::to_string(WEXITSTATUS(status));
            
            Logger::warn(msg);
            conn.write_buffer = build_cgi_error(500, msg, conn.cgi_response_raw);
            _set_pollout(client_fd, true);
            _prepare_for_next_request(client_fd);
            return;
        }
    } else {
        conn.cgi_pid = -1;
    }

    CgiHandler cgi_handler;
    CgiParsedOutput cgi_output = cgi_handler.parse_cgi_output(conn.cgi_response_raw, CgiResult::SUCCESS);
    
    std::vector< std::pair<std::string, std::string> > response_headers;
    std::string content_type = "";
    for (size_t i = 0; i < cgi_output.headers.size(); ++i) {
        std::string key_lower = StringUtils::to_lower(cgi_output.headers[i].first);
        if (key_lower == "status" || key_lower == "content-length" || key_lower == "connection") {
            continue;
        }
        if (key_lower == "content-type") {
            content_type = cgi_output.headers[i].second;
            continue;
        }
        response_headers.push_back(cgi_output.headers[i]);
    }

    HttpResponseBuilder builder;
    builder.setStatus(cgi_output.status_code)
           .setBody(cgi_output.body)
           .setConnection(conn.keep_alive ? "keep-alive" : "close");

    if (!content_type.empty()) {
        builder.setContentType(content_type);
    } else if (cgi_output.status_code != 204) {
        builder.setContentType("text/html");
    }

    for (size_t i = 0; i < response_headers.size(); ++i) {
        builder.setHeader(response_headers[i].first, response_headers[i].second);
    }

    append_cors_headers(builder);
    conn.write_buffer = builder.build();
    _set_pollout(client_fd, true);
    _prepare_for_next_request(client_fd);
}

void Server::_check_cgi_exits() {
    for (std::map<int, Connection>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
        Connection& conn = it->second;
        if (conn.cgi_pid != -1 && conn.cgi_state == CGI_WAITING_FOR_EXIT) {
            int status = 0;
            pid_t res = waitpid(conn.cgi_pid, &status, WNOHANG);
            if (res != 0) {
                _finalize_cgi_response(it->first, status, res);
            }
        }
    }
}
