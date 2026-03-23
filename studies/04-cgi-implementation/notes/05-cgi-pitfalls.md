# CGI — Armadilhas Comuns e Best Practices

## As Armadilhas Mortais do CGI

### 1️⃣ Nunca Fechar Write End do Pipe

**Problema:**
```cpp
// ERRADO — Script bloqueia no write, servidor está em read()
// É um deadlock esperando acontecer
while (bytes_read = read(pipe, buffer, 4096)) {
    if (bytes_read > 0) {
        response += std::string(buffer, bytes_read);
    }
}
// read() vai bloquear porque write end ainda está aberto!
```

**Solução:**
```cpp
// CORRETO — Fechar write end imediatamente após escrever
close(stdout_pipe[1]);

// Agora read() vai receber EOF após output terminar
while ((bytes_read = read(stdout_pipe[0], buffer, 4096)) > 0) {
    response += std::string(buffer, bytes_read);
}
```

---

### 2️⃣ Esquecer de Fechar File Descriptors Não Usados

**Problema:**
```cpp
if (pid == 0) {  // Child
    dup2(stdin_pipe[0], STDIN_FILENO);
    dup2(stdout_pipe[1], STDOUT_FILENO);
    
    // ERRADO — stdin_pipe[1] e stdout_pipe[0] ainda estão abertos!
    // Isso impede que o kernel feche os pipes mesmo com fork
    
    exec(script);
}
```

**Solução:**
```cpp
if (pid == 0) {  // Child
    dup2(stdin_pipe[0], STDIN_FILENO);
    dup2(stdout_pipe[1], STDOUT_FILENO);
    
    // CORRETO — Fechar todos
    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    
    exec(script);
}
```

---

### 3️⃣ Não Verificar Retorno de pipe()

**Problema:**
```cpp
int pipe_fd[2];
pipe(pipe_fd);  // Se falhar, pipe_fd[0/1] são lixo

// Mais tarde, segfault ou comportamento indefinido
write(pipe_fd[1], data, size);
```

**Solução:**
```cpp
int pipe_fd[2];
if (pipe(pipe_fd) == -1) {
    perror("pipe");
    return;  // Falhar graciosamente
}
```

---

### 4️⃣ Não Verificar Retorno de fork()

**Problema:**
```cpp
pid_t pid = fork();

if (pid == 0) {
    // Child
}
// else ASSUMIR que é pai
```

**Solução:**
```cpp
pid_t pid = fork();

if (pid == -1) {
    perror("fork");
    return;
}

if (pid == 0) {
    // Child
} else {
    // Parent (pid > 0)
}
```

---

### 5️⃣ Não Verificar Retorno de exec()

**Problema:**
```cpp
if (pid == 0) {
    setenv(...);
    execvp("python", args);
    // Se exec falhar, continua executando o código PAI dentro do FILHO!
}
```

**Solução:**
```cpp
if (pid == 0) {
    setenv(...);
    execvp("python", args);
    
    // Se chegou aqui, exec FALHOU
    perror("execvp");  // Sempre fazer algo
    exit(127);         // Sair imediatamente
}
```

---

### 6️⃣ Usar Caminhos Relativos para Scripts

**Problema:**
```cpp
// Script no diretório atual durante execução
execvp("script.py", args);  // De qual diretório?

// Se o servidor muda dir com chdir(), fica perdido
```

**Solução:**
```cpp
// Sempre caminho absoluto
const char* script_path = "/var/www/cgi-bin/script.py";
const char* args[] = { "python", script_path, NULL };
execvp("python", (char* const*)args);
```

---

### 7️⃣ Deadlock por Buffer Cheio do Pipe

**Problema:**
```
Script escreve >64KB de saída (pipe buffer é ~64KB):
├─ Buffer do pipe fica cheio
├─ Script bloqueia no write()
└─ Servidor aguarda no read()
   └─ Servidor não lê mais porque está escrevendo no stdin_pipe[1]!
      └─ DEADLOCK

Situação:
  Pai: write() em stdin_pipe[1] bloqueado (buffer cheio)
  Filho: write() em stdout_pipe[1] bloqueado (buffer cheio)
  Ninguém lê, ninguém escreve.
```

**Solução para MVP:**
- Limite `client_max_body_size` adequadamente
- Use non-blocking I/O (fcntl com O_NONBLOCK)
- Use `select()` ou `poll()` para ler/escrever alternadamente

**Simple approach:**
```cpp
// Fechar write end imediatamente após escrever
close(stdin_pipe[1]);

// Agora qualquer script pode escrever o quanto quiser
// Porque o pai está em read() esperando
```

---

### 8️⃣ Não Usar WIFEXITED() / WEXITSTATUS()

**Problema:**
```cpp
int status;
waitpid(pid, &status, 0);

int exit_code = status;  // ERRADO! status tem bits de sinal também!
// exit_code pode ser 256 (0x100) quando código foi 1
```

**Solução:**
```cpp
int status;
waitpid(pid, &status, 0);

if (WIFEXITED(status)) {
    int exit_code = WEXITSTATUS(status);  // Macros extraem corretamente
}

if (WIFSIGNALED(status)) {
    int signal = WTERMSIG(status);
    // Script foi killed por signal (segfault, SIGKILL, etc)
}
```

---

### 9️⃣ Não Validar Existência/Permissão do Script

**Problema:**
```cpp
// Script não existe
execvp("nonexistent.py", args);  // Falha silenciosamente?
```

**Solução:**
```cpp
#include <sys/stat.h>
#include <unistd.h>

bool is_executable(const std::string& path) {
    struct stat st;
    
    if (stat(path.c_str(), &st) == -1) {
        return false;  // Arquivo não existe
    }
    
    if (!(st.st_mode & S_IEXEC)) {
        return false;  // Não é executável
    }
    
    return true;
}

// Verificar ANTES de fork
if (!is_executable(script_path)) {
    return send_error(404, "Script not found or not executable");
}
```

---

### 🔟 Usar setenv() com C++98

**Problema - Compilação:**
```cpp
// setenv() é POSIX, não C++98 puro
// Compilador pode reclamar em ambiente strict
setenv("VAR", "value", 1);
```

**Solução:**
```cpp
// Incluir header certo
#define _POSIX_C_SOURCE 200112L  // Antes de unistd.h
#include <unistd.h>
#include <cstdlib>

// Agora setenv() está disponível em todos os SOs POSIX
setenv("VAR", "value", 1);
```

---

## Best Practices

### 1. Sempre Limpar Pipes em Pai e Filho

```cpp
// Pai e Filho precisam de disciplina

// Antes de exec() — Filho
close(stdin_pipe[0]);
close(stdin_pipe[1]);
close(stdout_pipe[0]);
close(stdout_pipe[1]);

// Depois de fork() — Pai
close(stdin_pipe[0]);
close(stdout_pipe[1]);
```

### 2. Criar Pipes Antes do fork()

```cpp
// ❌ ERRADO — Criar pipes dentro do filho
// Não há pipe para comunicação!

// ✅ CORRETO
int pipe1[2], pipe2[2];
if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
    perror("pipe");
    return;
}
pid = fork();
```

### 3. Post-Body via stdin, Query via Variável

```cpp
// GET /script.py?q=nginx
// ├─ QUERY_STRING variável: setenv("QUERY_STRING", "q=nginx", 1)
// └─ Nenhum dado em stdin

// POST /script.py
// ├─ Nenhuma QUERY_STRING
// └─ Body em stdin (pipes)
```

### 4. Sempre Usar Caminhos Absolutos

```cpp
// Script
std::string script_abs = "/var/www/cgi-bin/calc.py";

// Intérprete
// use which() ou hardcode common paths
std::string python_path = "/usr/bin/python";
```

### 5. Timeout para Scripts Lentos (Avançado)

```cpp
// Usar alarm() ou SIGALRM para timeout
// Ou usar poll() com timeout antes de waitpid()

// Simples: waitpid() com WNOHANG (non-blocking)
int ret = waitpid(pid, &status, WNOHANG);
if (ret == 0) {
    // Script ainda rodando
    // Pode implementar timeout aqui
}
```

### 6. Setar Variáveis de Ambiente em Ordem

```cpp
// Seguir ordem sensata:
// 1. Variáveis do servidor
setenv("SERVER_NAME", "localhost", 1);
setenv("SERVER_PORT", "8080", 1);

// 2. Variáveis da requisição
setenv("REQUEST_METHOD", "GET", 1);
setenv("SCRIPT_NAME", "/cgi-bin/calc.py", 1);

// 3. Variáveis opcionais
setenv("HTTP_USER_AGENT", "Mozilla/5.0", 1);
```

### 7. Documentar Qual FD é Qual

```cpp
// Deixar claro:
int stdin_pipe[2];   // Child reads from [0], parent writes to [1]
int stdout_pipe[2];  // Child writes to [1], parent reads from [0]

// Ou comentar explicitamente:
// pipe_fd[stdin][READ_END] = 0
// pipe_fd[stdin][WRITE_END] = 1
```

---

## Resumo

```
✅ DO's:

1. pipe() ANTES do fork()
2. close() não-usados em pai E filho
3. Validar TODOS os retornos (pipe, fork, exec, waitpid)
4. dup2() conecta streams corretamente
5. setenv() todas as variáveis obrigatórias
6. Usar caminhos absolutos
7. WIFEXITED() + WEXITSTATUS() corretamente
8. Fechar write end do pipe após escrever

❌ DON'Ts:

1. Deixar write end aberto (deadlock)
2. Esquecer de fechar FDs
3. Usar exec() sem verificar retorno
4. Não fazer waitpid() (zombies)
5. Usar caminhos relativos
6. Acessar variáveis não setadas
7. Ignorar permissões de arquivo
8. Misturar stdin/stdout sem pipes
```
