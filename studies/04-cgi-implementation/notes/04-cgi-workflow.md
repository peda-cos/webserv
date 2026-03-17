# CGI — Workflow de Integração no Servidor Web

## Bird's Eye View: Onde CGI Encaixa no Projeto

```
┌─────────────────────────────────────────────────────────────┐
│ CLIENTE                                                     │
└──────────────────┬──────────────────────────────────────────┘
                   │
        Envia: GET /cgi-bin/script.py?name=joao
                   │
┌──────────────────┴──────────────────────────────────────────┐
│ SERVIDOR LISTEN (Socket)                                    │
│                                                             │
│  - accept() conexão                                         │
│  - push para poll() ou select()                             │
└──────────────────┬──────────────────────────────────────────┘
                   │
     ┌─────────────┴─────────────┐
     │                           │
┌────▼──────────────┐   ┌────────▼──────────────┐
│ EVENT LOOP        │   │ HTTP PARSER           │
│ (poll/select)     │   │ (...)                 │
│                   │   │ ├─ Request line       │
│ Uma thread, muitos│   │ ├─ Headers            │
│ clientes          │   │ └─ Body               │
└────┬──────────────┘   └────┬───────────────────┘
     │                        │
     └────────────┬───────────┘
                  │
    ┌─────────────▼─────────────┐
    │ URL MATCHER               │
    │                           │
    │ Path: /cgi-bin/script.py  │
    │ ├─ Static file? NO        │
    │ ├─ CGI? YES               │
    │ └─ Handler: CGI Executor  │
    └─────────────┬─────────────┘
                  │
    ┌─────────────▼─────────────┐
    │ CGI EXECUTOR              │
    │ (Este módulo)             │
    │                           │
    │ 1. fork()                 │
    │ 2. pipes IO               │
    │ 3. setenv()               │
    │ 4. exec()                 │
    │ 5. read output            │
    │ 6. waitpid()              │
    └─────────────┬─────────────┘
                  │
    ┌─────────────▼─────────────┐
    │ RESPONSE BUILDER          │
    │                           │
    │ Output do CGI → HTTP 200  │
    │ Status: 200 OK            │
    │ Body: <output>            │
    └─────────────┬─────────────┘
                  │
         Envia: HTTP/1.1 200 OK
                  │
┌────────────────────────────────────────────────────────────┐
│ CLIENTE RECEBE RESPOSTA                                    │
└────────────────────────────────────────────────────────────┘
```

---

## Ponto de Integração: URL Matching → Decision Point

### Onde Decidir: CGI ou Static?

**Opção 1: Por Diretório**
```
Se path começa com /cgi-bin/ → CGI
Senão → Static file
```

**Opção 2: Por Extensão**
```
Se path termina com .php, .py, .cgi → CGI
Senão → Static file
```

**Opção 3: Por Configuração**
```
location /cgi-bin/ {
    # Tudo dentro daqui é CGI
}
```

### Pseudocódigo

```cpp
void handle_request(HttpRequest req, Config config) {
    
    // 1. Parse HTTP request
    std::string method = req.method();       // GET, POST, etc
    std::string path = req.path();           // /cgi-bin/script.py
    std::string query = req.query_string();  // a=1&b=2
    size_t content_length = req.body().size();
    std::string body = req.body();           // POST data
    
    // 2. Match URL to location config
    LocationConfig location = config.match_location(path);
    
    // 3. Decide: CGI or Static?
    if (location.is_cgi_location()) {
        // ───────── CGI HANDLER ─────────
        cgi_execute(method, path, query, content_length, body);
    } 
    else {
        // ───────── STATIC FILE HANDLER ─────────
        serve_static_file(path);
    }
}
```

---

## Estrutura de Dados Necessárias

### LocationConfig (Ampliado)

```cpp
struct LocationConfig {
    std::string path;                    // /cgi-bin/
    std::vector<std::string> index;      // Sempre presente
    std::string root;                    // Diretório base
    size_t client_max_body_size;
    bool is_cgi;                         // ← NEW: Sinaliza CGI location
    std::string cgi_interpreter;         // ← NEW: python, php, etc
    std::map<int, std::string> error_pages;
};
```

### Configuração `.conf`

```nginx
# Arquivo de configuração esperado
server {
    listen 8080;
    server_name localhost;
    root /var/www/html;
    
    # Location estático
    location / {
        index index.html;
        client_max_body_size 1M;
    }
    
    # Location CGI
    location /cgi-bin/ {
        # Indica que arquivos aqui são CGI
        # Detectar por: ter x bit, ou extensão, ou diretório
    }
}
```

---

## Fluxo Detalhado: Request → Response

### 1. Request Parsing

```
Cliente envia:
┌────────────────────────────────────┐
│ POST /cgi-bin/login.py HTTP/1.1    │
│ Host: localhost:8080               │
│ Content-Type: app/x-www-form-data  │
│ Content-Length: 25                 │
│                                    │
│ username=admin&password=pass123    │
└────────────────────────────────────┘

Servidor extrai:
{
 method: "POST",
 path: "/cgi-bin/login.py",
 query: "",  // Vazio em POST
 headers: {...},
 body: "username=admin&password=pass123",
 content_length: 25
}
```

### 2. URL Matching

```cpp
// Config tem:
// location / → static
// location /cgi-bin/ → cgi

path = "/cgi-bin/login.py"

if (path.find("/cgi-bin/") == 0) {
    location = cgi_location;
}
```

### 3. CGI Preparation

```cpp
// Preparar variáveis para o script

script_path = "/var/www/cgi-bin/login.py";
server_name = "localhost";
server_port = "8080";
request_method = "POST";
script_name = "/cgi-bin/login.py";
query_string = "";  // POST não usa URL query
content_length = 25;
content_type = "application/x-www-form-urlencoded";
post_body = "username=admin&password=pass123";
```

### 4. CGI Execution (Fork + Exec)

```
┌──────────────┐
│ Server Pai   │
│              │─────────────┐
│              │             │
└──────────────┘        fork()
                             │
           ┌─────────────────┼─────────────────┐
           │                 │                 │
      Retorna PID        ┌────▼─────┐           │
           │             │ child    │           │
           │             │ setup    │           │
           │             │ pipes    │           │
           │             │ setenv   │           │
           │             │ exec()   │           │
           │             └────┬─────┘           │
           │                  │                 │
           │          Script agora rodando      │
           │          (login.py)                │
           │                  │                 │
      write(stdin_pipe[1],    │ read(stdin_pipe[0],
      post_body)               │  "username=admin...")
           │                   │
      close(stdin_pipe[1])     │
      ← sinaliza EOF ──────────┤
           │
      read(stdout_pipe[0])     │ write(stdout_pipe[1],
           │                   │  "<html>Login OK</html>")
           │                   │
      resposta_html ← ────────┘ exit(0)
           │
      waitpid(pid)
      ← Status: exited=0
           │
```

### 5. Response Building

```cpp
// Script terminou, saída foi capturada
response_body = "<html>Login OK</html>";
exit_code = 0;

// Montar resposta HTTP
std::string response = "";
response += "HTTP/1.1 200 OK\r\n";
response += "Content-Type: text/html\r\n";
response += "Content-Length: " + to_string(response_body.size()) + "\r\n";
response += "Connection: close\r\n";
response += "\r\n";
response += response_body;

// Enviar para cliente
send(client_socket, response);
```

---

## Tratamento de Erros CGI

| Situação | Ação |
|----------|------|
| Script não existe | 404 Not Found |
| Script não é executável | 403 Forbidden |
| fork() falha | 500 Internal Server Error |
| exec() falha | 500 Internal Server Error |
| Script timeout (muito lento) | 504 Gateway Timeout (opcional) |
| Script retorna erro (exit != 0) | Use `WIFEXITED`, `WEXITSTATUS` para verificar |

### Código de Verificação

```cpp
int status;
int ret = waitpid(script_pid, &status, 0);

if (ret == -1) {
    // waitpid falhou
    return send_error(500, "waitpid failed");
}

if (WIFEXITED(status)) {
    int exit_code = WEXITSTATUS(status);
    
    if (exit_code != 0) {
        // Script retornou erro
        return send_error(500, "Script error: " + to_string(exit_code));
    }
    // OK, continue com resposta
}

if (WIFSIGNALED(status)) {
    // Script foi terminado por signal (crash, segfault, etc)
    int signal = WTERMSIG(status);
    return send_error(500, "Script killed by signal: " + to_string(signal));
}
```
