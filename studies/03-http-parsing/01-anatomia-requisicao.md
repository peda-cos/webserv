# Anatomia de uma Requisição HTTP — Notas de Estudo

## O que é uma Requisição HTTP
- É o bloco de texto puro (ASCII) que o cliente (browser/nc) envia pelo socket.
- Essa mensagem chega ao servidor como bytes brutos — cabe ao código interpretar o significado.
- A estrutura **obrigatoriamente** se divide em 3 partes separadas por marcadores exatos.

---

## Estrutura Completa de uma Requisição HTTP/1.1

```
GET /index.html HTTP/1.1\r\n          ← Linha de Requisição (Request Line)
Host: localhost:8080\r\n              ← Header 1
Accept: text/html\r\n                 ← Header 2
Connection: keep-alive\r\n            ← Header 3
\r\n                                  ← Linha vazia = fim dos headers (boundary!)
```

Para requisições com body (ex: POST):
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

| Campo | Exemplo | O que deve ser extraído |
|---|---|---|
| **Método** | `GET`, `POST`, `DELETE` | Ação a executar |
| **Caminho** | `/index.html`, `/api` | Qual arquivo/rota o cliente quer |
| **Versão** | `HTTP/1.1` | Protocolo usado (o Webserv só exige 1.1) |

### Métodos que o Webserv deve implementar
| Método | Uso | Recebe Body? |
|---|---|---|
| `GET` | Buscar um recurso | ❌ Não |
| `POST` | Enviar dados / fazer upload | ✅ Sim |
| `DELETE` | Apagar um recurso no servidor | ❌ Geralmente não |

---

## Parte 2 — Headers

- Um header por linha, formato: `Nome: Valor\r\n`
- Nomes são case-insensitive (`Content-Length` é igual a `content-length`)
- A sessão de headers termina com uma linha vazia (`\r\n` sozinho)

### Headers críticos que o servidor deve tratar

| Header | Importância no Webserv |
|---|---|
| `Host` | **Obrigatório** no HTTP/1.1. Usado para saber qual bloco `server {}` do `.conf` deve responder. Se faltar, retornar `400 Bad Request`. |
| `Content-Length` | Tamanho do body em bytes. Sem isso, nunca se sabe quando o body de um POST termina. |
| `Content-Type` | Indica o tipo de dado do body (texto, JSON, multipart-form). |
| `Connection` | Se for `close`, o `fd` deve ser fechado após o envio da resposta. |
| `Transfer-Encoding` | Se for `chunked`, o body chega em blocos fracionados (requer um parser especial para ler o tamanho hexadecimal de cada bloco). |

---

## Parte 3 — Body (Opcional)

- O body só é lido em métodos `POST` e `PUT` (ou `PATCH`).
- O tamanho exato do body a ser lido está no header `Content-Length`.
- Devem ser lidos **exatamente** `Content-Length` bytes a partir do caractere após o `\r\n\r\n`.

---

## O Marcador de Fim Crítico — `\r\n\r\n`

- O HTTP nasceu na época das máquinas de escrever: `\r` (carriage return = volta pro início da linha) e `\n` (line feed = pula pra linha de baixo).
- Cada header termina com `\r\n`.
- A separação entre headers e body é uma linha vazia (só um `\r\n`).
- Juntando o `\r\n` do último header com a linha vazia: **`\r\n\r\n`**.
- **Regra de ouro do parser:** Só é seguro começar a ler o conteúdo extraído quando o boundary `\r\n\r\n` for encontrado no `recv_buffer`.

---

## Fluxo Lógico de Parsing no Código

```text
1. Acumular os bytes que o recv() entrega no std::string recv_buffer.
2. Contém \r\n\r\n no buffer?
   - NÃO: Manter a string guardada e deixar o poll() voltar com mais bytes depois.
   - SIM: Prosseguir com o fatiamento da requisição.
3. Separar a Request Line (até o 1º \r\n).
4. Isolar os tokens por espaços: método, caminho, versão.
5. Fazer um loop iterando linha a linha e preencher um std::map<string, string> de headers.
6. Se o método é POST: verificar o Content-Length e confirmar se o tamanho do "resto do buffer" é igual ou maior que essa variável.
7. Requisição validada -> Marcar a requisição como "Completada" e direcionar para o gerador de Respostas.
```

---

## Armadilhas Comuns no Subject 

- **Nunca fazer o parse de uma requisição pela metade!** Aguardar sempre o `\r\n\r\n`.
- **Validar o método preventivamente:** Se enviarem `PUT` e a rota alvo no `.conf` só suportar `GET`, retornar imediatamente um `405 Method Not Allowed`.
- **Sem Caminhos Relativos:** O target na request line tem obrigatoriamente que começar com `/`.
- **Tratamento de Headers Gigantes:** Clientes mal-intencionados mandam headers infinitos para estourar o buffer. Impor um limite de corte (Ex: limite máximo fixado em 8KB).
- **Pipelining HTTP/1.1:** O cliente pode mandar 2 requisições `GET` inteiras agrupadas dentro do mesmo pacote TCP. Quando fatiar pelo `\r\n\r\n`, entender que o "resto" do buffer que sobrou lá dentro não é lixo; pode ser o início exato da próxima requisição que já está entrando.
