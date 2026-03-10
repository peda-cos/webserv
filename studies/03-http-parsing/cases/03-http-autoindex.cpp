// http-autoindex.cpp — Servidor que lida com Arquivos e Diretórios (Autoindex)
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o http_autoindex 04-http-autoindex.cpp
//
// TESTAR NO NAVEGADOR:
//   http://localhost:8080/          -> Deve listar os arquivos da pasta www/ (Autoindex)
//   http://localhost:8080/imagens/  -> Deve listar a pasta de imagens
//   http://localhost:8080/index.html -> Deve abrir o arquivo normalmente
//
// CONCEITO DEMONSTRADO:
//   - Uso de stat() para descobrir se o path pedido é ARQUIVO ou DIRETÓRIO.
//   - Uso de opendir() e readdir() para varrer pastas.
//   - Geração dinâmica de HTML com links para o cliente (Autoindex).

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
#include <dirent.h>

static volatile bool g_running = true;
static void handle_signal(int) { g_running = false; }

struct HttpRequest {
    std::string method, path, version, body;
    std::map<std::string, std::string> headers;
};

struct Client {
    int fd;
    std::string recv_buffer, send_buffer;
};

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
    if (ext == ".jpg")  return "image/jpeg";
    return "application/octet-stream";
}

// ── Gerador de HTML para Diretórios (Autoindex) ──────────────────────────────
static std::string generate_autoindex(const std::string& physical_path, const std::string& logical_path) {
    DIR* dir = opendir(physical_path.c_str());
    if (!dir) return ""; // Retorna vazio se falhar ao abrir o diretório

    std::ostringstream html;
    html << "<html><head><title>Index of " << logical_path << "</title></head><body>\n";
    html << "<h1>Index of " << logical_path << "</h1><hr><pre>\n";

    // Adiciona link para voltar (diretório pai)
    if (logical_path != "/") {
        html << "<a href=\"../\">../</a>\n";
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue; // Pula referências locais

        // Identifica se é pasta para colocar a barra "/" no final do nome
        std::string full_path = physical_path + "/" + name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            name += "/";
        }

        // href precisa do caminho absoluto na web (lógico) para evitar paths quebrados
        std::string href = logical_path;
        if (href[href.size() - 1] != '/') href += "/";
        href += name;

        html << "<a href=\"" << href << "\">" << name << "</a>\n";
    }
    
    closedir(dir); // CRÍTICO: Nunca vazar FDs!
    html << "</pre><hr></body></html>";
    return html.str();
}

// ── Montagem da Resposta (Tratando Arquivos vs Diretórios) ───────────────────
static std::string build_response(const HttpRequest& req) {
    std::string base_dir = "./www";
    std::string full_path = base_dir + req.path;
    std::cout << "[ROTEAMENTO] Path: " << req.path << " -> Físico: " << full_path << std::endl;

    struct stat st;
    std::ostringstream response;
    
    // 1. O caminho existe fisicamente no disco?
    if (stat(full_path.c_str(), &st) < 0) {
        std::string body = "<h1>404 Not Found</h1>";
        response << "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n"
                 << "Content-Length: " << body.size() << "\r\nConnection: close\r\n\r\n" << body;
        return response.str();
    }

    // 2. É UM DIRETÓRIO!
    if (S_ISDIR(st.st_mode)) {
        // Redirecionamento 301 se faltar a "Trailing Slash" ("/" final)
        if (req.path[req.path.size() - 1] != '/') {
            std::string new_url = req.path + "/";
            response << "HTTP/1.1 301 Moved Permanently\r\nLocation: " << new_url << "\r\n"
                     << "Content-Length: 0\r\nConnection: close\r\n\r\n";
            return response.str();
        }

        // Aqui eu geraria o index.html padrão caso existisse.
        // Como o foco é Autoindex, vou forçar a geração da lista HTML nativa.
        std::string body = generate_autoindex(full_path, req.path);
        
        if (body.empty()) { // Falhou ao ler diretório (ex: sem permissão)
            std::string err = "<h1>403 Forbidden</h1>";
            response << "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n"
                     << "Content-Length: " << err.size() << "\r\nConnection: close\r\n\r\n" << err;
            return response.str();
        }

        response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
                 << "Content-Length: " << body.size() << "\r\nConnection: close\r\n\r\n" << body;
        return response.str();
    }

    // 3. É UM ARQUIVO COMUM!
    std::ifstream file(full_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::string body = "<h1>403 Forbidden</h1><p>Sem permissão de leitura.</p>";
        response << "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n"
                 << "Content-Length: " << body.size() << "\r\nConnection: close\r\n\r\n" << body;
        return response.str();
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    response << "HTTP/1.1 200 OK\r\nContent-Type: " << get_mime_type(full_path) << "\r\n"
             << "Content-Length: " << content.size() << "\r\nConnection: close\r\n\r\n";
    
    std::string final_res = response.str();
    final_res.append(content.begin(), content.end());
    return final_res;
}

// ── Parser Mínimo (igual aos anteriores) ─────────────────────────────────────
static bool parse_request(const std::string& buffer, HttpRequest& req) {
    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) return false;
    size_t line_end = buffer.find("\r\n");
    std::string request_line = buffer.substr(0, line_end);
    size_t pos1 = request_line.find(' '), pos2 = request_line.find(' ', pos1 + 1);
    req.method = request_line.substr(0, pos1);
    req.path = request_line.substr(pos1 + 1, pos2 - pos1 - 1);
    return true;
}

// ── Boilerplate do Event Loop (poll) ─────────────────────────────────────────
int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    // Facilita os testes: copia a pasta de estudos para o tmp dinamicamente
    std::cout << "[SETUP] Copiando pasta www para /tmp para facilitar seus testes..." << std::endl;
    std::string src_path = __FILE__;
    size_t pos = src_path.find_last_of('/');
    std::string dir = (pos == std::string::npos) ? "." : src_path.substr(0, pos);
    std::string cmd = "cp -r " + dir + "/www /tmp 2>/dev/null";
    system(cmd.c_str());
    
    // Altera o diretório de trabalho do processo silenciosamente para o path de cópia
    if (chdir("/tmp") != 0) {
        std::cerr << "Falha ao mudar para o /tmp" << std::endl;
        return 1;
    }

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
    fds.push_back(spfd);
    Client sc; sc.fd = server_fd; clients.push_back(sc);

    std::cout << "\n► Servidor iniciou em HTTP://localhost:8080" << std::endl;
    std::cout << "► Teste os diretorios:\n   http://localhost:8080/\n   http://localhost:8080/imagens/\n";

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
                if (bytes <= 0) {
                    close(cl.fd); fds.erase(fds.begin() + i); clients.erase(clients.begin() + i); continue;
                }
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
