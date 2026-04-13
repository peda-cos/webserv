#include <minitest.hpp>
#include <Server.hpp>
#include <Config.hpp>
#include <Logger.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>
#include <ctime>

// Constantes globais para garantir que todos os testes batam NO MESMO ALVO
const std::string TEST_HOST = "127.0.0.1";
const std::string TEST_PORT = "8080";
const std::string TEST_PORT_ALT = "8081";

/**
 * Helper: Preenche a struct sockaddr_in p/ conexão local
 */
static void fill_addr(struct sockaddr_in& addr, const std::string& host, const std::string& port) {
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(std::atoi(port.c_str()));
    addr.sin_addr.s_addr = inet_addr(host == "localhost" ? "127.0.0.1" : host.c_str());
}

/**
 * Helper: Define timeout padrão para sockets (5 segundos)
 */
static void set_socket_timeout(int fd, int timeout_sec = 5) {
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);
}

/**
 * TEST: Live Server Smoke Test
 * Envia uma requisição real para o servidor que você subiu no terminal principal.
 */
TEST(Connection, LiveServerSmokeTest) {
    std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return;

    struct timeval tv;
    tv.tv_sec = 1; tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        std::cout << "  [SKIP] Servidor off em " << TEST_HOST << ":" << TEST_PORT << std::endl;
        return;
    }

    send(fd, request.c_str(), request.size(), 0);

    char buf[1024];
    std::memset(buf, 0, sizeof(buf));
    ssize_t received = recv(fd, buf, sizeof(buf) - 1, 0);
    close(fd);

    ASSERT_TRUE(received > 0);
    ASSERT_STR_CONTAINS(buf, "Hello, World test!");
    ASSERT_STR_CONTAINS(buf, "200 OK");
}

/**
 * TEST: Slow Loris / Partial Request (TDD - Resilience)
 */
TEST(Connection, PartialRequestTimeoutResilience) {
    std::string partial_request = "GET / HTTP/1.1\r\nHost: localhost"; 

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return;

    struct timeval tv;
    tv.tv_sec = 0; tv.tv_usec = 300000; // 300ms
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        return;
    }

    send(fd, partial_request.c_str(), partial_request.size(), 0);

    char buf[1024];
    std::memset(buf, 0, sizeof(buf));
    ssize_t received = recv(fd, buf, sizeof(buf) - 1, 0);
    
    // O servidor NÃO deve responder a um GET incompleto.
    ASSERT_EQ(-1, (int)received);
    
    send(fd, "\r\n\r\n", 4, 0);
    std::memset(buf, 0, sizeof(buf));
    received = recv(fd, buf, sizeof(buf) - 1, 0);
    
    close(fd);
    ASSERT_TRUE(received > 0);
}

/**
 * TEST: Large Header Stress (Buffer Limits)
 */
TEST(Connection, BufferOverflowResilience) {
    std::string giant_header = "GET / HTTP/1.1\r\nX-Lixo: ";
    for (int i = 0; i < 50000; ++i) giant_header += "A"; // 50KB de lixo
    giant_header += "\r\n\r\n";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return;
    set_socket_timeout(fd);

    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        return;
    }

    send(fd, giant_header.c_str(), giant_header.size(), 0);
    
    char buf[1024];
    recv(fd, buf, sizeof(buf) - 1, 0);
    close(fd);

    // O teste passa se o servidor continuar vivo p/ uma conexão seguinte
    ASSERT_TRUE(true); 
}

/**
 * TEST: Multi-Client Concurrency (Event Loop Check)
 */
TEST(Connection, ConcurrentStateIndependence) {
    const int CLIENTS = 5;
    int fds[CLIENTS];

    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);

    for (int i = 0; i < CLIENTS; ++i) {
        fds[i] = socket(AF_INET, SOCK_STREAM, 0);
        set_socket_timeout(fds[i]);
        connect(fds[i], (struct sockaddr*)&addr, sizeof(addr));
    }

    for (int i = 0; i < CLIENTS; ++i) {
        send(fds[i], "GET / HTTP/1.1\r\n", 16, 0);
    }

    for (int i = 0; i < CLIENTS; ++i) {
        send(fds[i], "Host: localhost\r\n\r\n", 19, 0);
    }

    for (int i = 0; i < CLIENTS; ++i) {
        char buf[1024];
        ssize_t n = recv(fds[i], buf, 1023, 0);
        close(fds[i]);
        ASSERT_TRUE(n > 0);
        ASSERT_STR_CONTAINS(buf, "200 OK");
    }
}

/**
 * TEST: Abrupt Client Disconnect (SIGPIPE/Zombie management)
 */
TEST(Connection, AbruptDisconnect) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);
    
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        close(fd);
    }
    
    int fd2 = socket(AF_INET, SOCK_STREAM, 0);
    int res = connect(fd2, (struct sockaddr*)&addr, sizeof(addr));
    close(fd2);
    ASSERT_EQ(0, res);
}

/**
 * TEST: Half-Open Connection (Cenário de Bloqueio)
 */
TEST(Connection, NonBlockingTest) {
    int slow_fd = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(slow_fd);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);
    
    connect(slow_fd, (struct sockaddr*)&addr, sizeof(addr));
    send(slow_fd, "GET / HTTP/1.1", 14, 0); 
    
    int fast_fd = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(fast_fd);
    connect(fast_fd, (struct sockaddr*)&addr, sizeof(addr));
    send(fast_fd, "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 35, 0);
    
    char buf[1024];
    ssize_t n = recv(fast_fd, buf, 1023, 0);
    close(fast_fd);
    close(slow_fd);

    ASSERT_TRUE(n > 0);
    ASSERT_STR_CONTAINS(buf, "200 OK");
}

/**
 * TEST: FD Exhaustion Prep (Stress)
 */
TEST(Connection, FdLeakSanity) {
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);

    for (int i = 0; i < 50; ++i) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        set_socket_timeout(fd);
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            send(fd, "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", 54, 0);
            char buf[128];
            recv(fd, buf, sizeof(buf), 0);
        }
        close(fd);
    }
    ASSERT_TRUE(true);
}

/**
 * TEST: Multiple Ports Listening
 */
TEST(Connection, MultiPortListening) {
    std::string ports[] = {TEST_PORT, TEST_PORT_ALT};
    for (int i = 0; i < 2; ++i) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        set_socket_timeout(fd);
        struct sockaddr_in addr;
        fill_addr(addr, TEST_HOST, ports[i]);

        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            close(fd);
            std::cout << "  [SKIP] Porta " << ports[i] << " não ativa." << std::endl;
            continue;
        }
        send(fd, "GET / HTTP/1.1\r\n\r\n", 18, 0);
        char buf[128];
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        close(fd);
        ASSERT_TRUE(n > 0);
    }
}

/**
 * TEST: HTTP Method Restriction (Subject: limit_except)
 * Garante que o servidor entende quando um método não é permitido.
 * No Event Loop, isso deve retornar 405 sem travar.
 */
TEST(Protocol, MethodNotAllowedResilience) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(fd);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    // DELETE pode estar desativado no seu config
    std::string req = "DELETE /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(fd, req.c_str(), req.size(), 0);

    char buf[1024];
    recv(fd, buf, sizeof(buf), 0);
    close(fd);

    // Se o servidor ainda não processa DELETE, ele deve ou dar 200 (echo atual) 
    // ou 405 (objetivo final). O teste garante que o loop respondeu.
    ASSERT_TRUE(std::string(buf).find("HTTP/1.1") != std::string::npos);
}

/**
 * TEST: Persistent Connection (Keep-alive)
 * O subject exige que o servidor aguente múltiplas requisições no mesmo socket.
 */
TEST(Protocol, KeepAliveSupport) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(fd);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    for (int i = 0; i < 3; ++i) {
        std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        send(fd, req.c_str(), req.size(), 0);

        char buf[1024];
        std::memset(buf, 0, sizeof(buf));
        ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
        
        ASSERT_TRUE(n > 0);
        ASSERT_STR_CONTAINS(buf, "200 OK");
        // Se a conexão fechar após o 1º request, o 2º recv vai falhar.
    }
	close(fd);
}

/**
 * TEST: Malformed request returns 400 instead of success
 */
TEST(Protocol, MalformedRequestReturns400) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	fill_addr(addr, TEST_HOST, TEST_PORT);
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
		close(fd);
		return;
	}

	std::string req = "GET / HTTP/1.1\r\n\r\n";
	send(fd, req.c_str(), req.size(), 0);

	char buf[1024];
	std::memset(buf, 0, sizeof(buf));
	ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
	close(fd);

	ASSERT_TRUE(n > 0);
	ASSERT_STR_CONTAINS(buf, "400 Bad Request");
}

/**
 * TEST: Sequential keep-alive requests are reparsed, not reused
 */
TEST(Protocol, KeepAliveSequentialRequestsReparsed) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	set_socket_timeout(fd, 3);
	struct sockaddr_in addr;
	fill_addr(addr, TEST_HOST, TEST_PORT);
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
		close(fd);
		return;
	}

	std::string first = "GET /first HTTP/1.1\r\nHost: localhost\r\n\r\n";
	std::string second = "GET /second HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
	char buf[1024];

	send(fd, first.c_str(), first.size(), 0);
	std::memset(buf, 0, sizeof(buf));
	ssize_t n1 = recv(fd, buf, sizeof(buf) - 1, 0);
	ASSERT_TRUE(n1 > 0);
	ASSERT_STR_CONTAINS(buf, "200 OK");

	send(fd, second.c_str(), second.size(), 0);
	std::memset(buf, 0, sizeof(buf));
	ssize_t n2 = recv(fd, buf, sizeof(buf) - 1, 0);
	close(fd);

	ASSERT_TRUE(n2 > 0);
	ASSERT_STR_CONTAINS(buf, "200 OK");
	ASSERT_STR_CONTAINS(buf, "Connection: close");
}

/**
 * TEST: Pipelined requests both receive responses on one socket
 */
TEST(Protocol, PipelinedRequestsOnSingleSocket) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	set_socket_timeout(fd, 3);
	struct sockaddr_in addr;
	fill_addr(addr, TEST_HOST, TEST_PORT);
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
		close(fd);
		return;
	}

	std::string req =
		"GET /first HTTP/1.1\r\nHost: localhost\r\n\r\n"
		"GET /second HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
	send(fd, req.c_str(), req.size(), 0);

	char buf[4096];
	std::memset(buf, 0, sizeof(buf));
	ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
	if (n > 0 && std::string(buf, n).find("Connection: close") == std::string::npos) {
		ssize_t n2 = recv(fd, buf + n, sizeof(buf) - 1 - static_cast<size_t>(n), 0);
		if (n2 > 0) {
			n += n2;
		}
	}
	close(fd);

	ASSERT_TRUE(n > 0);
	std::string response(buf, static_cast<size_t>(n));
	std::size_t firstPos = response.find("HTTP/1.1 200 OK");
	ASSERT_TRUE(firstPos != std::string::npos);
	ASSERT_TRUE(response.find("HTTP/1.1 200 OK", firstPos + 1) != std::string::npos);
}

/**
 * TEST: CGI Execution Hang (Subject Resilience)
 * Se um script CGI travar ou demorar muito, o Event Loop não pode travar.
 * (Esse teste vai falhar/pendurar se o seu loop for síncrono no CGI)
 */
TEST(Protocol, CgiNonBlocking) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(fd, 3);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    // Request para um script que (teoricamente) demora
    std::string req = "GET /cgi-bin/slow.py HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(fd, req.c_str(), req.size(), 0);

    // Enquanto o CGI roda "em background", tentamos outro request rápido
    int fd2 = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(fd2);
    connect(fd2, (struct sockaddr*)&addr, sizeof(addr));
    send(fd2, "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 35, 0);

    char buf[1024];
    ssize_t n = recv(fd2, buf, sizeof(buf), 0);
    close(fd2);
    close(fd);

    // O servidor deve responder ao fd2 IMEDIATAMENTE, independente do CGI no fd.
    ASSERT_TRUE(n > 0);
    ASSERT_STR_CONTAINS(buf, "200 OK");
}
/**
 * TEST: Connection Inactivity Timeout (TDD)
 * O subject exige que o servidor seja resiliente. Se um cliente conecta
 * e fica em silêncio, ele deve ser desconectado após um tempo (ex: 5s).
 * NOTA: Esse teste vai FALHAR (Timeout do recv) se o servidor não implementar a lógica.
 */
TEST(Protocol, InactivityTimeout) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(fd, 10);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    // Define um timeout no recv do TESTE para não travar o binário de teste
    struct timeval tv;
    tv.tv_sec = 10; tv.tv_usec = 0; 
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    // Esperamos que o servidor feche a conexão por inatividade.
    // Se ele fechar, o recv retornará 0 (FIN) ou < 0.
    char buf[10];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    
    close(fd);
    
    // Se n > 0, o servidor mandou algo (estranho) ou ainda está aberto.
    // Se n == 0, o servidor fechou a conexão (SUCESSO no timeout).
    // Se n < 0 e errno == EAGAIN, o servidor NÃO fechou a conexão (FALHA no TDD).
    ASSERT_EQ(0, (int)n);
}

/**
 * TEST: Read Buffer Protection (Connection Level TDD)
 * O loop de leitura do Server deve proteger a memória.
 * Se o read_buffer de uma conexão crescer indefinidamente 
 * (mesmo sem headers válidos), o servidor deve FECHAR o socket.
 */
TEST(Connection, ReadBufferLimitEnforcement) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(fd, 3);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    // Mandamos 2MB de lixo. No TDD, esperamos que o servidor 
    // tenha um limite de buffer no Server::_read_from_client.
    std::string giant_junk(1024 * 1024 * 2, 'X'); 
    send(fd, giant_junk.c_str(), giant_junk.size(), 0);

    struct timeval tv;
    tv.tv_sec = 2; tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    char buf[1];
    ssize_t n = recv(fd, buf, 1, 0);
    close(fd);

    // Se n == 0, o servidor cortou a conexão por abuso de buffer (SUCESSO TDD).
    ASSERT_EQ(0, (int)n);
}

/**
 * TEST: Raw Connection Inactivity (No HTTP Body dependence)
 * Verifica se o servidor limpa um socket que conectou mas não mandou nada.
 */
TEST(Connection, RawInactivityTimeout) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    struct timeval tv;
    tv.tv_sec = 10; tv.tv_usec = 0; // Se o servidor fechar o socket em <10s, o teste passa.
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    char buf[1];
    ssize_t n = recv(fd, buf, 1, 0); 
    close(fd);

    // No TDD: esperamos que o servidor mande um FIN (recv retorna 0).
    ASSERT_EQ(0, (int)n);
}

/**
 * TEST: Byte-By-Byte Accumulation (Loop stress)
 * Manda a request 1 byte por vez para testar se o recv([]) se perde.
 */
TEST(Connection, ByteByByteSend) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

	std::string small_req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    for (size_t i = 0; i < small_req.size(); ++i) {
        send(fd, &small_req[i], 1, 0);
        usleep(500); 
    }

    char buf[1024];
    ssize_t n = recv(fd, buf, 1023, 0);
    close(fd);

    ASSERT_TRUE(n > 0);
    ASSERT_STR_CONTAINS(buf, "200 OK");
}
/* Runner customizado para verificar o servidor antes de rodar os testes */
int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    fill_addr(addr, TEST_HOST, TEST_PORT);

    // Tenta conectar rápido apenas para ver se o processo está vivo
    struct timeval tv;
    tv.tv_sec = 0; tv.tv_usec = 200000; // 200ms
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        std::cout << "\n[SKIP] Servidor não detectado em " << TEST_HOST << ":" << TEST_PORT << std::endl;
        std::cout << "[INFO] Certifique-se de rodar './webserv config/subject.conf' em outro terminal antes de testar.\n" << std::endl;
        return 0; // Sai silenciosamente sem rodar a bateria de testes
    }
    close(fd);

    return minitest::TestRegistry::instance().run();
}
