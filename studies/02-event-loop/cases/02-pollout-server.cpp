// 03-pollout-server.cpp — Demonstração de POLLOUT com envio de arquivo grande
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o pollout 03-pollout-server.cpp
//
// TESTAR:
//   Terminal 1: /tmp/pollout
//   Terminal 2: nc localhost 8080 > /tmp/recebido.txt
//              (Ctrl+D após receber tudo, depois: wc -c /tmp/recebido.txt)
//
// CONCEITO DEMONSTRADO:
//   O servidor lê um arquivo grande (passado como argv[1]) inteiro para memória,
//   depois tenta enviar de uma só vez. send() retorna menos do que pedido.
//   O restante vai para o send_buffer do cliente e é enviado via POLLOUT.

#include <iostream>
#include <string>
#include <vector>
#include <fstream>       // std::ifstream — leitura de arquivo
#include <cstdio>        // perror
#include <cstring>       // bzero
#include <cstdlib>       // EXIT_FAILURE
#include <csignal>       // signal, SIGPIPE, SIG_IGN
#include <sys/socket.h>  // socket, bind, listen, accept, send, recv, setsockopt
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY, htons
#include <poll.h>        // poll(), struct pollfd, POLLIN, POLLOUT, POLLHUP
#include <fcntl.h>       // fcntl(), F_SETFL, O_NONBLOCK
#include <unistd.h>      // close

static void set_nonblocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

static int create_server(int porta)
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
    set_nonblocking(fd);
    return fd;
}

// ── Struct por cliente — a inovação deste arquivo ────────────────────────────
// Cada cliente tem seu próprio send_buffer (dados aguardando envio).
// Quando send_buffer não estiver vazio, ativamos POLLOUT no pollfd desse cliente.
struct Client
{
    int         fd;
    std::string send_buffer; // bytes ainda não enviados para este cliente
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Uso: ./pollout <caminho-do-arquivo-grande>" << std::endl;
        return EXIT_FAILURE;
    }

    // ── Lê o arquivo inteiro para memória ────────────────────────────────
    // O servidor vai enviar esse conteúdo completo para cada cliente que conectar.
    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    file.close();

    std::cout << "Arquivo carregado: " << content.size() << " bytes" << std::endl;

    signal(SIGPIPE, SIG_IGN);

    int server_fd = create_server(8080);
    if (server_fd < 0) return EXIT_FAILURE;

    // ── Arrays paralelos ─────────────────────────────────────────────────
    // fds[i] é o pollfd monitorado pelo poll()
    // clients[i] é o Client com o send_buffer do mesmo fd
    std::vector<struct pollfd> fds;
    std::vector<Client>        clients;

    struct pollfd spfd;
    spfd.fd      = server_fd;
    spfd.events  = POLLIN;
    spfd.revents = 0;
    fds.push_back(spfd);
    Client sc; sc.fd = server_fd;
    clients.push_back(sc);

    std::cout << "Servidor POLLOUT rodando na porta 8080." << std::endl;
    std::cout << "Conecte com: nc localhost 8080 > /tmp/recebido.txt" << std::endl;

    while (true)
    {
        int ready = poll(&fds[0], fds.size(), -1);
        if (ready < 0) { perror("poll"); break; }

        for (int i = (int)fds.size() - 1; i >= 0; --i)
        {
            if (fds[i].revents == 0) continue;

            // ── Nova conexão no server_fd ─────────────────────────────────
            if (fds[i].fd == server_fd)
            {
                while (true)
                {
                    int cfd = accept(server_fd, NULL, NULL);
                    if (cfd < 0) break;

                    set_nonblocking(cfd);

                    struct pollfd cpfd;
                    cpfd.fd      = cfd;
                    cpfd.events  = POLLIN;  // começa só escutando
                    cpfd.revents = 0;
                    fds.push_back(cpfd);

                    Client c;
                    c.fd          = cfd;
                    c.send_buffer = content; // carrega o arquivo inteiro no buffer de saída

                    clients.push_back(c);

                    // Ativa imediatamente POLLOUT — temos dados para enviar
                    fds.back().events |= POLLOUT; // bitwise |= serve para adicionar o POLLOUT ao POLLIN

                    std::cout << "[+] fd=" << cfd << " conectado — "
                              << content.size() << " bytes para enviar." << std::endl;
                }
                continue;
            }

            Client& client = clients[i];

            // ── POLLERR / POLLHUP — cliente desconectou ───────────────────
            if (fds[i].revents & (POLLHUP | POLLERR)) {
                std::cout << "[-] fd=" << client.fd << " desconectou." << std::endl;
                close(client.fd);
                fds.erase(fds.begin() + i);
                clients.erase(clients.begin() + i);
                continue;
            }

            // ── POLLOUT — socket do cliente pronto para receber dados ─────
            // Isso é ativado quando o buffer TCP de saída tem espaço livre.
            // Enviamos o que couber do send_buffer e guardamos o resto.
            if (fds[i].revents & POLLOUT)
            {
                if (!client.send_buffer.empty())
                {
                    int enviado = send(client.fd,
                                       client.send_buffer.c_str(),
                                       client.send_buffer.size(), 0);

                    if (enviado < 0) {
                        // Erro real: remove o cliente
                        std::cout << "[-] fd=" << client.fd << " erro no send(). Removendo." << std::endl;
                        close(client.fd);
                        fds.erase(fds.begin() + i);
                        clients.erase(clients.begin() + i);
                        continue;
                    }

                    // ── O CORAÇÃO DO POLLOUT ──────────────────────────────
                    // Descarta apenas os bytes que realmente foram enviados.
                    // O restante permanece no send_buffer para a próxima vez.
                    client.send_buffer.erase(0, enviado);

                    std::cout << "[fd=" << client.fd << "] enviou " << enviado
                              << " bytes | restam " << client.send_buffer.size()
                              << " bytes no buffer." << std::endl;

                    if (client.send_buffer.empty()) {
                        // Buffer esvaziou — desativa POLLOUT para não queimar CPU
                        fds[i].events &= ~POLLOUT;
                        std::cout << "[fd=" << client.fd
                                  << "] Transferência completa! POLLOUT desativado." << std::endl;
                    }
                }
            }

            // ── POLLIN — cliente enviou algo (ignoramos neste demo) ───────
            if (fds[i].revents & POLLIN)
            {
                char buf[64];
                recv(client.fd, buf, sizeof(buf), 0); // descarta o que o cliente mandou
            }
        }
    }

    for (size_t i = 0; i < fds.size(); ++i)
        close(fds[i].fd);
    return 0;
}
