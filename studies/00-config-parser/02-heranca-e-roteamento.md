# 02 — Herança, Roteamento e Múltiplos Servidores

## O Problema dos Servidores no Mesmo Endereço (Virtual Hosts)

A etapa mais traiçoeira do `00-config-parser` não é fatiar o texto (Lexer), mas entender **como o Webserv decide** qual bloco `server {}` responderá a uma requisição se houver duas máquinas configuradas para a *mesma exata porta* (ex: 8080).

Imagine um `.conf` assim:

```nginx
# Servidor A
server {
    listen 8080;
    server_name www.gatos.com;
    root /var/www/gatos;
}

# Servidor B
server {
    listen 8080;
    server_name www.cachorros.com;
    root /var/www/cachorros;
}
```

Ambos os servidores físicos irão levantar o SO vinculando-se à Porta 8080 (`bind(8080)`). 
Quando o Cliente ("Chrome") enviar uma requisição TCP na 8080, quem responde? O Servidor A ou o Servidor B?

### O Papel do `server_name` e o Cabeçalho `Host:`

HTTP/1.1 introduziu o header `Host:` pensando justamente nisso. No momento do Parse (Departamento 04), o C++ receberá a string da requisição.

Se o Browser enviar:
```http
GET / HTTP/1.1
Host: www.cachorros.com:8080
```
O algoritmo deve iterar sobre todos os objetos Server que mapeiam a porta 8080. Se o header bater perfeitamente com o campo `server_name` do **Servidor B**, o roteamento interno repassa a leitura dos diretórios pro `root` do B (`/var/www/cachorros`).

---

## O "Default Server" Fallback

O que acontece se o cliente usar um endereço IP direto (Host: 192.168.0.5) ou enviar um nome de domínio não catalogado (Host: site-aleatorio.com)?

O Nginx possui a regra do **Default Server**: O **primeiro bloco `server {}`** lido pelo C++ (no topo do arquivo arquivo .conf) que casar com a "Porta e IP" exigidos no Host **assume a responsabilidade total** da requisição "órfã".

- *Restrição:* Essa triagem deve ser estritamente controlada pela instância ConfigManager da raiz (Módulo 00), alimentando o Parser com o Server Config correto.

---

## Herança de Bloco Pai para Bloco Filho (`location {}`)

Quando o C++ instanciar um bloco genérico (ex: o objecto Server A do Gatos), de dezenas de `location {}` podem derivar dele. O comportamento de escopo é fundamental.

```nginx
server {
    listen 80;
    root /var/www/html;
    client_max_body_size 10M;   # Regra PAI
    autoindex off;              # Regra PAI

    location /imagens {
        autoindex on;           # FILHO esmaga autoindex (Overwrite)
    }                           # MAS herda 10M passivamente.

    location /videos {
        client_max_body_size 50M; # FILHO esmaga 10M
    }
}
```

### O Desafio da Cópia no Lexer C++
No instante em que a Árvore de Sintaxe C++ for povoar a classe `LocationConfig`, antes de ler as diretivas locais (`autoindex on`), o algoritmo já deve puxar uma cópia espelho integral do pai (`ServerConfig`). 
Essa clonagem de propriedades viabiliza que as informações não alteradas permaneçam herdadas silenciosamente (Deep Copy ou Assignment Operator).

---

## O Roteamento de Locations (Match Priority)

Seja a Requisição HTTP exigindo a imagem: `GET /gatos/filhotes/foto.jpg`.
E no Conf existam dois blocos location disputando:

1. `location / { ... }`
2. `location /gatos { ... }`

**A Regra da Compatibilidade Nginx (Prefix Match)**
O Nginx não prioriza por ordem no arquivo `.conf`. A escolha do Webserv do bloco final que rege a lei recai sobre a "Maior String Compatível com a Rota Pedida" (Maximum Prefix Match).

- A rota exigida contém `/gatos/filhotes/foto.jpg`.
- `location /` bate com apenas **1 caractere** no inicio.
- `location /gatos` bate com **6 caracteres perfeitos**.

O bloco de `/gatos` vence o despache da requisição e esmaga o / raiz.

> *Dica Crítica C++:* Para gerir essa classificação rápida, a estrutura que armazena na memória as strings das Locations não deve ser num `std::vector` raso, mas iteradas por tamanho reverso, ou dispostas em blocos preordenados para validar o prefix_match sem perda de performance CPU a cada requisição TCP.
