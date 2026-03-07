// http-upload.cpp — Servidor focado em Extração de Boundary e Salvamento em Disco
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o http_upload 07-http-upload.cpp
//
// TESTAR NO TERMINAL:
//   Passo 1: curl -v http://localhost:8080/upload.html
//   Passo 2: curl -v -X POST -F "arquivo=@/etc/passwd" http://localhost:8080/upload
//   
// O arquivo será salvo na pasta root (/tmp/www/uploads/...) com 201 Created!

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
    std::string method, path, version;
    std::string raw_body;
    std::map<std::string, std::string> headers;
    
    // Controles de Upload
    bool is_multipart;
    std::string boundary;
    std::string upload_filename;
    std::string upload_content;
    
    bool is_complete;
    int status_code;
    
    HttpRequest() : is_multipart(false), is_complete(false), status_code(200) {}
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

// Extrai o nome do arquivo do form-data header: Content-Disposition: form-data; name="x"; filename="teste.txt"
static std::string extract_filename(const std::string& disposition) {
    size_t pos = disposition.find("filename=\"");
    if (pos == std::string::npos) return "arquivo_sem_nome.dat";
    pos += 10;
    size_t end = disposition.find("\"", pos);
    if (end == std::string::npos) return "arquivo_sem_nome.dat";
    return disposition.substr(pos, end - pos);
}

// ── O Coração: Fatiador Multipart ─────────────────────────────────────────────
// Objetivo: Achar o Boundary, pular os headers do form-data e extrair unicamente os bytes físicos do arquivo
static bool parse_multipart(HttpRequest& req) {
    std::string full_boundary = "--" + req.boundary;
    std::string end_boundary = full_boundary + "--";

    // Acha onde começa a engrenagem
    size_t start_pos = req.raw_body.find(full_boundary);
    if (start_pos == std::string::npos) return false;

    // Acha ondem terminam os headers do bloco interno (ex: Content-Disposition...)
    size_t header_end = req.raw_body.find("\r\n\r\n", start_pos);
    if (header_end == std::string::npos) return false;

    // Extrai o filename para podermos salvar pro sistema
    std::string block_headers = req.raw_body.substr(start_pos, header_end - start_pos);
    size_t disp_pos = block_headers.find("Content-Disposition:");
    if (disp_pos != std::string::npos) {
        req.upload_filename = extract_filename(block_headers);
    } else {
        req.upload_filename = "arquivo_desconhecido.dat";
    }

    // Acha ondem termina tudo para extrair o payload
    size_t payload_start = header_end + 4;
    size_t payload_end = req.raw_body.find(full_boundary, payload_start);
    
    // Subtraimos 2 do End para remover o \r\n que cola antes do próximo Boundary
    if (payload_end == std::string::npos) return false;

    req.upload_content = req.raw_body.substr(payload_start, (payload_end - 2) - payload_start);
    return true; // Arquivo dissecado com sucesso em RAM (Ideal no projeto final é gravar file.write em tempo real!)
}

// ── Parsing HTTP Inicial ─────────────────────────────────────────────────────
static bool parse_request(Client& cl) {
    std::string& buffer = cl.recv_buffer;
    HttpRequest& req = cl.req;

    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) return false;

    if (req.method.empty()) {
        size_t line_end = buffer.find("\r\n");
        std::string request_line = buffer.substr(0, line_end);
        
        size_t p1 = request_line.find(' '), p2 = request_line.find(' ', p1 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos) { req.status_code = 400; req.is_complete = true; return true; }
        
        req.method = request_line.substr(0, p1);
        req.path = request_line.substr(p1 + 1, p2 - p1 - 1);
        
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

        // Detecta se é um Upload (Multipart Form Data) e raspa o ID do Boundary no processo
        if (req.headers.count("content-type")) {
            std::string ct = req.headers["content-type"];
            if (ct.find("multipart/form-data") != std::string::npos) {
                req.is_multipart = true;
                size_t b_pos = ct.find("boundary=");
                if (b_pos != std::string::npos) {
                    req.boundary = ct.substr(b_pos + 9);
                }
            }
        }
    }

    size_t body_start = header_end + 4;
    req.raw_body = buffer.substr(body_start);

    // Se tem Length declarado, acumula!
    if (req.headers.count("content-length")) {
        std::stringstream ss(req.headers["content-length"]);
        size_t cl_len; ss >> cl_len;
        
        if (req.raw_body.size() >= cl_len) {
            if (req.is_multipart) {
                if (!parse_multipart(req)) {
                    req.status_code = 400; // Parse quebrou a sintaxe do Boundary! Corrompido!
                }
            }
            req.is_complete = true;
            return true;
        }
        return false; // Ainda aguardando mais pedaços do Socket do Browser/cURL
    }

    req.is_complete = true;
    return true;
}

// ── Montagem da Resposta Lógica (Salvamento no Disco) ────────────────────────
static std::string build_response(const HttpRequest& req) {
    if (req.status_code != 200) {
        std::ostringstream res; res << "HTTP/1.1 " << req.status_code << " Error\r\nConnection: close\r\n\r\n"; return res.str();
    }
    
    // Serve páginas comuns 
    if (req.method == "GET") {
        std::string fp = "./www" + (req.path == "/" ? "/index.html" : req.path);
        std::ifstream file(fp.c_str(), std::ios::binary);
        if (!file.is_open()) return "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::ostringstream res;
        res << "HTTP/1.1 200 OK\r\nContent-Length: " << content.size() << "\r\nConnection: close\r\n\r\n" << content;
        return res.str();
    }

    // Ponto Focal Acadêmico: A Gravação Real no Disco de Host (201 Created)
    if (req.method == "POST" && req.is_multipart && req.path == "/upload") {
        mkdir("./www/uploads", 0777); // Cria rota se não houver
        std::string save_path = "./www/uploads/" + req.upload_filename;
        
        std::ofstream outfile(save_path.c_str(), std::ios::binary);
        if (outfile.is_open()) {
            outfile.write(req.upload_content.c_str(), req.upload_content.size());
            outfile.close();
            
            std::ostringstream res;
            res << "HTTP/1.1 201 Created\r\n"; // 201 porque um recurso NOVO Cansativo foi materializado no sistema host!
            res << "Content-Type: text/plain\r\n";
            res << "Connection: close\r\n\r\n";
            res << "Sucesso! Arquivo [" << req.upload_filename << "] salvo com " << req.upload_content.size() << " bytes em " << save_path << "\n";
            return res.str();
        } else {
            return "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\nErro ao escrever o arquivo no disco!\n";
        }
    }

    return "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n";
}

// ── Boilerplate Servidor ────────────────────────────────────────────────────
int main() {
    signal(SIGPIPE, SIG_IGN); signal(SIGINT, handle_signal); signal(SIGTERM, handle_signal);
    
    // Facilita os testes: copia a pasta de estudos dinamicamente para onde o server rodar
    std::cout << "[SETUP] Copiando pasta www para /tmp..." << std::endl;
    std::string src_path = __FILE__;
    size_t pos = src_path.find_last_of('/');
    std::string dir = (pos == std::string::npos) ? "." : src_path.substr(0, pos);
    std::string cmd = "cp -r " + dir + "/www /tmp 2>/dev/null";
    system(cmd.c_str());
    if (chdir("/tmp") != 0) return 1;

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

    std::cout << "\n► Servidor HTTP Upload ativado em http://localhost:8080" << std::endl;
    std::cout << "► Passo 1: Acesse http://localhost:8080/upload.html no Chrome" << std::endl;
    std::cout << "► Passo 2: Faça upload de uma foto ou txt e olhe o disco (Terminal: ls /tmp/www/uploads)" << std::endl;

    while (g_running) {
        if (poll(&fds[0], fds.size(), -1) < 0) break;
        for (int i = (int)fds.size() - 1; i >= 0; --i) {
            if (fds[i].revents == 0) continue;
            
            if (fds[i].fd == server_fd) {
                int cfd = accept(server_fd, NULL, NULL);
                if (cfd >= 0) {
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
                char buf[2048]; bzero(buf, sizeof(buf)); // Buffer estendido pra arquivos pesados
                int bytes = recv(cl.fd, buf, sizeof(buf) - 1, 0);
                if (bytes <= 0) { close(cl.fd); fds.erase(fds.begin() + i); clients.erase(clients.begin() + i); continue; }
                
                cl.recv_buffer.append(buf, bytes);
                if (parse_request(cl)) {
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
    return 0;
}
