# SIGPIPE e errno — Notas de Estudo

## SIGPIPE — O Sinal que Mata o Servidor em Silêncio

### O que é / Para que serve
- `SIGPIPE` é um sinal enviado pelo SO ao processo quando ele tenta fazer `send()` ou `write()` num socket cujo lado receptor já fechou a conexão.
- Por padrão, `SIGPIPE` **mata o processo imediatamente** sem nenhuma mensagem de erro.
- Viola diretamente a regra do subject: *"seu programa não deve travar sob nenhuma circunstância"*.

### Como Funciona (Mecanismo)
1. Cliente A conecta ao servidor.
2. Servidor começa um `send()` grande para o Cliente A.
3. Cliente A fecha conexão abruptamente (queda de Wi-Fi, Ctrl+C, crash).
4. SO detecta que o socket do Cliente A está morto.
5. SO envia `SIGPIPE` ao servidor → **servidor morre**.

### Como Evitar (Duas Abordagens)
```cpp
// Abordagem 1: ignorar SIGPIPE globalmente (uma linha no início do main)
signal(SIGPIPE, SIG_IGN);
// send() retornará -1 em vez de matar o processo

// Abordagem 2: flag por chamada de send()
send(client_fd, buffer, len, MSG_NOSIGNAL);
// MSG_NOSIGNAL suprime o SIGPIPE apenas naquele send()
```

### Regra / Restrição Crítica
- **SEMPRE** colocar `signal(SIGPIPE, SIG_IGN)` no início do `main()` do Webserv.
- Sem isso, qualquer cliente que desconectar no momento errado derruba o servidor.

---

## errno — A Variável que o Subject Proíbe Verificar

### O que é / Para que serve
- `errno` é uma variável global do sistema (definida em `<cerrno>`) que armazena o código do **último erro de chamada de sistema**.
- Funções como `read()`, `write()`, `recv()`, `send()` atualizam `errno` quando retornam `-1`.

### Por que o Subject Proíbe Verificar errno Após read/write
- Em modo **não-bloqueante** (que o Webserv usa com `poll()`), `recv()` pode retornar `-1` com `errno == EAGAIN` ou `errno == EWOULDBLOCK` — isso **não é um erro**: significa "não há dados agora, tente depois".
- Verificar `errno` leva a comportamentos errados: tratar `EAGAIN` como falha fatal fecha conexões válidas.
- A forma correta é **usar o valor de retorno** da função para decidir o que fazer.

### Comportamento Correto vs. Errado
```
❌ ERRADO (proibido pelo subject):
   bytes = recv(fd, buf, len, 0);
   if (errno == EAGAIN) { /* trata como erro */ }

✅ CORRETO:
   bytes = recv(fd, buf, len, 0);
   if (bytes == 0)  { /* cliente desconectou — fecha fd */ }
   if (bytes < 0)   { /* erro real — fecha fd */ }
   if (bytes > 0)   { /* dados chegaram — processa */ }
```

### Armadilhas Comuns (Não Esquecer)
- `errno` é **compartilhado** entre todas as chamadas de sistema — outra função pode sobrescrever antes de você checar.
- `EINTR` (interrompido por sinal) não é erro fatal: basta chamar `recv()` novamente.
- `EAGAIN` / `EWOULDBLOCK` não são erros em sockets não-bloqueantes: são sinais de "nada disponível agora".

## Próximo Passo de Aprendizado
- Estudar: `poll()` — como monitorar múltiplos fds sem bloquear e sem verificar errno
