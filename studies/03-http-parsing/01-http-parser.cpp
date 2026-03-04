// 02-http-parser.cpp — Parser básico de requisição HTTP/1.1
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o http_parser 02-http-parser.cpp
//
// TESTAR:
//   Terminal 1: /tmp/http_parser
//   Terminal 2: printf "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n" | nc localhost 8080
//   Terminal 2: printf "POST /upload HTTP/1.1\r\nContent-Length: 11\r\n\r\nhello world" | nc localhost 8080
//
// CONCEITO DEMONSTRADO:
//   Receber bytes brutos do socket, acumular até \r\n\r\n,
//   depois extrair: método, caminho, versão, headers e body.

#include <iostream>
#include <string>
#include <sstream>       // std::ostringstream — converter número para string em C++98
#include <map>
#include <vector>
#include <cstdio>        // perror
#include <cstring>       // bzero
#include <cstdlib>       // EXIT_FAILURE, atoi
#include <cctype>        // tolower
#include <csignal>       // signal, SIGPIPE, SIG_IGN
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

// ── Flag global de encerramento gracioso ────────────────────────────────────
// Setada pelo handler de sinal — faz o while(g_running) parar na próxima iteração.
// volatile: garante que o compilador não otimize/cache a variável em registrador.
static volatile bool g_running = true;

static void handle_signal(int)
{
    g_running = false; // SIGINT (Ctrl+C) ou SIGTERM (kill) → encerra o loop
}

// ── Estrutura que representa uma requisição HTTP parseada ────────────────────
// Isso é o "resultado" do parser — preenchido após encontrar \r\n\r\n no buffer.
struct HttpRequest
{
    std::string                        method;   // "GET", "POST", "DELETE"
    std::string                        path;     // "/index.html"
    std::string                        version;  // "HTTP/1.1"
    std::map<std::string, std::string> headers;  // "host" → "localhost"
    std::string                        body;     // conteúdo após \r\n\r\n (se houver)
    bool                               complete; // true = requisição totalmente recebida

    HttpRequest() : complete(false) {}
};

// ── Converte string para lowercase ───────────────────────────────────────────
// Headers são case-insensitive — normaliza para comparação segura.
static std::string to_lower(const std::string& s)
{
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = (char)tolower((unsigned char)r[i]);
    return r;
}

// ── Extrai o primeiro token antes do delimitador e avança a posição ──────────
static std::string next_token(const std::string& s, size_t& pos, char delim)
{
    size_t start = pos;
    size_t end   = s.find(delim, pos);
    if (end == std::string::npos) end = s.size();
    pos = end + 1;
    return s.substr(start, end - start);
}

// ── Parser principal ──────────────────────────────────────────────────────────
// Tenta parsear o buffer acumulado.
// Retorna false se a requisição ainda está incompleta (não chegou \r\n\r\n).
// Retorna true se parseou com sucesso — altera `req` com os dados extraídos.
static bool parse_request(const std::string& buffer, HttpRequest& req)
{
    // PASSO 1: Verifica se os headers já chegaram completos
    // Busca o boundary \r\n\r\n — sem ele, não temos os headers completos
    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false; // requisição incompleta — aguardar mais dados

    // PASSO 2: Separa a seção de headers (tudo antes de \r\n\r\n)
    std::string header_section = buffer.substr(0, header_end);

    // PASSO 3: Extrai a Request Line — primeira linha dos headers
    size_t      line_end = header_section.find("\r\n");
    std::string request_line = header_section.substr(0, line_end);

    // Parseia método, caminho e versão (separados por espaço)
    size_t pos = 0;
    req.method  = next_token(request_line, pos, ' ');
    req.path    = next_token(request_line, pos, ' ');
    req.version = request_line.substr(pos); // restante é a versão

    // PASSO 4: Parseia cada header — linha por linha após a Request Line
    // Formato de cada linha: "Nome: Valor\r\n"
    size_t cursor = line_end + 2; // pula o \r\n da Request Line
    while (cursor < header_section.size())
    {
        size_t end = header_section.find("\r\n", cursor);
        if (end == std::string::npos) end = header_section.size();

        std::string line = header_section.substr(cursor, end - cursor);
        cursor = end + 2;

        if (line.empty()) break;

        size_t colon = line.find(':');
        if (colon == std::string::npos) continue; // linha malformada — ignora

        std::string name  = to_lower(line.substr(0, colon));
        std::string value = line.substr(colon + 2); // pula ": "
        req.headers[name] = value;
    }

    // PASSO 5: Extrai o body se Content-Length estiver presente
    // Lê exatamente Content-Length bytes após o \r\n\r\n
    if (req.headers.count("content-length") > 0)
    {
        int body_len = atoi(req.headers["content-length"].c_str());
        size_t body_start = header_end + 4; // pula o \r\n\r\n

        if ((int)(buffer.size() - body_start) < body_len)
            return false; // body ainda não chegou completo — aguardar

        req.body = buffer.substr(body_start, body_len);
    }

    req.complete = true;
    return true;
}

// ── Formata e imprime a requisição parseada ───────────────────────────────────
static void print_request(const HttpRequest& req)
{
    std::cout << "\n┌─ Requisição Parseada ─────────────────────────" << std::endl;
    std::cout << "│ Método  : " << req.method  << std::endl;
    std::cout << "│ Caminho : " << req.path    << std::endl;
    std::cout << "│ Versão  : " << req.version << std::endl;
    std::cout << "│ Headers :" << std::endl;
    for (std::map<std::string,std::string>::const_iterator it = req.headers.begin();
         it != req.headers.end(); ++it)
        std::cout << "│   " << it->first << ": " << it->second << std::endl;
    if (!req.body.empty())
        std::cout << "│ Body    : [" << req.body << "]" << std::endl;
    std::cout << "└───────────────────────────────────────────────" << std::endl;
}

// ── Monta resposta HTTP mínima ────────────────────────────────────────────────
static std::string build_response(const HttpRequest& req)
{
    // Verifica método suportado
    if (req.method != "GET" && req.method != "POST" && req.method != "DELETE") {
        return "HTTP/1.1 405 Method Not Allowed\r\n"
               "Content-Length: 0\r\n"
               "Connection: close\r\n\r\n";
    }

    // Resposta de sucesso simples — sem arquivo real ainda
    std::string body = "OK: " + req.method + " " + req.path + "\n";
    std::string response = "HTTP/1.1 200 OK\r\n";
    std::ostringstream oss;
    oss << body.size();
    response += "Content-Length: " + oss.str() + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    return response;
}

// ── Setup do servidor ────────────────────────────────────────────────────────
static int criar_servidor(int porta)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)porta);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }
    if (listen(fd, 10) < 0) {
        perror("listen"); close(fd); return -1;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    return fd;
}

// ── Struct por cliente com buffer de leitura e escrita ───────────────────────
struct Client {
    int         fd;
    std::string recv_buffer; // acumula bytes recebidos até \r\n\r\n
    std::string send_buffer; // resposta aguardando envio (POLLOUT)
};

int main()
{
    signal(SIGPIPE, SIG_IGN);             // evita morte por cliente desconectado
    signal(SIGINT,  handle_signal);         // Ctrl+C → encerramento gracioso
    signal(SIGTERM, handle_signal);         // kill <PID> → encerramento gracioso
    // SIGKILL (kill -9) não pode ser interceptado — o SO mata diretamente

    int server_fd = criar_servidor(8080);
    if (server_fd < 0) return EXIT_FAILURE;

    std::vector<struct pollfd> fds;
    std::vector<Client> clients;

    struct pollfd spfd;
    spfd.fd = server_fd; spfd.events = POLLIN; spfd.revents = 0;
    fds.push_back(spfd);
    Client sc; sc.fd = server_fd;
    clients.push_back(sc);

    std::cout << "Parser HTTP rodando na porta 8080." << std::endl;

    while (g_running) // para quando SIGINT ou SIGTERM chegar
    {
        int ready = poll(&fds[0], fds.size(), -1);
        if (ready < 0) { perror("poll"); break; }

        for (int i = (int)fds.size() - 1; i >= 0; --i)
        {
            if (fds[i].revents == 0) continue;

            // ── Nova conexão ──────────────────────────────────────────────
            if (fds[i].fd == server_fd)
            {
                while (true)
                {
                    int cfd = accept(server_fd, NULL, NULL);
                    if (cfd < 0) break;
                    fcntl(cfd, F_SETFL, fcntl(cfd, F_GETFL, 0) | O_NONBLOCK);

                    struct pollfd cpfd;
                    cpfd.fd = cfd; cpfd.events = POLLIN; cpfd.revents = 0;
                    fds.push_back(cpfd);
                    Client c; c.fd = cfd;
                    clients.push_back(c);
                    std::cout << "[+] fd=" << cfd << " conectado." << std::endl;
                }
                continue;
            }

            Client& cl = clients[i];

            if (fds[i].revents & (POLLHUP | POLLERR)) {
                close(cl.fd);
                fds.erase(fds.begin() + i);
                clients.erase(clients.begin() + i);
                continue;
            }

            // ── POLLIN: recebe dados e tenta parsear ──────────────────────
            if (fds[i].revents & POLLIN)
            {
                char buf[512];
                bzero(buf, sizeof(buf));
                int bytes = recv(cl.fd, buf, sizeof(buf) - 1, 0);

                if (bytes <= 0) {
                    close(cl.fd);
                    fds.erase(fds.begin() + i);
                    clients.erase(clients.begin() + i);
                    continue;
                }

                // Acumula no recv_buffer — pode chegar em fragmentos
                cl.recv_buffer.append(buf, bytes);

                // Tenta parsear — só processa quando \r\n\r\n for encontrado
                HttpRequest req;
                if (parse_request(cl.recv_buffer, req))
                {
                    print_request(req);                // imprime no terminal
                    cl.recv_buffer.clear();            // limpa para próxima requisição
                    cl.send_buffer = build_response(req); // prepara resposta
                    fds[i].events |= POLLOUT;          // ativa envio
                }
            }

            // ── POLLOUT: envia resposta (pode ser parcial!) ───────────────
            if (fds[i].revents & POLLOUT)
            {
                if (!cl.send_buffer.empty())
                {
                    int sent = send(cl.fd, cl.send_buffer.c_str(), cl.send_buffer.size(), 0);
                    if (sent > 0)
                        cl.send_buffer.erase(0, sent); // remove só o que foi enviado
                }
                if (cl.send_buffer.empty()) {
                    fds[i].events &= ~POLLOUT; // desativa quando buffer esvaziar
                    // Fecha conexão (Connection: close no build_response)
                    close(cl.fd);
                    fds.erase(fds.begin() + i);
                    clients.erase(clients.begin() + i);
                }
            }
        }
    }

    // Cleanup: fecha todos os fds antes de sair
    std::cout << "\n[SHUTDOWN] Encerrando servidor..." << std::endl;
    for (size_t i = 0; i < fds.size(); ++i)
        close(fds[i].fd);
    std::cout << "[SHUTDOWN] Todos os fds fechados. Saindo." << std::endl;
    return 0;
}
