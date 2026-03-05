# poll() — O Coração do Servidor Não-Bloqueante

## O que é / Para que serve
- `poll()` é uma syscall do SO que monitora **múltiplos file descriptors simultaneamente**.
- Fica em espera ("dorme") até que **ao menos um fd** esteja pronto para leitura ou escrita.
- Substitui o problema do `accept()` e `recv()` bloqueantes: em vez de travar num único fd,
  o servidor delega ao SO a tarefa de vigiar todos ao mesmo tempo.
- É o mecanismo central que permite um servidor de **thread única atender N clientes** sem travar.

## Por que o subject exige poll() (ou equivalente)
- O subject proíbe `fork()` para clientes — logo, sem threads ou processos extras.
- O único jeito de atender múltiplos clientes numa única thread é a **multiplexação de I/O**.
- `poll()`, `select()`, `epoll_wait()` e `kqueue()` são as formas padronizadas de fazer isso.
- O subject permite qualquer um deles — escolha `poll()` (Linux/Mac) ou `epoll()` (só Linux).

## Como Funciona (Mecanismo)

### A Estrutura `pollfd`
```c
struct pollfd {
    int   fd;       // o file descriptor a monitorar
    short events;   // quais eventos queremos vigiar (POLLIN, POLLOUT)
    short revents;  // quais eventos realmente ocorreram (preenchido pelo SO)
};
```

### O fluxo completo em pseudocódigo
```
fds[] = array de pollfd com todos os fds ativos (server_fd + todos os client_fds)

loop eterno:
    poll(fds, quantidade, timeout)    ← SO adormece aqui até haver evento

    para cada fd em fds:
        se fd == server_fd e POLLIN:
            novo_cliente = accept()   ← nova conexão chegou
            adiciona novo_cliente ao array fds[]

        se fd == client_fd e POLLIN:
            bytes = recv(fd, ...)     ← dados chegaram desse cliente
            acumula no buffer do cliente

        se fd == client_fd e POLLOUT:
            send(fd, ...)             ← socket pronto para envio
```

### Os eventos mais usados
| Flag | Significado |
|---|---|
| `POLLIN` | fd tem dados prontos para leitura |
| `POLLOUT` | fd está pronto para receber dados (escrita) |
| `POLLERR` | ocorreu um erro no fd |
| `POLLHUP` | cliente desconectou (hang-up) |

## O que muda em relação ao eco bloqueante
| Comportamento | Eco bloqueante | Com poll() |
|---|---|---|
| Cliente A lento | Trava o servidor inteiro | poll() ignora A, atende B e C |
| Múltiplos clientes | Sequencial (um por vez) | Simultâneo no mesmo loop |
| CPU em espera | 0% (bloqueado no recv) | 0% (bloqueado no poll) |
| fd único monitorado | server_fd ou client_fd | todos os fds de uma vez |

## Regras / Restrições Críticas
- **NUNCA fazer `recv()` ou `send()` sem passar por `poll()` antes** — regra explícita do subject.
- **Monitorar leitura E escrita simultaneamente** — ativar `POLLIN` e `POLLOUT` conforme necessário.
- **Apenas 1 poll()** para todo o servidor — não chamar poll() separado para cliente e servidor.
- **`poll()` retorna 0** se o timeout expirou sem eventos — não é erro.
- **`poll()` retorna -1** se foi interrompido por sinal (`EINTR`) — não é erro fatal, chamar novamente.

## Armadilhas Comuns (Não Esquecer)
- Remover do array `fds[]` os clientes que desconectam — fd fechado mas ainda no array causa POLLNVAL.
- Não confundir `events` (o que foi solicitado) com `revents` (o que o SO reportou) — são campos diferentes.
- `POLLOUT` ativo o tempo todo desperdiça CPU — ativar apenas quando há dados no buffer de saída aguardando envio.
- O array `fds[]` precisa ser redimensionado quando novos clientes entram.
