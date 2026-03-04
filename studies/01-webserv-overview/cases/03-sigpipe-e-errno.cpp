// 03-sigpipe-e-errno.cpp — Demonstração de SIGPIPE e tratamento correto de errno
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o sigpipe 03-sigpipe-e-errno.cpp
//
// COMO USAR:
//   ./sigpipe sem    → servidor SEM proteção (morre com SIGPIPE)
//   ./sigpipe com    → servidor COM proteção (sobrevive ao SIGPIPE)
//
// TESTE:
//   Terminal 1: ./sigpipe com
//   Terminal 2: nc localhost 8080  →  Ctrl+C imediatamente após conectar
//   Observe: o servidor COM proteção continua vivo

#include <iostream>
#include <cstdio>        // perror
#include <cstring>       // bzero, strcmp
#include <cstdlib>       // EXIT_FAILURE
#include <csignal>       // signal, SIGPIPE, SIG_IGN
#include <cerrno>        // errno, EAGAIN, EWOULDBLOCK, EINTR
#include <sys/socket.h>  // socket, bind, listen, accept, send, recv, setsockopt
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY, htons
#include <unistd.h>      // close, sleep

// ── Criação e setup do servidor ──────────────────────────────────────────────
// Encapsula socket + setsockopt + bind + listen num helper para clareza.
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
    addr.sin_port        = htons(porta);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }
    if (listen(fd, 5) < 0) {
        perror("listen"); close(fd); return -1;
    }
    return fd;
}

int main(int argc, char **argv)
{
    // ── Verifica argumento ───────────────────────────────────────────────
    if (argc < 2 || (strcmp(argv[1], "sem") != 0 && strcmp(argv[1], "com") != 0)) {
        std::cerr << "Uso: ./sigpipe [sem|com]" << std::endl;
        std::cerr << "  sem = servidor SEM proteção de SIGPIPE" << std::endl;
        std::cerr << "  com = servidor COM proteção de SIGPIPE" << std::endl;
        return EXIT_FAILURE;
    }

    bool protegido = (strcmp(argv[1], "com") == 0);

    // ── SIGPIPE: ignora ou deixa matar ───────────────────────────────────
    // signal(SIGPIPE, SIG_IGN) faz send() retornar -1 em vez de matar o processo.
    // Sem isso, qualquer cliente que fechar a conexão no momento errado derruba o servidor.
    if (protegido) {
        signal(SIGPIPE, SIG_IGN);
        std::cout << "[MODO] COM proteção: SIGPIPE ignorado via signal(SIGPIPE, SIG_IGN)" << std::endl;
    } else {
        std::cout << "[MODO] SEM proteção: SIGPIPE vai matar o servidor!" << std::endl;
    }

    int server_fd = create_server(8080);
    if (server_fd < 0) return EXIT_FAILURE;

    std::cout << "Servidor na porta 8080. Conecte e feche abruptamente (Ctrl+C no nc)." << std::endl;

    // ── LOOP PRINCIPAL ───────────────────────────────────────────────────
    // Mantém o servidor rodando e demonstra o comportamento do errno.
    while (true)
    {
        std::cout << "\n[AGUARDANDO] conexão..." << std::endl;

        socklen_t len = sizeof(struct sockaddr_in);
        struct sockaddr_in client_addr;
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (client_fd < 0) { perror("accept"); continue; }

        std::cout << "[CONECTADO] fd=" << client_fd
                  << " — feche a conexão do cliente nos próximos 3 segundos (Ctrl+C no nc)" << std::endl;

        // ── DEMONSTRAÇÃO DE SIGPIPE ──────────────────────────────────────
        // Envia 10 mensagens em loop com 1 segundo de pausa entre cada uma.
        // Após o Ctrl+C no nc, o kernel recebe RST e a próxima tentativa
        // de send() gera SIGPIPE.
        // SEM signal(SIGPIPE, SIG_IGN): processo morre na iteração com RST.
        // COM signal(SIGPIPE, SIG_IGN): send() retorna -1, loop continua.
        const char *msg = "ping do servidor\n";
        for (int i = 1; i <= 10; ++i)
        {
            std::cout << "  [send #" << i << "] tentando..." << std::flush;
            int resultado = send(client_fd, msg, strlen(msg), 0);
            if (resultado < 0) {
                std::cout << " FALHOU! send() retornou -1" << std::endl;
                std::cout << "[SOBREVIVEU] SIGPIPE ignorado — servidor continua!" << std::endl;
                break;
            }
            std::cout << " OK (" << resultado << " bytes)" << std::endl;
            sleep(1); // pausa para dar tempo de fechar o nc entre tentativas
        }

        // ── DEMONSTRAÇÃO DO PADRÃO CORRETO de recv() ────────────────────
        // Classifica o retorno de recv() sem depender de errno para a decisão.
        char buf[256];
        bzero(buf, sizeof(buf));
        int bytes = recv(client_fd, buf, sizeof(buf) - 1, 0);

        if (bytes == 0) {
            // 0 = cliente fechou a conexão (EOF) — normal, fechar o fd
            std::cout << "[RECV] retornou 0 → cliente desconectou (EOF)" << std::endl;
        } else if (bytes < 0) {
            // -1 = erro — mas qual? Só examina errno para LOG, não para lógica
            std::cout << "[RECV] retornou -1 → erro de rede. fd será fechado." << std::endl;
        } else {
            std::cout << "[RECV] " << bytes << " bytes: " << buf << std::endl;
        }

        close(client_fd);
        std::cout << "[FECHADO] fd=" << client_fd << std::endl;
    }

    close(server_fd);
    return 0;
}
