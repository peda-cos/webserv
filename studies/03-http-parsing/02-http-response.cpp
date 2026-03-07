// http-response.cpp — Servidor estático básico (MIME Types, 404 e Leitura do Disco)
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o http_response 03-http-response.cpp
//
// TESTAR NO NAVEGADOR:
//   Abra o Google Chrome e digite: http://localhost:8080/index.html
//
// CONCEITO DEMONSTRADO:
//   - Servir arquivos reais armazenados no disco.
//   - Identificar a extensão e retornar o Content-Type correto (MIME Type).
//   - Se o arquivo não existir, gerar um bloco `404 Not Found`.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
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

// ── Estruturas Básicas ───────────────────────────────────────────────────────
struct HttpRequest {
    std::string method, path, version, body;
    std::map<std::string, std::string> headers;
};

struct Client {
    int fd;
    std::string recv_buffer, send_buffer;
};

// ── Utils de String ──────────────────────────────────────────────────────────
static std::string to_lower(const std::string& s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)tolower((unsigned char)r[i]);
    return r;
}

static std::string get_extension(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    return to_lower(path.substr(pos));
}

// ── Mapeamento MIME Types ────────────────────────────────────────────────────
// Isso ensina o navegador como renderizar os dados que vamos enviar.
static std::string get_mime_type(const std::string& path) {
    std::string ext = get_extension(path);
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")                   return "text/css";
    if (ext == ".js")                    return "application/javascript";
    if (ext == ".png")                   return "image/png";
    if (ext == ".jpg"  || ext == ".jpeg") return "image/jpeg";
    if (ext == ".ico")                   return "image/x-icon";
    return "application/octet-stream"; // Caso padrão: força aba de download
}

// ── O Coração da Construção da Resposta ──────────────────────────────────────
static std::string build_response(const HttpRequest& req) {
    // 1. Caminho Raiz do nosso servidor estático fictício
    std::string base_dir = "./www";
    
    // Tratamento básico pro caso de só mandar "/"
    std::string file_path = req.path;
    if (file_path == "/") file_path = "/index.html"; 
    
    // Caminho final procurado
    std::string full_path = base_dir + file_path;
    std::cout << "[ROTEAMENTO] Procurando arquivo físico: " << full_path << std::endl;

    // 2. Abordar erro de arquivo inexistente
    std::ifstream file(full_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::string body = "<h1>Erro 404</h1><p>Arquivo ou diretorio nao encontrado.</p>";
        std::ostringstream oss;
        oss << "HTTP/1.1 404 Not Found\r\n";
        oss << "Content-Type: text/html\r\n";
        oss << "Content-Length: " << body.size() << "\r\n";
        oss << "Connection: close\r\n\r\n";
        oss << body;
        return oss.str();
    }

    // 3. Se o arquivo existe: Lê da RAM, extrai Content-Type, e monta resposta
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string mime_type = get_mime_type(full_path);
    
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Type: " << mime_type << "\r\n";
    oss << "Content-Length: " << content.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    
    // Concatena Header + File Body de forma segura
    std::string response = oss.str();
    response.append(content.begin(), content.end());
    
    file.close();
    return response;
}

// ── Parser Mínimo de Requisição (Baseado no 01-http-parser.cpp) ──────────────
static bool parse_request(const std::string& buffer, HttpRequest& req) {
    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) return false;

    size_t line_end = buffer.find("\r\n");
    std::string request_line = buffer.substr(0, line_end);
    
    size_t pos1 = request_line.find(' ');
    size_t pos2 = request_line.find(' ', pos1 + 1);
    
    req.method = request_line.substr(0, pos1);
    req.path = request_line.substr(pos1 + 1, pos2 - pos1 - 1);
    req.version = request_line.substr(pos2 + 1);
    
    return true; // Só parseamos o topo pq Connection: close já ta hardcoded
}

// ── Boilerplate do Servidor poll() ───────────────────────────────────────────
static int criar_servidor() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr; bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = htons(8080);
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(fd, 128);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    return fd;
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    // Facilita os testes: copia a pasta de estudos dinamicamente para onde o server rodar
    std::cout << "[SETUP] Copiando pasta www para /tmp para facilitar seus testes..." << std::endl;
    std::string src_path = __FILE__;
    size_t pos = src_path.find_last_of('/');
    std::string dir = (pos == std::string::npos) ? "." : src_path.substr(0, pos);
    std::string cmd = "cp -r " + dir + "/www /tmp 2>/dev/null";
    system(cmd.c_str());
    
    // Altera o diretório de trabalho do processo silenciosamente para rodar lá
    if (chdir("/tmp") != 0) {
        std::cerr << "Falha ao mudar para o /tmp" << std::endl;
        return 1;
    }

    int server_fd = criar_servidor();
    std::vector<struct pollfd> fds;
    std::vector<Client> clients;

    struct pollfd spfd; spfd.fd = server_fd; spfd.events = POLLIN; spfd.revents = 0;
    fds.push_back(spfd);
    Client sc; sc.fd = server_fd; clients.push_back(sc);

    std::cout << "\n► Servidor iniciou em HTTP://localhost:8080" << std::endl;
    std::cout << "► Abra o Chrome no PC Host e acesse:" << std::endl;
    std::cout << "   http://localhost:8080/index.html" << std::endl;
    std::cout << "   http://localhost:8080/nao-existe.html (Para ver o 404)" << std::endl;

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
                    fds.push_back(cpfd);
                    Client c; c.fd = cfd; clients.push_back(c);
                }
                continue;
            }

            Client& cl = clients[i];

            if (fds[i].revents & (POLLHUP | POLLERR)) {
                close(cl.fd); fds.erase(fds.begin() + i); clients.erase(clients.begin() + i);
                continue;
            }

            if (fds[i].revents & POLLIN) {
                char buf[512]; bzero(buf, sizeof(buf));
                int bytes = recv(cl.fd, buf, sizeof(buf) - 1, 0);
                if (bytes <= 0) {
                    close(cl.fd); fds.erase(fds.begin() + i); clients.erase(clients.begin() + i);
                    continue;
                }
                cl.recv_buffer.append(buf, bytes);
                HttpRequest req;
                if (parse_request(cl.recv_buffer, req)) {
                    cl.recv_buffer.clear();
                    cl.send_buffer = build_response(req); // Essa função mapeia e envia o disk I/O!
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
