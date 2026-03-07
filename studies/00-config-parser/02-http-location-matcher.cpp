// 03-http-location-matcher.cpp — Simulador de AST do Conf (Herança e Prefix Match)
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o conf_matcher 03-http-location-matcher.cpp
//
// TESTAR NO TERMINAL:
//   /tmp/conf_matcher
//
// CONCEITO DEMONSTRADO:
//   - Como o C++ escolhe qual Servidor da porta 8080 atende a requisição (Server Name / Virtual Hosts)
//   - Como o `location /gatos` vence o `location /` na escolha de Rotas (Maximum Prefix Match)
//   - Como a classe `Location` herda o limite de Body Size da classe `Server` pai silenciosamente!

#include <iostream>
#include <string>
#include <vector>

// ── 1. Estruturas (AST) que simulam o que o seu Parser guardaria na RAM ──────

// Define as regras individuais de uma Rota (Location)
struct LocationConfig {
    std::string path_prefix;       // Ex: "/", "/admin", "/gatos"
    std::string root_folder;       // Ex: "/var/www/html/admin"
    bool autoindex;
    int client_max_body_size;      // isso existe tanto no Server quanto aqui!
    
    // Construtor Básico
    LocationConfig(const std::string& p, const std::string& r, bool ai) 
        : path_prefix(p), root_folder(r), autoindex(ai), client_max_body_size(-1) {}
};

// Define a Máquina Virtual de Atendimento (O "server {}")
struct ServerConfig {
    int listen_port;
    std::string server_name;       // Ex: "dev.com" ou "localhost"
    
    int client_max_body_size;      // A regra Global do Pai
    bool autoindex_default;        // A regra Global do Pai
    std::string default_error_page;
    
    std::vector<LocationConfig> locations; // Todos os sub-blocos que pertencem a ele
    
    ServerConfig(int port, const std::string& name, int max_size) 
        : listen_port(port), server_name(name), client_max_body_size(max_size), autoindex_default(false) {}

    // A MÁGICA DA HERANÇA (O Overwrite)
    // O Parser deve rodar isso ao terminar de ler o arquivo .conf
    void apply_inheritance() {
        for (size_t i = 0; i < locations.size(); ++i) {
            // Se o Location não definiu um max_body (está em -1), ele herda agressivamente o do Pai (Server)
            if (locations[i].client_max_body_size == -1) {
                locations[i].client_max_body_size = this->client_max_body_size;
            }
        }
    }
};

// Define O Gerente Geral que segura e roteia os Servers (O "ConfigManager")
struct ConfigManager {
    std::vector<ServerConfig> servers;

    // A MÁGICA DOS SERVIDORES VIRTUAIS (VHOSTS) e DEFAULTS
    // Dado uma porta (ex: 8080) e o cabeçalho 'Host' vindo do Chrome, quem responde?
    ServerConfig* find_server(int port, const std::string& host_header) {
        ServerConfig* default_server_fallback = NULL;

        for (size_t i = 0; i < servers.size(); ++i) {
            if (servers[i].listen_port == port) {
                
                // PELA REGRA DO NGINX: O primeiro Servidor da porta a ser lido É O DEFAULT FALLBACK
                if (default_server_fallback == NULL) {
                    default_server_fallback = &servers[i];
                }
                
                // Se o Header `Host` bate exatamente com o nome dado lá no .conf, é o Match Perfeito!
                if (servers[i].server_name == host_header) {
                    return &servers[i];
                }
            }
        }
        
        // Se o dominio "Host" enviado pelo Chrome for desconhecido, caímos matando no "Primeiro da Fila"
        return default_server_fallback; 
    }

    // A MÁGICA DA GUERRA DAS ROTAS (The Maximum Prefix Match Algorithm)
    // Dentro de UM servidor escolhido, qual `location` vence a disputa de Rota?
    LocationConfig* match_location(ServerConfig* server, const std::string& request_path) {
        LocationConfig* best_match = NULL;
        size_t max_matched_len = 0;

        for (size_t i = 0; i < server->locations.size(); ++i) {
            std::string prefix = server->locations[i].path_prefix;
            
            // O caminho da URL começa Exatamente igual o meu Prefix?
            if (request_path.find(prefix) == 0) {
                // É o maior Casamento de Letras que achei até agora?
                if (prefix.length() > max_matched_len) {
                    max_matched_len = prefix.length();
                    best_match = &server->locations[i];
                }
            }
        }
        return best_match;
    }
};

// ── Boilerplate: A Simulação do Gerente HTTP trabalhando ───────────────────────
int main() {
    ConfigManager manager;

    // 1. Simula que o Parser C++ leu o arquivo ".conf" perfeitamente e 
    // converteu as chaves { } nestas duas instâncias "ServerConfig" puras na memória RAM!
    
    // [Servidor 1] - O Primogênito (Vai ser o Default) | Escutando 8080
    ServerConfig srv1(8080, "site1.com", 100); // 100 Bytes limit de Body global
    srv1.locations.push_back(LocationConfig("/", "/var/www/site1", true));
    
    LocationConfig loc_images_srv1("/imagens", "/var/www/site1/imagens", false);
    loc_images_srv1.client_max_body_size = 500; // Aqui a regar "esmaga" os 100 Global do Pai!
    srv1.locations.push_back(loc_images_srv1);
    
    srv1.apply_inheritance(); // Herda os 100 Bytes pro Location "/" passivamente!
    manager.servers.push_back(srv1);

    // [Servidor 2] - O Virtual Host Secreto | TAMBÉM escutando a mesmíssima porta 8080!
    ServerConfig srv2(8080, "site2.com", 2000); // 2000 Bytes limit de Body global
    srv2.locations.push_back(LocationConfig("/", "/var/www/site2", true));
    srv2.apply_inheritance();
    manager.servers.push_back(srv2);

    std::cout << "========= LABORATORIO DE ROTEAMENTO (VHOSTS E PREFIX) =========\n\n";

    // -------------------------------------------------------------------------
    // TESTE 1: Virtual Host Correto vs Default
    std::cout << "[TESTE 1] Req TCP: Porta 8080 | Host: site2.com" << std::endl;
    ServerConfig* winner_srv = manager.find_server(8080, "site2.com");
    if (winner_srv) std::cout << "   -> Atendido pelo Servidor [" << winner_srv->server_name << "]" << std::endl;
    // Ele consegue achar certinho o srv2 isolando o dominio em tempo O(N)

    // TESTE 2: O domínio Falso caindo no Padrão do Webserv
    std::cout << "\n[TESTE 2] Req TCP: Porta 8080 | Host: ip-estranho.com (Falha VHOST)" << std::endl;
    winner_srv = manager.find_server(8080, "ip-estranho.com");
    if (winner_srv) std::cout << "   -> Cedeu ao Default Server [" << winner_srv->server_name << "]" << std::endl;
    // Cai no Primogênito

    // -------------------------------------------------------------------------
    // TESTE 3: Roteamento de Rotas (Prefix Match) + Herança Provada
    std::cout << "\n[TESTE 3] Validando Herança da Maior Rota no srv1 ('site1.com')" << std::endl;
    
    // Teste 3.1: A raiz simples
    std::string urlA = "/index.html"; 
    LocationConfig* loc_A = manager.match_location(&srv1, urlA);
    std::cout << "   -> URL: " << urlA << " casou com o location [" << loc_A->path_prefix << "]" << std::endl;
    std::cout << "      Limite MAX BODY herdado do Pai: " << loc_A->client_max_body_size << " bytes.\n";

    // Teste 3.2: O Match Máximo! A corda Bate!
    std::string urlB = "/imagens/gatos/peludos.jpg"; 
    LocationConfig* loc_B = manager.match_location(&srv1, urlB);
    std::cout << "\n   -> URL: " << urlB << " casou com o location [" << loc_B->path_prefix << "]" << std::endl;
    std::cout << "      Note como o '/imagens' VENCEU o '/', e puxou do seu BLOCO uma Override da Herança!" << std::endl;
    std::cout << "      Limte MAX BODY isolado apenas dessa rota: " << loc_B->client_max_body_size << " bytes.\n";

    return 0;
}
