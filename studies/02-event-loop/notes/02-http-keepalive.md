# HTTP Keep-Alive e Gerenciamento de Conexões — Notas de Estudo

## Versões do HTTP e Comportamento Padrão de Conexão

| Versão | Padrão | Como mudar |
|---|---|---|
| HTTP/1.0 | Fecha após cada resposta | Header `Connection: keep-alive` para manter |
| HTTP/1.1 | **Mantém aberta** (keep-alive) | Header `Connection: close` para fechar |
| HTTP/2+ | Mantém aberta + múltiplas requisições paralelas no mesmo socket | — |

## HTTP/1.1 Keep-Alive — Como Funciona na Prática
- Uma única conexão TCP serve **múltiplas requisições sequenciais** do browser.
- O browser envia `GET /index.html`, recebe a resposta, e **reutiliza o mesmo socket** para `GET /style.css`, `GET /logo.png`, etc.
- A conexão só é fechada quando:
  - O browser envia `Connection: close` na requisição.
  - O servidor responde com `Connection: close` e fecha após a resposta.
  - O `keepalive_timeout` do servidor expira (inatividade).

## O que Acontece quando o Servidor Fecha por Timeout
1. Servidor faz `close(client_fd)` → SO envia TCP `FIN` para o browser.
2. Browser recebe o `FIN` → marca a conexão como encerrada internamente.
3. Na próxima requisição do usuário, o browser percebe a conexão morta.
4. Browser abre nova conexão TCP automaticamente — **invisível ao usuário**.

## O Servidor Pode Avisar Antes de Fechar (Header `Connection: close`)
Enviar `Connection: close` na resposta avisa o browser com antecedência:
```
HTTP/1.1 200 OK
Connection: close
Content-Length: 42

...body...
```
O browser não tentará reutilizar aquela conexão. Mais elegante, mas não obrigatório — o browser reconecta de qualquer forma se a conexão estiver morta.

## Implicação para o Webserv
- Após enviar uma resposta, **verificar o header `Connection`** da requisição recebida.
- Se `Connection: close` → fechar o fd após enviar a resposta.
- Se `Connection: keep-alive` (ou HTTP/1.1 default) → manter o fd aberto no array do poll() e aguardar nova requisição.
- O buffer do cliente deve ser **resetado** após processar cada requisição completa — pode chegar outra logo em seguida no mesmo socket.

## Regras / Restrições Críticas
- **NUNCA fechar o fd** após cada resposta em HTTP/1.1 sem verificar o header `Connection`.
- **SEMPRE resetar o buffer** do cliente após processar uma requisição completa — outra pode vir em seguida.
- O suject do Webserv referencia HTTP/1.0 como ponto de partida (fechar sempre é válido), mas browsers modernos usam HTTP/1.1.

## Armadilhas Comuns (Não Esquecer)
- Em HTTP/1.1 podem chegar duas requisições "coladas" no mesmo buffer (pipelining) — processar uma de cada vez.
- O `Connection: keep-alive` do cliente é um pedido, não uma ordem — o servidor pode ignorar e fechar mesmo assim.
- Browsers mantêm pool de até 6 conexões por domínio — múltiplos fds do mesmo IP são normais.

