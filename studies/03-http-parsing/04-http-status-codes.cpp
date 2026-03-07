//http-status-codes.cpp — Servidor focando em validação e Códigos de Status (Erros Custom)
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o http_status 05-http-status-codes.cpp
//
// TESTAR NO NAVEGADOR:
//   http://localhost:8080/             -> 200 OK (Raiz - Index HTML gerado)
//   http://localhost:8080/qualquercoisa -> 404 Not Found Customizado (Lê erro404.html)
//
// TESTAR NO TERMINAL (MÉTODOS NÃO PERMITIDOS):
//   curl -X PUT http://localhost:8080/  -> 405 Method Not Allowed
//   curl -v -X POST http://localhost:8080/ -> 413 Payload Too Large (Simulando Bloqueio)
//
// CONCEITO DEMONSTRADO:
//   - Geração de páginas de erro parametrizadas do File System (error_page)
//   - Bloqueio de métodos indesejados e validação preventiva na requisição
//   - Resiliência estruturada dos HTTP Codes

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static volatile bool g_running = true;
static void handle_signal(int) { g_running = false; }

struct HttpRequest {
    std::string method, path, version, body;
    std::map<std::string, std::string> headers;
    int status_code; // Guardará falhas detectadas ainda no Parse!
};

struct Client {
    int fd;
    std::string recv_buffer, send_buffer;
};

// ── Helpers ──────────────────────────────────────────────────────────────────
static std::string to_lower(const std::string& s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)tolower((unsigned char)r[i]);
    return r;
}

static std::string get_mime_type(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) return "application/octet-stream";
    std::string ext = to_lower(path.substr(pos));
    
    if (ext == ".html") return "text/html";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".png")  return "image/png";
    return "text/plain";
}

// ── Gestor de Error Pages ────────────────────────────────────────────────────
// Puxa o erro nativo do C++ se o admin não configurou um HTML customizado pro 404
static std::string build_error_response(int status, const std::string& msg, const std::string& custom_file = "") {
    std::string body;
    std::ostringstream response;
    
    if (!custom_file.empty()) {
        std::ifstream file(custom_file.c_str(), std::ios::binary);
        if (file.is_open()) {
            body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
        }
    }
    
    // Fallback: se o arquivo de erro custom não abrir, crie um no punho
    if (body.empty()) {
        std::ostringstream default_html;
        default_html << "<html><body style='text-align:center;'><h1>" << status << " " << msg << "</h1><hr><p>Webserv Nativo</p></body></html>";
        body = default_html.str();
    }
    
    response << "HTTP/1.1 " << status << " " << msg << "\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;
             
    return response.str();
}

// ── Parser com Validação Preventiva de Status ────────────────────────────────
static bool parse_request(const std::string& buffer, HttpRequest& req) {
    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) return false;
    
    req.status_code = 200; // Começa inocente
    
    size_t line_end = buffer.find("\r\n");
    std::string request_line = buffer.substr(0, line_end);
    size_t pos1 = request_line.find(' '), pos2 = request_line.find(' ', pos1 + 1);
    
    if (pos1 == std::string::npos || pos2 == std::string::npos) {
        req.status_code = 400; // Sintaxe da Request Line quebrada
        return true;
    }
    
    req.method = request_line.substr(0, pos1);
    req.path = request_line.substr(pos1 + 1, pos2 - pos1 - 1);
    req.version = request_line.substr(pos2 + 1);

    // Validação de métodos baseada no nosso ".conf imaginário"
    if (req.method == "PUT" || req.method == "PATCH" || req.method == "OPTIONS") {
        req.status_code = 405; // Method Not Allowed
    }
    else if (req.method == "POST") {
        // Simulando que o bloco local bloqueou POST para proteção
        req.status_code = 413; // Payload Too Large // Limitado a 0 bytes
    }
    
    return true; // Terminou o Parse (mesmo que com erro)
}

// ── Montagem da Resposta Lógica ──────────────────────────────────────────────
static std::string build_response(const HttpRequest& req) {
    // 1. O erro já bateu no bloco de parsing?
    if (req.status_code == 400) return build_error_response(400, "Bad Request");
    if (req.status_code == 405) return build_error_response(405, "Method Not Allowed");
    if (req.status_code == 413) return build_error_response(413, "Payload Too Large");

    std::string base_dir = "./www";
    std::string file_path = req.path;
    if (file_path == "/") file_path = "/index.html";
    
    std::string full_path = base_dir + file_path;
    std::cout << "[ROTEAMENTO] Arquivo procurado: " << full_path << std::endl;

    struct stat st;
    
    // 2. Erro de Caminho 404 e Injeção de "error_page"
    if (stat(full_path.c_str(), &st) < 0) {
        // Imagina o Nginx lendo: error_page 404 /www/erro404.html
        return build_error_response(404, "Not Found", "./www/erro404.html"); 
    }

    if (S_ISDIR(st.st_mode)) {
        return build_error_response(403, "Forbidden"); // Sem autoindex deste vez
    }

    std::ifstream file(full_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return build_error_response(403, "Forbidden"); // Bloqueado pelo OS
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << get_mime_type(full_path) << "\r\n"
             << "Content-Length: " << content.size() << "\r\n"
             << "Connection: close\r\n\r\n";
    
    std::string final_res = response.str();
    final_res.append(content.begin(), content.end());
    return final_res;
}

// ── Boilerplate Servidor ────────────────────────────────────────────────────
int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);
    
    std::cout << "[SETUP] Copiando pasta www para /tmp para testes fluídos..." << std::endl;
    system("cp -r ./www /tmp 2>/dev/null");
    if (chdir("/tmp") != 0) { std::cerr << "Falha chdir /tmp\n"; return 1; }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr; bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = htons(8080);
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 128);
    fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK);

    std::vector<struct pollfd> fds;
    std::vector<Client> clients;
    struct pollfd spfd; spfd.fd = server_fd; spfd.events = POLLIN; spfd.revents = 0;
    fds.push_back(spfd); Client sc; sc.fd = server_fd; clients.push_back(sc);

    std::cout << "\n► Servidor iniciou em HTTP://localhost:8080" << std::endl;
    std::cout << "► Teste o 404 Customizado: http://localhost:8080/caminho-falso" << std::endl;

    while (g_running) {
        if (poll(&fds[0], fds.size(), -1) < 0) break;
        for (int i = (int)fds.size() - 1; i >= 0; --i) {
            if (fds[i].revents == 0) continue;
            if (fds[i].fd == server_fd) {
                while (true) {
                    int cfd = accept(server_fd, NULL, NULL);
                    if (cfd < 0) break;
                    fcntl(cfd, F_SETFL, fcntl(cfd, F_GETFL, 0) | O_NONBLOCK);
                    struct pollfd cpfd; cpfd.fd = cfd; cpfd.events = POLLIN; cpfd.revents = 0;
                    fds.push_back(cpfd); Client c; c.fd = cfd; clients.push_back(c);
                }
                continue;
            }
            Client& cl = clients[i];
            if (fds[i].revents & (POLLHUP | POLLERR)) {
                close(cl.fd); fds.erase(fds.begin() + i); clients.erase(clients.begin() + i); continue;
            }
            if (fds[i].revents & POLLIN) {
                char buf[512]; bzero(buf, sizeof(buf));
                int bytes = recv(cl.fd, buf, sizeof(buf) - 1, 0);
                if (bytes <= 0) { close(cl.fd); fds.erase(fds.begin() + i); clients.erase(clients.begin() + i); continue; }
                cl.recv_buffer.append(buf, bytes);
                HttpRequest req;
                if (parse_request(cl.recv_buffer, req)) {
                    cl.recv_buffer.clear();
                    cl.send_buffer = build_response(req);
                    fds[i].events |= POLLOUT;
                }
            }
            if (fds[i].revents & POLLOUT) {
                if (!cl.send_buffer.empty()) {
                    int sent = send(cl.fd, cl.send_buffer.c_str(), cl.send_buffer.size(), 0);
                    if (sent > 0) cl.send_buffer.erase(0, sent);
                }
                if (cl.send_buffer.empty()) {
                    fds[i].events &= ~POLLOUT;
                    close(cl.fd); fds.erase(fds.begin() + i); clients.erase(clients.begin() + i);
                }
            }
        }
    }
    for (size_t i = 0; i < fds.size(); ++i) close(fds[i].fd);
    return 0;
}
