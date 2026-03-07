// http-chunked.cpp — Servidor focado em decodificar Transfer-Encoding: chunked
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o http_chunked 06-http-chunked.cpp
//
// TESTAR NO TERMINAL:
//   A) Envio normal (sem chunked):
//      curl -v -X POST -d "Hello" http://localhost:8080/
//   B) Envio Chunked simulado pelo curl:
//      curl -v -X POST -H "Transfer-Encoding: chunked" -d "Hello" -d "World!" http://localhost:8080/
//
// CONCEITO DEMONSTRADO:
//   - Identificação do header Transfer-Encoding
//   - Parsing da máquina de estados (ler tamanho em HEX, ler payload, pular \r\n, bater 0).
//   - Reconstrução do Body limpo para o C++ processar ou jogar no disco.

#include <iostream>
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

static volatile bool g_running = true;
static void handle_signal(int) { g_running = false; }

struct HttpRequest {
    std::string method, path, version;
    std::string raw_body;   // Body criptografado / misturado da rede
    std::string clean_body; // Body pronto para uso após "unchunk" (desencapsulamento)
    std::map<std::string, std::string> headers;
    bool is_chunked;
    bool is_complete;
    int status_code;
    
    HttpRequest() : is_chunked(false), is_complete(false), status_code(200) {}
};

struct Client {
    int fd;
    std::string recv_buffer, send_buffer;
    HttpRequest req;
};

// ── Utils ────────────────────────────────────────────────────────────────────
static std::string to_lower(const std::string& s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)tolower((unsigned char)r[i]);
    return r;
}

static void trim_spaces_and_r(std::string& str) {
    size_t first = str.find_first_not_of(" \t\r");
    if (first == std::string::npos) { str.clear(); return; }
    size_t last = str.find_last_not_of(" \t\r");
    str = str.substr(first, last - first + 1);
}

// ── O Coração: Decodificador de Chunked (State Machine Simples) ──────────────
// Lê os marcadores hexadecimais (ex: "5\r\nHello\r\n0\r\n\r\n") e extrai apenas a payload limpa.
// Retorna false se o buffer não tiver a string do bloco completo ainda (polling resume depoix).
static bool decode_chunked_body(std::string& raw_body, std::string& clean_body) {
    size_t offset = 0;
    clean_body.clear();

    while (offset < raw_body.size()) {
        // Passo 1: Encontrar o \r\n que finaliza da string em hexadecimal
        size_t line_end = raw_body.find("\r\n", offset);
        if (line_end == std::string::npos) return false; // Faltam dados no Socket!
        
        std::string hex_str = raw_body.substr(offset, line_end - offset);
        
        // Passo 2: Conversão de HEX para decimal (C++98)
        std::stringstream ss;
        ss << std::hex << hex_str;
        size_t chunk_size = 0;
        ss >> chunk_size;

        // Se a leitura falhar, ou algo fugir à formatação: quebra corrompida!
        if (ss.fail()) return false;

        // Passo 3: Limite Final! Se chunk for 0, marca sucesso e para a varredura
        if (chunk_size == 0) return true;

        // Passo 4: Validar se os dados (payload) + "\r\n" já engataram no recv() original do SO
        // Tamanho da quebra: line_end (início) + 2 ('\r\n') + tamanho em bytes + 2 ('\r\n' final do chunk)
        if (line_end + 2 + chunk_size + 2 > raw_body.size()) return false;

        // Passo 5: Cortar a carga limpa e adicionar na classe
        clean_body.append(raw_body.substr(line_end + 2, chunk_size));

        // Passe virtual do Offset (Pular a leitura processada pro próximo Hex)
        offset = line_end + 2 + chunk_size + 2;
    }
    
    return false; // Loop estourou, mas o zero `0\r\n\r\n` não veio ainda.
}

// ── Parsing HTTP ─────────────────────────────────────────────────────────────
static bool parse_request(Client& cl) {
    std::string& buffer = cl.recv_buffer;
    HttpRequest& req = cl.req;

    // Se Headers não terminaram de chegar, pare
    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) return false;

    // Apenas dá Parse nos headers na primeira passagem
    if (req.method.empty()) {
        size_t line_end = buffer.find("\r\n");
        std::string request_line = buffer.substr(0, line_end);
        
        size_t p1 = request_line.find(' '), p2 = request_line.find(' ', p1 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos) { req.status_code = 400; req.is_complete = true; return true; }
        
        req.method = request_line.substr(0, p1);
        req.path = request_line.substr(p1 + 1, p2 - p1 - 1);
        
        // Extrator de Headers
        size_t start = line_end + 2;
        while (start < header_end) {
            size_t end = buffer.find("\r\n", start);
            std::string header_line = buffer.substr(start, end - start);
            size_t colon = header_line.find(':');
            
            if (colon != std::string::npos) {
                std::string key = to_lower(header_line.substr(0, colon));
                std::string val = header_line.substr(colon + 1);
                trim_spaces_and_r(val);
                req.headers[key] = val;
            }
            start = end + 2;
        }

        // Detecta modalidade: Body Chunked vs Body Normal (Content-Length)
        if (req.headers.count("transfer-encoding") && req.headers["transfer-encoding"] == "chunked") {
            req.is_chunked = true;
        }
    }

    // Processar Body (Tudo que estiver além do `\r\n\r\n`)
    size_t body_start_index = header_end + 4;
    req.raw_body = buffer.substr(body_start_index);

    // Método não prevê body (GET/DELETE)? Passou.
    if (req.method == "GET" || req.method == "DELETE") {
        req.is_complete = true;
        return true;
    }

    // Modalidade 1: É Chunked! Roda o Parser e checa o "0" marcador
    if (req.is_chunked) {
        if (decode_chunked_body(req.raw_body, req.clean_body)) {
            std::cout << "[PARSING] Chunk processado com sucesso! Limpamos: " << req.clean_body.size() << " bytes." << std::endl;
            req.is_complete = true;
            return true;
        }
        return false; // Ainda aguardando mais pedaços
    }

    // Modalidade 2: Content-Length comum
    if (req.headers.count("content-length")) {
        std::stringstream ss(req.headers["content-length"]);
        size_t cl_len; ss >> cl_len;
        if (req.raw_body.size() >= cl_len) {
            req.clean_body = req.raw_body.substr(0, cl_len);
            req.is_complete = true;
            return true;
        }
        return false; // Esperar recv() completar os dados lógicos da carga inteira
    }

    // Não forneceu Length e nem é Chunked, mas é um verbo de Ação. Invalida
    req.status_code = 400;
    req.is_complete = true;
    return true;
}

// ── Resposta do Teste ────────────────────────────────────────────────────────
static std::string build_response(const HttpRequest& req) {
    if (req.status_code != 200) {
        std::ostringstream res;
        res << "HTTP/1.1 " << req.status_code << " Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        return res.str();
    }
    
    std::ostringstream res;
    res << "HTTP/1.1 200 OK\r\n";
    res << "Content-Type: text/plain\r\n";
    res << "Connection: close\r\n";
    
    std::string report = "=== Relatorio do Servidor ===\n";
    report += "Metodo: " + req.method + "\n";
    report += "Path: " + req.path + "\n";
    report += "Tamanho do Body: " + (req.is_chunked ? std::string("(Extraido do Chunk) ") : "(Comum) ");
    
    std::ostringstream sz; sz << req.clean_body.size() << " bytes\n";
    report += sz.str();
    
    report += "Body extraído:\n'" + req.clean_body + "'\n";
    
    res << "Content-Length: " << report.size() << "\r\n\r\n";
    res << report;
    
    return res.str();
}

// ── Event Loop Boilerplate ───────────────────────────────────────────────────
int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

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
    std::cout << "► Teste envio em Chunks com Curl:" << std::endl;
    std::cout << "   curl -v -X POST -H \"Transfer-Encoding: chunked\" -d \"Primeiro\" -d \"Segundo\" http://localhost:8080/\n" << std::endl;

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
                
                if (parse_request(cl)) { // Se a máquina de estados validou a request completa
                    cl.recv_buffer.clear();
                    cl.send_buffer = build_response(cl.req);
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
