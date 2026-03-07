// 02-http-config-parser.cpp — Analisador Léxico (Tokenizer) básico de Conf
// Compilar: c++ -std=c++98 -Wall -Wextra -Werror -o conf_parser 02-http-config-parser.cpp
//
// TESTAR NO TERMINAL:
//   Passo 1: /tmp/conf_parser ./exemplo.conf
//   Passo 2: Edite o ./exemplo.conf (tire um ponto-e-vírgula) e veja a quebra sintática atuando!
//
// CONCEITO DEMONSTRADO:
//   - Sanitização de Comentários de Hastag (#)
//   - Extração do Fluxo Textual em Array de Strings (Tokens)
//   - Identificação do Bloco Estrutural "server" e sintaxe com ";" e "{ }"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// ── O Coração: Etapa Lexer (Extrator de Tokens Inteligente) ────────────────
static std::vector<std::string> get_tokens(const std::string& filepath) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        std::cerr << "Falha Crítica Ocorreu: Arquivo [" << filepath << "] imacessível." << std::endl;
        exit(1);
    }

    std::vector<std::string> tokens;
    std::string line;
    
    while (std::getline(file, line)) {
        // [Etapa 1] Sanitização (Remove Comentários Nginx)
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        // [Etapa 2] Transformando Formatação Estruturante em Tokens Individuais
        // (Isso é obrigatório pois o usuario pode socar 'server{' ser espaçá-lo)
        std::string processed_line;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '{' || line[i] == '}' || line[i] == ';') {
                processed_line += " "; // Espaçamento na Frente
                processed_line += line[i];
                processed_line += " "; // Espaçamento Atrás
            } else {
                processed_line += line[i];
            }
        }
        
        // [Etapa 3] Separação formal do Vetor pelo Delimitador Espaço Branco
        std::stringstream ss(processed_line);
        std::string token;
        while (ss >> token) {
            tokens.push_back(token);
        }
    }
    
    file.close();
    return tokens;
}

// ── Abstract Syntax Analyzer (AST) Demonstrativo ─────────────────────────────
// Objetivo: Validar a ordem estrita imposta pelo Lexer. Verifica Ponto-E-Vírgulas Faltantes.
static void parse_and_validate(const std::vector<std::string>& tokens) {
    std::cout << "\n=== Iniciando Verificação de Sintaxe CGI / Webserv ===\n" << std::endl;
    
    int bracket_balance = 0; // Se não for 0 no final do arquivo, está corrompido!
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::string t = tokens[i];
        
        if (t == "{") {
            bracket_balance++;
        }
        else if (t == "}") {
            bracket_balance--;
            if (bracket_balance < 0) {
                std::cerr << "[EMERGENCIA] Chave End '}' Solta sem Abertura: Token " << i << std::endl;
                exit(1);
            }
        }
        else if (t == "listen" || t == "root" || t == "autoindex") { // Simulação de Diretivas Conhecidas
            std::cout << "[ AST ] Diretiva Encontrada: " << t;
            
            // O próximo Token tem que ser o VALOR associado (ex: "8080" ou "/var/www")
            if (i + 1 >= tokens.size() || tokens[i+1] == "{" || tokens[i+1] == "}" || tokens[i+1] == ";") {
                std::cerr << "\n[EMERGENCIA] Diretiva '" << t << "' não possui Valores definidos!" << std::endl;
                exit(1);
            }
            std::cout << " -> " << tokens[i+1];
            
            // O Valor (ou Cadeia de Valores) sempre deve terminar no Semmicolon Nginx
            // O parser real daria um WHILE até encontrar o `;`. No Nginx simples, é no index + 2
            if (i + 2 >= tokens.size() || tokens[i+2] != ";") {
                std::cerr << "\n[EMERGENCIA] Ponto e Vírgula (';') Obrigatório Faltante Pós [" << tokens[i+1] << "]" << std::endl;
                exit(1);
            }
            std::cout << " (Finalizada e Válida)\n";
            i += 2; // Pula a checagem manual do Valor e Ponto e Vírgula da Main Thread
        }
    }

    if (bracket_balance != 0) {
        std::cerr << "[EMERGENCIA] Contexto sem fechamento adequado '}'. Arquivo Incompleto." << std::endl;
        exit(1);
    }

    std::cout << "\n[SUCESSO] Conf Válido e Testado! Nenhum erro Sintático no Servidor." << std::endl;
}

// ── Boilerplate Testador Automático ──────────────────────────────────────────
int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Informe arquivo .conf: ./conf_parser <arquivo.conf>" << std::endl;
        return 1;
    }

    // Etapa 1: Pega do Disco pro Array
    std::vector<std::string> token_chain = get_tokens(argv[1]);

    // Opcional Acadêmico: Imprime a Tokenização Híbrida
    std::cout << "[LEXER] Total extraído do código de máquina: " << token_chain.size() << " Tokens" << std::endl;
    for (size_t x = 0; x < token_chain.size(); ++x) {
        // std::cout << "[" << token_chain[x] << "] ";
    }
    
    // Etapa 2: A Validação Crítica da Árvore de Sintaxe
    parse_and_validate(token_chain);

    return 0;
}
