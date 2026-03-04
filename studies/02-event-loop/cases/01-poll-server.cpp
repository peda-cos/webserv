// 02-poll-server.cpp — Servidor com poll(), múltiplos clientes + timeout de inatividade
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o poll_server 02-poll-server.cpp
//
// TESTAR:
//   Terminal 1: /tmp/poll_server
//   Terminal 2: nc localhost 8080  — conecte e fique em silêncio por 10s
//   Observe: o servidor fecha a conexão sozinho após o timeout
//
// CONCEITOS NOVOS NESTE ARQUIVO:
//   - poll() com timeout (3º argumento) em vez de -1 (espera infinita)
//   - Vetor paralelo last_activity[] para rastrear inatividade por cliente
//   - CTRL+Z no nc suspende o processo mas não fecha o socket — o timeout resolve isso

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>        // perror
#include <cstring>       // bzero
#include <cstdlib>       // EXIT_FAILURE
#include <ctime>         // time(), time_t — para medir inatividade
#include <csignal>       // signal, SIGPIPE, SIG_IGN
#include <sys/socket.h>  // socket, bind, listen, accept, send, recv, setsockopt
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY, htons
#include <poll.h>        // poll(), struct pollfd, POLLIN, POLLHUP, POLLERR
#include <fcntl.h>       // fcntl(), F_SETFL, O_NONBLOCK
#include <unistd.h>      // close

// Tempo em segundos sem atividade antes de fechar a conexão.
// Protege contra: Ctrl+Z, clientes lentos, ataques slowloris.
static const int TIMEOUT_SECS = 10;

// Intervalo de verificação do poll() em milissegundos.
// A cada POLL_INTERVAL_MS, poll() retorna mesmo sem eventos para checar timeouts.
static const int POLL_INTERVAL_MS = 1000;

// ── Torna um fd não-bloqueante ────────────────────────────────────────────────
// fcntl(F_SETFL, O_NONBLOCK): recv/send/accept retornam imediatamente.
// Obrigatório ao usar poll() para garantir que nunca bloqueia.
static void set_nonblocking(int fd)
{
    /**
     * F_SETFL: Define flags do descritor de arquivo.
     * F_GETFL: Obtém flags do descritor de arquivo.
     * O_NONBLOCK: Flag para modo não bloqueante.
     * 
     * A função fcntl() é usada para manipular descritores de arquivo.
     * Aqui, estamos definindo o descritor de arquivo para modo não bloqueante.
     * Isso significa que as operações de leitura e escrita não bloquearão
     * se os dados não estiverem disponíveis ou se o buffer estiver cheio.
     * fcntl(fd, F_GETFL, 0) retorna as flags atuais do descritor de arquivo.
     * O_NONBLOCK é uma flag que define o descritor de arquivo para modo não bloqueante.
     */
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

// ── Cria e configura o socket do servidor ────────────────────────────────────
static int create_server(int porta)
{
    // PASSO 1: Cria o socket — pede ao SO o ponto de comunicação
    // AF_INET = IPv4 | SOCK_STREAM = TCP (entrega ordenada e garantida)
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    // SO_REUSEADDR: evita "Address already in use" ao reiniciar o servidor
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // PASSO 2: Configura o endereço e faz o bind
    // INADDR_ANY = aceita em qualquer interface | htons = big-endian (network byte order)
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(porta);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }

    // PASSO 3: Coloca em modo de escuta com backlog de 128 conexões na fila
    if (listen(fd, 128) < 0) {
        perror("listen"); close(fd); return -1;
    }

    set_nonblocking(fd); // server_fd também não bloqueia no accept()
    return fd;
}

// ── Remove um cliente do array de fds e do array de timestamps ───────────────
// Os dois vetores são paralelos: fds[i] e last_activity[i] sempre pertencem
// ao mesmo cliente. Remover um exige remover o outro no mesmo índice.
static void remover_cliente(std::vector<struct pollfd>& fds,
                             std::vector<time_t>& last_activity,
                             int index)
{
    std::cout << "[-] Cliente fd=" << fds[index].fd
              << " removido." << std::endl;
    close(fds[index].fd);
    fds.erase(fds.begin() + index);
    last_activity.erase(last_activity.begin() + index);
}

int main()
{
    // SIGPIPE: ignora para sobreviver a clientes que desconectam abruptamente
    signal(SIGPIPE, SIG_IGN);

    int server_fd = create_server(8080);
    if (server_fd < 0) return EXIT_FAILURE;

    // ── ARRAYS PARALELOS ─────────────────────────────────────────────────
    // fds[i]           = pollfd do fd (monitorado pelo poll)
    // last_activity[i] = timestamp (time_t) da última atividade do fd
    // Posição 0 de ambos = server_fd (não tem timeout, nunca é removido)
    std::vector<struct pollfd> fds;
    std::vector<time_t>        last_activity;

    struct pollfd server_pfd;
    server_pfd.fd      = server_fd;
    server_pfd.events  = POLLIN;  // vigiar novas conexões
    server_pfd.revents = 0;       // preenchido pelo SO com o que aconteceu
    fds.push_back(server_pfd);
    last_activity.push_back(time(NULL)); // server_fd não tem timeout real

    std::cout << "Servidor poll() na porta 8080 | timeout por inatividade: "
              << TIMEOUT_SECS << "s" << std::endl;
    std::cout << "Teste: nc localhost 8080 + Ctrl+Z para suspender o cliente." << std::endl;

    // ── LOOP PRINCIPAL — EVENT LOOP ──────────────────────────────────────
    // Cada iteração processa TODOS os eventos disponíveis sem bloquear em ninguém.
    while (true)
    {
        // poll() adormece até POLL_INTERVAL_MS milissegundos.
        // Retorna: >0 = há fds prontos | 0 = timeout expirou | -1 = erro
        int ready = poll(&fds[0], fds.size(), POLL_INTERVAL_MS);
        if (ready < 0) {
            perror("poll");
            break;
        }

        time_t agora = time(NULL);

        // ── VERIFICAÇÃO DE TIMEOUT (roda a cada POLL_INTERVAL_MS) ────────
        // Percorre de trás para frente para poder remover com segurança.
        // Ignora índice 0 (server_fd não tem timeout).
        for (int i = (int)fds.size() - 1; i >= 1; --i)
        {
            double inativo = difftime(agora, last_activity[i]);
            if (inativo >= TIMEOUT_SECS) {
                std::cout << "[TIMEOUT] fd=" << fds[i].fd
                          << " inativo por " << (int)inativo << "s — encerrando." << std::endl;
                remover_cliente(fds, last_activity, i);
            }
        }

        // Se poll() retornou 0 (só timeout expirou), não há eventos para processar
        if (ready == 0)
            continue;

        // ── PROCESSA EVENTOS DOS FDS ─────────────────────────────────────
        // Percorre de trás para frente para remover com segurança
        for (int i = (int)fds.size() - 1; i >= 0; --i)
        {
            if (fds[i].revents == 0)
                continue; // nenhum evento neste fd — pula

            // ── EVENTO no server_fd: nova conexão chegou ─────────────────
            if (fds[i].fd == server_fd)
            {
                // Loop para aceitar todos os clientes pendentes de uma vez
                while (true)
                {
                    int client_fd = accept(server_fd, NULL, NULL);
                    if (client_fd < 0)
                        break; // EAGAIN = não há mais conexões na fila

                    set_nonblocking(client_fd);

                    struct pollfd cpfd;
                    cpfd.fd      = client_fd;
                    cpfd.events  = POLLIN;  // vigiar dados chegando
                    cpfd.revents = 0;       // preenchido pelo SO
                    fds.push_back(cpfd);
                    last_activity.push_back(time(NULL)); // registra conexão

                    std::cout << "[+] fd=" << client_fd
                              << " conectado. Total: " << fds.size() - 1
                              << " | timeout em " << TIMEOUT_SECS << "s sem atividade." << std::endl;
                }
                continue;
            }

            // ── EVENTO em client_fd ──────────────────────────────────────
            int client_fd = fds[i].fd;

            // POLLHUP ou POLLERR: cliente fechou (Ctrl+C) ou erro de rede
            if (fds[i].revents & (POLLHUP | POLLERR)) {
                std::cout << "[-] fd=" << client_fd << " HUP/ERR — removendo." << std::endl;
                remover_cliente(fds, last_activity, i);
                continue;
            }

            // POLLIN: dados disponíveis para ler
            if (fds[i].revents & POLLIN)
            {
                char buffer[256];
                bzero(buffer, sizeof(buffer));

                int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

                if (bytes <= 0) {
                    // 0 = EOF (Ctrl+D no nc), <0 = erro
                    std::cout << "[-] fd=" << client_fd << " EOF — removendo." << std::endl;
                    remover_cliente(fds, last_activity, i);
                    continue;
                }

                // Atividade registrada: reseta o timer de timeout deste cliente
                last_activity[i] = time(NULL);

                std::cout << "[fd=" << client_fd << " RECV " << bytes
                          << "b] " << buffer;

                // Eco: devolve os mesmos bytes recebidos
                send(client_fd, buffer, bytes, 0);
            }
        }
    }

    for (size_t i = 0; i < fds.size(); ++i)
        close(fds[i].fd);

    return 0;
}
