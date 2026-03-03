// Eco de 1 Socket — Versão com acumulador de buffer + boundary detector
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o eco eco-de-1-socket.cpp
// Testar:
//   Mensagem simples:    echo -e "oi\r\n\r\n" | nc localhost 8080
//   Sem boundary:        echo -n "sem fim" | nc localhost 8080
//   Mensagem em partes:  printf "parte1\r\n" | nc -q1 localhost 8080
//
// OBJETIVO DESTE EXPERIMENTO:
//   Demonstrar que recv() entrega fragmentos — não mensagens completas.
//   O servidor deve ACUMULAR os bytes até encontrar o marcador de fim (\r\n\r\n),
//   exatamente como o Webserv precisará fazer com requisições HTTP.

#include <iostream>
#include <string>        // std::string para acumular o buffer
#include <cstdio>        // perror
#include <cstring>       // bzero
#include <cstdlib>       // EXIT_FAILURE
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// Marcador de fim de mensagem — o mesmo que o HTTP usa para encerrar os headers
static const std::string BOUNDARY = "\r\n\r\n";

int main()
{
    // ── PASSO 1: Cria o socket ──────────────────────────────────────────
    // socket() pede um "ponto de comunicação" ao SO.
    // AF_INET     = usa IPv4
    // SOCK_STREAM = usa TCP (entrega ordenada e garantida)
    // Retorna um file descriptor (fd) — trate como um arquivo aberto.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return EXIT_FAILURE; }

    // SO_REUSEADDR: evita "Address already in use" ao reiniciar o servidor.
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    // ── PASSO 2: Configura o endereço e faz o bind ──────────────────────
    // bind() mapeia o socket a uma porta específica da máquina.
    // INADDR_ANY = aceita conexões em qualquer interface (localhost, eth0...).
    // htons()    = converte a porta para "network byte order" (big-endian),
    //              necessário para que máquinas de arquiteturas diferentes se entendam.
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind"); close(server_fd); return EXIT_FAILURE;
    }

    // ── PASSO 3: Coloca o socket em modo de escuta ──────────────────────
    // listen() sinaliza ao SO que estamos prontos para receber conexões.
    // O segundo argumento (10) é o backlog — quantas conexões podem aguardar
    // na fila do SO antes de serem aceitas pelo accept().
    if (listen(server_fd, 10) < 0) {
        perror("listen"); close(server_fd); return EXIT_FAILURE;
    }

    std::cout << "Servidor rodando na porta 8080." << std::endl;
    std::cout << "Envie uma mensagem terminada com \\r\\n\\r\\n para completar." << std::endl;

    // ── LOOP PRINCIPAL ───────────────────────────────────────────────────
    // Mantém o servidor vivo indefinidamente, atendendo um cliente por vez.
    int conexao = 0;
    while (true)
    {
        std::cout << "\n[AGUARDANDO] Conexão #" << ++conexao << "..." << std::endl;

        socklen_t addrlen = sizeof(address);
        int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_fd < 0) { perror("accept"); continue; }

        std::cout << "[CONECTADO] fd=" << client_fd << std::endl;

        // ── ACUMULADOR DE BUFFER ─────────────────────────────────────────
        // Acumula todos os fragmentos recebidos numa std::string.
        // Só processamos quando o boundary \r\n\r\n for encontrado — igual ao HTTP.
        // Cada recv() pode trazer apenas parte da mensagem: isso é o TCP stream.
        std::string acumulado;
        char chunk[64]; // buffer pequeno PROPOSITALMENTE para forçar fragmentação
        int  bytes_lidos;
        int  fragmento = 0;

        while (true)
        {
            bzero(chunk, sizeof(chunk));

            // recv() com buffer de 64 bytes: garante que mensagens grandes
            // cheguem em múltiplos fragmentos — tornando o stream visível.
            bytes_lidos = recv(client_fd, chunk, sizeof(chunk) - 1, 0);

            if (bytes_lidos <= 0) {
                std::cout << "[DESCONECTADO] Cliente fechou sem enviar boundary." << std::endl;
                break;
            }

            ++fragmento;
            acumulado += chunk; // cola o fragmento no acumulador

            std::cout << "[FRAG #" << fragmento << " | "
                      << bytes_lidos << " bytes | total=" << acumulado.size()
                      << "] recebido" << std::endl;

            // Verifica se a mensagem completa já chegou (boundary encontrado)
            if (acumulado.find(BOUNDARY) != std::string::npos) {
                std::cout << "\n[MENSAGEM COMPLETA em " << fragmento
                          << " fragmento(s)]" << std::endl;
                std::cout << "Conteúdo: [" << acumulado << "]" << std::endl;

                // Resposta ao cliente — sem o boundary
                std::string resposta = "OK: mensagem recebida em "
                    + std::string(1, '0' + (fragmento / 10))
                    + std::string(1, '0' + (fragmento % 10))
                    + " fragmento(s)\r\n";
                send(client_fd, resposta.c_str(), resposta.size(), 0);
                break;
            }
        }

        close(client_fd);
        std::cout << "[FECHADO] Pronto para próxima conexão." << std::endl;
    }

    close(server_fd);
    return 0;
}
