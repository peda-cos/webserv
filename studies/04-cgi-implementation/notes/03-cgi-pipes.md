# CGI — Pipes: Comunicação entre Processo Pai e Filho

## O Que é um Pipe?

**Pipe** é um **canal de comunicação unidirecional** entre dois processos.

### Analogia

```
Pai escreve em um END, Filho lê no outro END:

┌─────────────────────────────────────────┐
│ PAI (Servidor Web)                      │
│                                         │
│    write(pipe_fd[1], "dados")          │ ← escreve
└────────────────┬────────────────────────┘
                 │
                [PIPE] (buffer no Kernel)
                 │
┌────────────────┴────────────────────────┐
│ FILHO (Script CGI)                      │
│                                         │
│    read(pipe_fd[0], buffer)            │ ← lê
└─────────────────────────────────────────┘
```

### Estrutura em C

```cpp
int pipe_fd[2];

if (pipe(pipe_fd) == -1) {
    perror("pipe");
    return;
}

// pipe_fd[0] = read end  (ler)
// pipe_fd[1] = write end (escrever)
```

---

## Caso CGI: Duas Comunicações Necessárias

### 1️⃣ POST Body → Script (stdin)

```
Requisição HTTP com POST body:
├─ HTTP headers
└─ BODY (e.g., "username=admin&pass=123")

Script precisa ler esse body como stdin.

PAI:                                FILHO:
┌──────────┐                        ┌──────────┐
│ POST body│                        │ Script   │
└────┬─────┘                        └────┬─────┘
     │ write(fd[1], body)                │
     │─────[PIPE]────────────────────────> read(fd[0])
```

### 2️⃣ Script Output → Response (stdout)

```
Script escreve resultado:
├─ Resultado da lógica
└─ Response esperada

PAI precisa capturar como stdout.

FILHO:                              PAI:
┌──────────┐                        ┌──────────┐
│ Script   │                        │ Response │
└────┬─────┘                        └────┬─────┘
     │ write(stdout, resultado)         │
     │<─────[PIPE]───────────────────── read(fd[0])
```
---

## Diagrama Completo: Pipes + fork() + exec()

```
ANTES DO FORK:
┌── stdin_pipe[0|1] ──┐
│                     │
└─────────────────────┘
┌── stdout_pipe[0|1] ──┐
│                      │
└──────────────────────┘

            ↓↓↓ fork() ↓↓↓

PROCESSO PAI (PID X)          PROCESSO FILHO (PID Y)
┌──────────────────────────┐  ┌──────────────────────────┐
│ Fecha:                   │  │ Dup2:                    │
│ - stdin_pipe[0]          │  │ - stdin_pipe[0] → fd 0   │
│ - stdout_pipe[1]         │  │ - stdout_pipe[1] → fd 1  │
│                          │  │                          │
│ Escreve em:              │  │ Fecha todos os pipes     │
│ - stdin_pipe[1]          │  │                          │
│                          │  │ Executa:                 │
│ Lê em:                   │  │ exec("python", script)   │
│ - stdout_pipe[0]         │  │                          │
│                          │  │ Script agora usa:        │
│ Aguarda:                 │  │ - cin (ou stdin)         │
│ - waitpid(Y, ...)        │  │ - cout (ou stdout)       │
└──────────────────────────┘  └──────────────────────────┘

POST Body Flow:
PAI escrevePOST em stdin_pipe[1] ──> FILHO lê de stdin_pipe[0]

Response Flow:
FILHO escreve em stdout_pipe[1] ──> PAI lê de stdout_pipe[0]
```

---

## Tratamento de Erros Críticos

### ❌ Erro: Nunca Fechar Write End

```cpp
// ERRADO:
while (true) {
    bytes = read(pipe, buf, 1024);  // Vai esperar eternamente!
    // Por quê? O write end ainda está aberto no pai,
    // então o kernel acha que mais dados podem chegar.
}

// CORRETO:
close(write_fd);  // Fechar write end
while (true) {
    bytes = read(pipe, buf, 1024);  // Agora EOF após dados
}
```

### ❌ Erro: Deadlock por Buffer Cheio

```
Situação:
- Script escreve muito output (>4KB buffer padrão)  
- Buffer de stdout_pipe fica cheio
- Script para de escrever (bloqueado no write)
- Pai tenta escrever em stdin_pipe (que também está cheio)
- Pai bloqueia, filho bloqueia
- DEADLOCK

Solução: Use pipes grandes ou non-blocking I/O
```

### ❅ Erro: Não Aguardar Filho

```cpp
// ERRADO:
pid = fork();
if (pid == 0) {
    exec(script);
}
// Programa continua sem waitpid()!
// Script vira "zombie process"

// CORRETO:
pid = fork();
if (pid == 0) {
    exec(script);
}
int status;
waitpid(pid, &status, 0);  // Sempre aguardar
```

---

## Resumo: Pipes para CGI

```
1. Criar dois pipes ANTES do fork
   ├─ stdin_pipe[2]  — dados DO servidor PARA script
   └─ stdout_pipe[2] — dados DO script PARA servidor

2. No fork() — Processo Filho:
   ├─ dup2(stdin_pipe[0], 0)     — conectar stdin
   ├─ dup2(stdout_pipe[1], 1)    — conectar stdout
   ├─ close(todos os fd's)       — limpeza
   └─ exec(script)               — iniciar script

3. No fork() — Processo Pai:
   ├─ close(stdin_pipe[0])       — não usa read end
   ├─ close(stdout_pipe[1])      — não usa write end
   ├─ write(stdin_pipe[1], body) — enviar POST body
   ├─ close(stdin_pipe[1])       — sinalizar EOF
   ├─ read(stdout_pipe[0], ...)  — capturar output
   ├─ close(stdout_pipe[0])      — fechar leitura
   └─ waitpid(pid)               — aguardar filho
```

---