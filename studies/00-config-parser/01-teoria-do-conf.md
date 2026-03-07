# 00 — O Arquivo de Configuração (.conf)

## História e Contexto: Por que um `.conf`?

Antes dos anos 90, os programas eram monólitos engessados. Se um administrador quisesse que o servidor rodasse na porta `8080` em vez da `80`, ele precisaria abrir o código em C, alterar a variável `int port = 80;`, recompilar o servidor inteiro e reiniciá-lo.

À medida que a web cresceu, essa abordagem se provou insustentável. Servidores como o Apache (1995) e depois o Nginx (2004) revolucionaram a infraestrutura separando a **Lógica C++** (como lidar com TCP e HTTP) da **Parametrização** (quais portas abrir, quais pastas servir).

Foi assim que nasceu o modelo de Arquivos de Configuração (Configuration Files ou `.conf`). O Webserv não apenas recria o Nginx, mas exige a formulação de um *parser* idêntico ao sintagma deste gigante russo.

---

## O Desafio Prático
O Webserv recebe o caminho de um arquivo texto puro como argumento:
```bash
./webserv /etc/nginx/meu_server.conf
```

Seu programa C++98 deve abrir esse arquivo, lê-lo de ponta a ponta, validá-lo e guardar todas essas matrizes de regras num objeto inteligente antes mesmo de sonhar em criar o `socket_fd` ou chamar o `poll()`.

Se o arquivo tiver um erro de sintaxe (faltar um `;` ou uma chave `{`), o C++ deve abortar a execução graciosamente informando onde ocorreu a quebra, exatamente como o comando `nginx -t` faria.

---

## A Gramática e a Sintaxe Nginx-like

O padrão Nginx assemelha-se vagamente à linguagem C, operando sob uma estrutura de **Contextos (Blocos)** e **Diretivas (Instruções)**.

```nginx
server {
    listen 8080;
    server_name localhost;
    root /var/www/html;
    client_max_body_size 5M;
    error_page 404 /404.html;

    location /imagens {
        root /var/www/midia;
        autoindex on;
    }

    location /admin {
        limit_except GET POST {
            deny all;
        }
    }
}
```

### 1. Diretivas Simples
São regras diretas que terminam obrigatoriamente com ponto-e-vírgula (`;`).
Exemplo: `listen 8080;` (Diretiva "listen", Valor "8080").

### 2. Contextos ou Blocos ( `{ }` )
Agrupam múltiplas diretivas sob um escopo. O principal contexto obrigatório do WebServ é o bloco `server { ... }`, que define uma "máquina virtual" de atendimento.
Dentro do Server, o projeto requer a implementação do sub-bloco `location /caminho { ... }`, que serve para sobrescrever regras gerais apenas quando os usuários acessarem rotas específicas.

---

## Criando o Parser em C++ (A Estratégia de Tokenization)

A abordagem ingênua é ler o arquivo linha por linha (`std::getline()`) e ficar caçando substrings. Essa tática sucumbe rapidamente, pois o usuário pode escrever tudo numa linha só ou colocar três diretivas na mesma linha, o que quebra o `getline()`.

```nginx
# Tática assassina para quebrar seu Parser Ingênuo:
server{listen 80;server_name test;}
```

A abordagem madura — a mesma usada por compiladores reais — divide-se em três etapas imperativas:

### Etapa 1: Limpeza (Sanitization)
Ler o arquivo desconsiderando integralmente tudo que estiver após um caractere `#` (comentários) e substituindo todas as quebras de linha `\n` e `\t` por fragmentos ignorados ou espaços simples.

### Etapa 2: Lexical Analysis (O Tokenizer)
Fatiar a string gigante remanescente num Array ou Vector de palavras exatas (Tokens). Chaves (`{`, `}`) e Ponto-e-vírgulas (`;`) deixam de ser formatação e se tornam "Tokens Especiais".
```cpp
// Array de Tokens extraídos:
["server", "{", "listen", "80", ";", "server_name", "test", ";", "}"]
```

### Etapa 3: Abstract Syntax Tree (AST) ou Construtor de Estados
O algoritmo percorre o Array do passo 2.
1. Achou a palavra `"server"`? O código deduz que deve vir uma chave `"{"` a seguir. Se não vier, `Throw Syntax_Error()`.
2. Se a chave abriu, instancie um novo Objeto `ServerConfig`.
3. O próximo Token é `"listen"`? Aja sabendo que o próximo Token DEVE ser uma string preenchível de porta e o Token conseguinte OBRIGATORIAMENTE será um `";"`.
4. Repetir incansavelmente até o fechamento da chave final `"}"`.

---

## Herança e Sobrescrita: O Princípio de Cascatas

Uma das maiores dificuldades lógicas desta etapa no projeto é a **Herança Contextual**.
Se existir uma diretiva `root /var/www;` estabelecida na raiz do bloco `server`, todos os `locations` ali inseridos herdam este Path Base silenciosamente.
Porém, se o bloco incrustar o sub-contexto `location /admin { root /mnt/seguro; }`, o C++ deve entender que a regra base foi esmagada naquele escopo (Overwrite), exigindo prioridade na avaliação das Rotas Dinâmicas (Módulo 03 de HTTP Routing).

O uso de `Structs` C++ que copiam preventivamente as variáveis do bloco-pai para o bloco-filho antes da varredura interna costuma blindar esta herança perfeitamente de falhas de arquitetura.
