# CGI — Variáveis de Ambiente

## O Que São Variáveis de Ambiente CGI?

**Variáveis de ambiente** são strings `"NAME=VALUE"` que o servidor define **antes de executar** o script. O script as lê via `getenv()` ou `environ[]`.

### Analogia

```
Você está saindo de casa (servidor pai).
Você deixa uma nota na geladeira (variáveis de ambiente):

"Seu almoço está no freezer"
"Passei café na cozinha"
"Sua mãe ligou, deixei número 1234"

Seu filho (processo filho/script) chega em casa:
Lê as notas:
  - almoço = freezer
  - café = pronto
  - mãe = 1234

Seu filho age baseado nessas informações, sem precisar
que você esteja lá explicando.
```

---

## Variáveis CGI Padrão (RFC 3875)

The CGI standard defines these variables:

| Variável | Propósito | Exemplo |
|----------|-----------|---------|
| `REQUEST_METHOD` | Método HTTP | `"GET"`, `"POST"`, `"DELETE"` |
| `SCRIPT_NAME` | Caminho do script no URL | `"/cgi-bin/calc.py"` |
| `QUERY_STRING` | Dados GET (sem decodificação) | `"name=joao&age=25"` |
| `CONTENT_LENGTH` | Bytes de POST body | `"256"` |
| `CONTENT_TYPE` | Tipo MIME do POST | `"application/x-www-form-urlencoded"` |
| `HTTP_HOST` | Hostname do servidor | `"example.com:8080"` |
| `HTTP_USER_AGENT` | Browser user agent | `"Mozilla/5.0 ..."` |
| `REMOTE_ADDR` | IP do cliente | `"192.168.1.100"` |
| `SERVER_NAME` | Nome do servidor | `"example.com"` |
| `SERVER_PORT` | Porta do servidor | `"8080"` |
| `SERVER_PROTOCOL` | Versão HTTP | `"HTTP/1.1"` |
| `HTTPS` | Se conexão é segura | `"on"` ou vazio |

---

## Mapeamento: HTTP Request → CGI Variables

### Exemplo: GET Request

```
┌─────────────────────────────────────────────────┐
│ REQUISIÇÃO HTTP QUE CHEGA DO CLIENTE:           │
├─────────────────────────────────────────────────┤
│ GET /cgi-bin/search.py?q=nginx&lang=en HTTP/1.1│
│ Host: example.com:8080                          │
│ User-Agent: Mozilla/5.0                         │
│ Remote IP: 192.168.1.50                         │
└─────────────────────────────────────────────────┘

           ↓↓↓ SERVIDOR EXTRAI ↓↓↓

REQUEST_METHOD     = "GET"
SCRIPT_NAME        = "/cgi-bin/search.py"
QUERY_STRING       = "q=nginx&lang=en"
HTTP_HOST          = "example.com:8080"
HTTP_USER_AGENT    = "Mozilla/5.0"
REMOTE_ADDR        = "192.168.1.50"
SERVER_NAME        = "example.com"
SERVER_PORT        = "8080"
SERVER_PROTOCOL    = "HTTP/1.1"
CONTENT_LENGTH     = "" (GET não tem body)
CONTENT_TYPE       = "" (GET não tem corpo)

           ↓↓↓ SERVIDOR EXECUTA: ↓↓↓

./search.py (com todas as variáveis setadas acima)
```

### Exemplo: POST Request

```
┌──────────────────────────────────────┐
│ REQUISIÇÃO HTTP QUE CHEGA:           │
├──────────────────────────────────────┤
│ POST /cgi-bin/login.php HTTP/1.1     │
│ Host: example.com                    │
│ Content-Type: applic...form-data     │
│ Content-Length: 30                   │
│                                      │
│ username=admin&password=secret       │ ← BODY (30 bytes)
└──────────────────────────────────────┘

           ↓↓↓ SERVIDOR EXTRAI ↓↓↓

REQUEST_METHOD     = "POST"
SCRIPT_NAME        = "/cgi-bin/login.php"
QUERY_STRING       = "" (POST usa body, não URL)
CONTENT_LENGTH     = "30"
CONTENT_TYPE       = "application/x-www-form-urlencoded"
HTTP_HOST          = "example.com"
...

           ↓↓↓ SERVIDOR EXECUTA: ↓↓↓

STDIN redireciona para: username=admin&password=secret
./login.php (lê stdin para ler POST data)
```

---

## Como Setando Variáveis de Ambiente em C++

### Opção 1: Usando `setenv()` (Moderno, Mais Claro)

```cpp
setenv("REQUEST_METHOD", "GET", 1);
setenv("SCRIPT_NAME", "/script.py", 1);
setenv("QUERY_STRING", "name=joao&age=25", 1);
```

**Problema para C++98**: `setenv()` é POSIX, não C++98 padrão.
**Solução**: Usar com `#define _POSIX_C_SOURCE` (funciona na prática).

> POSIX significa que é amplamente suportado em sistemas Unix-like, incluindo Linux e macOS.

### Opção 2: Usando `environ[]` Global (C Clássico, Mais Compatível)

```cpp
// O sistema coloca todas as variáveis aqui como strings
extern char** environ;

// Para adicionar nova variável:
// (1) Alocar string no formato "NAME=VALUE"
char* var = strdup("REQUEST_METHOD=GET");

// (2) Inserir no environmental (precisa gerenciar array)
// Isso é complexo porque environ é array terminado por NULL
```

**Problema**: Arrays de `char*` em C++ são trabalhosos.
**Solução Prática**: Use `setenv()` + `#define _POSIX_C_SOURCE 200112L`

---

## Opção 3: Usando `envp` Customizado (Abordagem Webserver Profissional)

Esta é a abordagem mais robusta para servidores que lidam com múltiplas requisições simultâneas, pois evita a poluição do ambiente do processo pai.

### Por que usar `envp` em vez de `setenv()`?

1. **Isolamento Total**: `setenv()` modifica o ambiente do seu servidor (processo pai). Se duas requisições CGI rodarem ao mesmo tempo, uma pode sobrescrever os dados da outra.
2. **Segurança**: O script filho recebe **apenas** as variáveis que você explicitamente definiu no array, ignorando variáveis sensíveis do seu sistema (como `PATH` ou `USER` do servidor).
3. **Gerenciamento de Memória**: Você controla exatamente quando as strings do ambiente são criadas e destruídas via `char**`.

### Fluxo com `envp`:
1. Monte um `std::map<string, string>` com as variáveis.
2. Converta o mapa para um `char** envp` (array de ponteiros terminado em `NULL`).
3. Passe este array como terceiro argumento do `execve()`.

---

## Padrão de Uso em Código C++98

```cpp
#include <cstdlib>      // setenv, getenv
#include <unistd.h>     // fork, execvp
#include <sys/wait.h>   // waitpid
#include <cstring>

void execute_cgi(
    const std::string& script_path,
    const std::string& method,
    const std::string& query_string,
    size_t content_length
) {
    pid_t pid = fork();
    
    if (pid == 0) {  // Processo filho
        // SETUP: Definir todas as variáveis antes de exec
        setenv("REQUEST_METHOD", method.c_str(), 1);
        setenv("SCRIPT_NAME", script_path.c_str(), 1);
        setenv("QUERY_STRING", query_string.c_str(), 1);
        
        // Se POST, também passar content length
        if (method == "POST") {
            char buf[256];
            snprintf(buf, sizeof(buf), "%zu", content_length);
            setenv("CONTENT_LENGTH", buf, 1);
            setenv("CONTENT_TYPE", 
                   "application/x-www-form-urlencoded", 1);
        }
        
        // EXEC: Iniciar o script
        // Exemplo: executar como script Python
        const char* args[] = { "python", script_path.c_str(), NULL };
        execvp("python", (char* const*)args);
        
        // SE exec FALHAR:
        perror("execvp");
        exit(1);
    } 
    else if (pid > 0) {  // Processo pai
        // Aguardar filho terminar
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            // Sucesso: exit code = 0
        }
    } 
    else {  // fork falhou
        perror("fork");
    }
}
```

---

## Query String vs POST Body

### GET (Query String)

```
URL: /search.py?q=nginx&limit=10

→ REQUEST_METHOD = "GET"
→ QUERY_STRING = "q=nginx&limit=10"
→ Script lê: getenv("QUERY_STRING")
→ Decodifica: q → "nginx", limit → "10"
```

**Vantagem**: Rápido ler do ambiente.
**Desvantagem**: Limite de tamanho (geralmente ~4KB).

### POST (Body)

```
POST /login.py HTTP/1.1
Content-Length: 30
Content-Type: application/x-www-form-urlencoded

username=admin&password=secret

→ REQUEST_METHOD = "POST"
→ CONTENT_LENGTH = "30"
→ CONTENT_TYPE = "application/x-www-form-urlencoded"
→ Script lê: stdin (os 30 bytes)
→ Decodifica os bytes: username → "admin", password → "secret"
```

**Vantagem**: Sem limite de tamanho (até `client_max_body_size`).
**Desvantagem**: Script precisa ler stdin corretamente.
