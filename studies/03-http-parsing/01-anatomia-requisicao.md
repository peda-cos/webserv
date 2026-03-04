# Anatomia de uma Requisição HTTP — Notas de Estudo

## O que é uma Requisição HTTP
- Um bloco de texto puro (ASCII) enviado pelo cliente (browser/nc) ao servidor.
- Chega pelo socket como bytes brutos — o servidor que interpreta o significado.
- Estrutura **obrigatoriamente** dividida em 3 partes separadas por marcadores exatos.

---

## Estrutura Completa de uma Requisição HTTP/1.1

```
GET /index.html HTTP/1.1\r\n          ← Linha de Requisição (Request Line)
Host: localhost:8080\r\n              ← Header 1
Accept: text/html\r\n                 ← Header 2
Connection: keep-alive\r\n            ← Header 3
\r\n                                  ← Linha vazia = fim dos headers (boundary!)
```

Para requisição com body (POST, PUT):
```
POST /upload HTTP/1.1\r\n
Host: localhost:8080\r\n
Content-Type: application/x-www-form-urlencoded\r\n
Content-Length: 27\r\n
\r\n                                  ← fim dos headers
nome=Jonnathan&idade=20              ← Body (tamanho = Content-Length)
```

---

## Parte 1 — Request Line (Linha de Requisição)

Sempre a primeira linha. Formato obrigatório:
```
MÉTODO SP caminho SP versão\r\n
```

| Campo | Exemplo | O que significa |
|---|---|---|
| Método | `GET`, `POST`, `DELETE` | Ação a executar |
| Caminho | `/index.html`, `/api/user` | Recurso solicitado |
| Versão | `HTTP/1.1` | Protocolo usado |

### Métodos que o Webserv deve implementar
| Método | Uso | Body? |
|---|---|---|
| `GET` | Buscar um recurso | ❌ Não |
| `POST` | Enviar dados / criar recurso | ✅ Sim |
| `DELETE` | Deletar um recurso | ❌ Geralmente não |

---

## Parte 2 — Headers

- Um header por linha, formato: `Nome: Valor\r\n`
- O nome é case-insensitive (`Content-Length` = `content-length`)
- Headers terminam com uma linha vazia (`\r\n` sozinho)

### Headers críticos para o Webserv

| Header | Exemplo | Importância |
|---|---|---|
| `Host` | `Host: localhost:8080` | **Obrigatório** em HTTP/1.1 — identifica o virtual host |
| `Content-Length` | `Content-Length: 42` | Tamanho do body em bytes — necessário para saber quando o body termina |
| `Content-Type` | `Content-Type: text/html` | Tipo do dado enviado no body |
| `Connection` | `Connection: close` | Fechar conexão após resposta? |
| `Transfer-Encoding` | `Transfer-Encoding: chunked` | Body enviado em chunks sem Content-Length |

---

## Parte 3 — Body (opcional)

- Presente em `POST`, `PUT` e `PATCH`
- Ausente em `GET` e `DELETE`
- Tamanho definido pelo header `Content-Length`
- Lê exatamente `Content-Length` bytes após o `\r\n\r\n`

---

## O Marcador de Fim — `\r\n\r\n`

- `\r` = carriage return (ASCII 13)
- `\n` = line feed (ASCII 10)
- Cada header termina com `\r\n`
- A linha vazia entre headers e body é `\r\n` sozinho
- A sequência `\r\n\r\n` = fim do último header + linha vazia = **fim dos headers**
- Só após encontrar `\r\n\r\n` no buffer acumulado é que os headers podem ser parseados

---

## Fluxo de Parsing que o Webserv Precisa Seguir

```
1. Acumular bytes no recv_buffer até encontrar \r\n\r\n
2. Separar a Request Line — tudo antes do primeiro \r\n
3. Extrair o método (primeiro token)
4. Extrair o caminho (segundo token)
5. Extrair a versão (terceiro token)
6. Parsear cada header — linha por linha até a linha vazia
7. Se método for POST: verificar Content-Length e ler exatamente N bytes de body
8. Montar a resposta e colocar no send_buffer
```

---

## Regras / Restrições Críticas

- **NUNCA parsear requisição incompleta** — aguardar `\r\n\r\n` antes de interpretar.
- **SEMPRE validar** o método — retornar `405 Method Not Allowed` se não suportado.
- **Content-Length ausente em POST** → retornar `400 Bad Request`.
- **Caminho deve começar com `/`** — caminhos relativos são inválidos.
- **Headers são case-insensitive** — normalizar para lowercase ao comparar.

## Armadilhas Comuns (Não Esquecer)
- `\r\n` é diferente de `\n` — browsers enviam `\r\n`, mas clientes mal-formados podem enviar só `\n`.
- Um header com valor muito longo pode ser vetor de ataque (buffer overflow) — limitar o tamanho.
- O caminho pode conter query string: `/busca?q=hello` — separar o path do `?` em diante.
- Pipelining HTTP/1.1: dois requests podem "colar" no mesmo buffer após o `\r\n\r\n`.
