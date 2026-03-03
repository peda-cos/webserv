// 02-tcp-stream.cpp — Cliente TCP com envio fragmentado proposital
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o stream 02-tcp-stream.cpp
//
// COMO USAR:
//   Terminal 1: /tmp/eco          (servidor do eco-de-1-socket, ainda rodando)
//   Terminal 2: /tmp/stream       (este cliente)
//
// OBJETIVO:
//   Demonstrar o stream TCP do LADO DO CLIENTE — enviando a mesma requisição
//   em 3 fragmentos deliberados com pausa entre eles.
//   O servidor recebe pedaços e acumula no buffer até encontrar \r\n\r\n.
//   Isso replica exatamente o que acontece com browsers lentos ou conexões 3G.

#include <iostream>
#include <string>
#include <cstdio>        // perror
#include <cstring>       // bzero
#include <cstdlib>       // EXIT_FAILURE
#include <sys/socket.h>  // socket, connect, send, recv
#include <netinet/in.h>  // sockaddr_in, htons
#include <arpa/inet.h>   // inet_addr
#include <unistd.h>      // close, usleep

int main()
{
    // ── PASSO 1: Cria o socket do CLIENTE ───────────────────────────────
    // Igual ao servidor, mas agora não faremos bind — o SO escolhe a porta.
    // AF_INET     = usa IPv4
    // SOCK_STREAM = usa TCP
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) { perror("socket"); return EXIT_FAILURE; }

    // ── PASSO 2: Configura o endereço do SERVIDOR (destino) ─────────────
    // em vez de INADDR_ANY (qualquer interface), especificamos onde conectar.
    // inet_addr("127.0.0.1") = "localhost" em formato binário para o SO.
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // ── PASSO 3: Conecta ao servidor ────────────────────────────────────
    // connect() é o oposto do accept() — o cliente vai até o servidor.
    // Bloqueia até a conexão ser estabelecida (handshake TCP de 3 vias).
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        std::cerr << "Certifique-se que /tmp/eco está rodando em outro terminal." << std::endl;
        close(client_fd);
        return EXIT_FAILURE;
    }
    std::cout << "[CONECTADO] ao servidor 127.0.0.1:8080" << std::endl;

    // ── PASSO 4: Envia a mensagem em 3 FRAGMENTOS DELIBERADOS ───────────
    // Uma requisição HTTP típica enviada "picotada" para simular rede lenta.
    // O \r\n\r\n no final do fragmento 3 é o boundary que o servidor aguarda.
    //
    // Fragmento 1: só o método e o início do caminho
    // Fragmento 2: o resto do caminho e os headers
    // Fragmento 3: o final com o boundary \r\n\r\n
    //
    // O servidor (eco) vai receber cada um separadamente e acumular.

    const char* frag1 = "GET /index";
    const char* frag2 = ".html HTTP/1.1\r\nHost: localhost";
    const char* frag3 = "\r\nAccept: */*\r\n\r\n";

    std::cout << "\n[ENVIANDO fragmento 1/3]: \"" << frag1 << "\"" << std::endl;
    send(client_fd, frag1, strlen(frag1), 0);

    // Pausa propositalmente para garantir que o OS entregue como pacote separado
    // usleep(microsegundos) — 200ms de delay simula rede lenta
    usleep(200000);

    std::cout << "[ENVIANDO fragmento 2/3]: \"" << frag2 << "\"" << std::endl;
    send(client_fd, frag2, strlen(frag2), 0);

    usleep(200000);

    std::cout << "[ENVIANDO fragmento 3/3]: \"" << frag3 << "\" (com boundary \\r\\n\\r\\n)" << std::endl;
    send(client_fd, frag3, strlen(frag3), 0);

    // ── PASSO 5: Recebe a resposta do servidor ───────────────────────────
    // Após enviar o boundary, o servidor processa e responde.
    // recv() aguarda aqui até a resposta chegar.
    char resposta[256];
    bzero(resposta, sizeof(resposta));
    int bytes = recv(client_fd, resposta, sizeof(resposta) - 1, 0);

    if (bytes > 0)
        std::cout << "\n[RESPOSTA DO SERVIDOR]: " << resposta << std::endl;
    else
        std::cout << "\n[SEM RESPOSTA] Servidor não respondeu ou desconectou." << std::endl;

    // ── PASSO 6: Fecha a conexão ─────────────────────────────────────────
    // close() libera o fd — o servidor receberá EOF e saberá que o cliente saiu.
    close(client_fd);
    std::cout << "[DESCONECTADO] Conexão encerrada." << std::endl;

    return 0;
}
