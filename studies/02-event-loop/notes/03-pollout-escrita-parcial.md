# POLLOUT e Escrita Parcial — Notas de Estudo

## O Problema — send() Parcial

- `send()` **não garante** enviar todos os bytes solicitados numa única chamada.
- Em modo não-bloqueante (`O_NONBLOCK`), `send()` retorna imediatamente com o que couber no buffer TCP de saída do kernel.
- O buffer TCP de saída por conexão tem tamanho limitado (~128KB–256KB).
- Se o cliente consome dados devagar (internet lenta), o buffer enche e `send()` retorna menos que o pedido.

```
Servidor chama: send(fd, dados, 500000, 0)
Buffer do kernel para fd: tem apenas 28KB livres
send() retorna: 28000   ← enviou só 28KB, não 500KB
→ 472.000 bytes ainda não foram enviados
```

## Por que isso acontece mais com respostas grandes
- Arquivos de imagem, CSS, HTML grandes, downloads
- Clientes com conexão lenta (3G, Wi-Fi ruim) — buffer enche porque o cliente não consome rápido
- Muitas transferências simultâneas — o kernel divide a banda entre todos os sockets

## A Solução — Buffer de Saída por Cliente + POLLOUT

### Estrutura por cliente
Cada cliente precisa de um **buffer de saída** interno no servidor:
```
cliente[fd=5]:
  recv_buffer = "dados acumulados da requisição"   ← lado de entrada
  send_buffer = "dados ainda não enviados ao cliente"  ← lado de saída (novo!)
```

### O Fluxo Completo
```
1. Servidor prepara resposta: 500KB de HTML

2. Tenta: send(fd=5, resposta, 500000, 0) → retorna 28000
   Guarda os 472KB restantes no send_buffer[fd=5]
   Ativa POLLOUT para fd=5 no array do poll()

3. poll() retorna → fd=5 tem POLLOUT (buffer TCP tem espaço)
   Tenta: send(fd=5, send_buffer, 472000, 0) → retorna 100000
   send_buffer[fd=5] agora tem 372KB

4. Repete até send_buffer[fd=5] esvaziar
   Quando vazio: DESATIVA POLLOUT para fd=5
```

### Regra de Ouro do POLLOUT
- **POLLIN:** quase sempre ativo para todos os clientes (sempre interessado em ler)
- **POLLOUT:** ativar **SOMENTE** quando há dados no send_buffer aguardando envio
- Manter POLLOUT sempre ativo desperdiça CPU: poll() retorna imediatamente toda hora mesmo sem nada para enviar

## Implicação para o Webserv
- Cada objeto/struct que representa um cliente precisa de 2 buffers:
  - `recv_buffer` (std::string) — acumula dados recebidos (POLLIN)
  - `send_buffer` (std::string) — acumula dados a enviar (POLLOUT)
- Após chamar `send()`, verificar o retorno e guardar o resto no `send_buffer`.
- Ativar `events |= POLLOUT` no `pollfd` daquele cliente se `send_buffer` não estiver vazio.
- Desativar `events &= ~POLLOUT` quando `send_buffer` esvaziar.

## Armadilhas Comuns (Não Esquecer)
- Nunca assumir que `send()` enviou tudo — sempre checar o valor de retorno.
- Não ativar POLLOUT desnecessariamente — causa busy-loop no event loop.
- O `send_buffer` pode acumular múltiplas respostas se o cliente usar keep-alive — processar em ordem.
- `send()` retornando -1 com socket não-bloqueante pode ser erro real ou `EAGAIN` — tratar pelo valor de retorno, não por errno.
