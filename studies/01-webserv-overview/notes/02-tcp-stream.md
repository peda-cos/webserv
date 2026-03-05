# TCP é um Stream, Não um Protocolo de Mensagens — Notas de Estudo

## O que é / Para que serve
- TCP garante entrega **ordenada e sem perdas** de bytes — mas não garante que chegam no mesmo "pedaço" que foram enviados.
- Um único `send()` pode ser fragmentado pelo SO/rede e chegar em **múltiplos `recv()`** no lado receptor.
- Um único `recv()` pode conter **dados de dois `send()`** diferentes colados juntos.

## Como Funciona (Mecanismo)
- TCP opera num **fluxo contínuo de bytes** (stream), sem fronteiras de mensagem.
- O SO decide quando encapsular os bytes em pacotes (segmentos TCP) baseado em MTU, latência e algoritmo de Nagle.
- O `recv()` lê o que **já chegou no buffer do SO** naquele instante — pode ser parte, tudo ou mais do que o esperado.

## Implicação Direta para o Webserv
- Nunca assumir `1 recv() = 1 requisição HTTP completa`.
- Acumular os bytes recebidos num **buffer próprio por cliente** e só processar quando tiver dados suficientes.
- O marcador de "mensagem completa" no HTTP é a sequência `\r\n\r\n` (fim dos headers).
  - Para o body com upload, o `Content-Length` indica quantos bytes ainda faltam ler.

## Exemplo Real do Problema
```
Cliente envia de uma vez:
  "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"

Servidor pode receber em 3 recv() separados:
  recv #1 → "GET /index.ht"
  recv #2 → "ml HTTP/1.1\r\nHo"
  recv #3 → "st: localhost\r\n\r\n"
```
O servidor **precisa juntar** os três pedaços antes de interpretar a requisição.

## Regras / Restrições Críticas
- **NUNCA parsear** o buffer parcial como se fosse uma requisição completa.
- **SEMPRE acumular** os bytes de `recv()` numa `std::string` ou buffer do cliente antes de processar.
- **Só processar** quando o marcador de fim for detectado no buffer acumulado.

## Armadilhas Comuns (Não Esquecer)
- Assumir que um `send()` de 200 bytes gera exatamente um `recv()` de 200 bytes no outro lado — **isso está errado**.
- Zerar o buffer entre chamadas de `recv()` sem copiar o que já havia acumulado — **perde dados parciais**.
- Interpretar `recv()` retornando menos bytes que o tamanho do buffer como erro — **é comportamento normal**.
