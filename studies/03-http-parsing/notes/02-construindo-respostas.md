# Construindo Respostas HTTP — Notas de Estudo

## O que é uma Resposta HTTP

Assim como o browser envia uma Requisição dividida em 3 partes, o servidor (Webserv) **deve obrigatoriamente** devolver uma Resposta estruturada também em 3 partes:

1. **Status Line** (Linha de Status)
2. **Headers** (Cabeçalhos)
3. **Body** (O conteúdo real: HTML, imagem, etc.)

Se a estrutura não for rigorosamente obedecida (com os `\r\n` corretos), o browser exibe erro ou entra num loop de carregamento infinito.

---

## 1. Status Line — O Julgamento do Servidor

Sempre a primeira linha enviada. Formato obrigatório:
```
versão SP código SP mensagem_ingles\r\n
```
Exemplo:
```
HTTP/1.1 200 OK\r\n
HTTP/1.1 404 Not Found\r\n
HTTP/1.1 405 Method Not Allowed\r\n
```

### Códigos Críticos para mapear no Webserv

| Código | Mensagem | Quando deve ser retornado |
|---|---|---|
| **200** | `OK` | Arquivo encontrado e pronto para envio. |
| **201** | `Created` | Upload de arquivo via POST concluído com êxito. |
| **204** | `No Content` | Arquivo foi deletado via DELETE (sem body na resposta). |
| **400** | `Bad Request` | Cliente mandou requisição malformada (ex: sem Host, sem `\r\n\r\n`). |
| **403** | `Forbidden` | Cliente tentou acessar diretório sem autoindex ligado ou o arquivo não possui permissão de leitura (`chmod`). |
| **404** | `Not Found` | Caminho do arquivo não gerou hit no disco do servidor. |
| **405** | `Method Not Allowed` | A rota no `.conf` só aceita `GET`, mas a requisição usou `POST`. |
| **413** | `Payload Too Large` | O body recebido é maior do que a diretiva `client_max_body_size` do `.conf`. |
| **500** | `Internal Server Error` | Erro genérico interno (ex: CGI travou ou malloc falhou). |

---

## 2. Headers de Resposta — O Manual de Instruções

Após a Status Line, os Headers devem ser despachados.
Eles instruem o browser do cliente de forma categórica: "*o que é isso que chegou?*" e "*qual é o tamanho total em bytes?*"

```
Content-Type: text/html\r\n           ← Qual o tipo de arquivo?
Content-Length: 1024\r\n              ← Qual o tamanho exato em bytes?
Connection: keep-alive\r\n            ← O socket deve ser mantido aberto depois?
\r\n                                  ← FIM DOS HEADERS (linha vazia obrigatória)
```

### A Criptonita do Browser: MIME Types (`Content-Type`)

Navegadores dependem de uma declaração explícita de tipo de arquivo. Se o disco servir um `.png` puramente binário, e o cabeçalho carimbá-lo como `Content-Type: text/plain`, o browser do cliente forçará uma transcrição de caracteres ANSI soltos exibindo jargão caótico na tela.

**A Solução de Código:**
Desenvolva um dicionário lógico (ex: `std::map<std::string, std::string>`) que analise a **extensão do arquivo**, originada da etapa de parsing, e aplique o cabeçalho correto:

| Extensão de Arquivo | Header `Content-Type` a ser despachado |
|---|---|
| `.html` / `.htm` | `text/html` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.png` | `image/png` |
| `.jpg` / `.jpeg` | `image/jpeg` |
| `.pdf` | `application/pdf` |
| *Desconhecido* | `application/octet-stream` (faz o browser abrir prompt local de download seguro) |

---

## 3. O Body — Mapeamento com o Sistema de Arquivos (File System)

Após a injeção da linha vazia `\r\n`, escreva sem interrupção a cascata de bytes virgens originária da varredura de disco.

### Caminho Lógico → Físico

Se a requisição do cliente é: `GET /imagens/logo.png HTTP/1.1`

1. **Leitura:** Isole a rota: `/imagens/logo.png`.
2. **Tradução:** Concatene a string com a diretriz `root` configurada no block server do `.conf` (ex: `/var/www/site`).
   Caminho físico resolvido: `/var/www/site/imagens/logo.png`.
3. **Validação:** Invoque funções nativas ou tente `std::ifstream` sobre a rota combinada.
   - Resultou falso: Aborte e preencha internamente uma resposta para gerar um 404 `Not Found`.
   - Resultou verdadeiro (Arquivo Aberto): Avance.
4. **Resolução de MIMEType:** Isole a `.extensao`, consulte o mapa lógico e preencha a estrutura.
5. **Carregamento (I/O)**: Carregue o descritor utilizando bibliotecas de extração integral e tabule qual o Size (Length) final de transição.
6. **Agrupamento**: Junte: Status Line + Headers preenchidos + `\r\n\r\n` + Conteúdo do disco extraído. Anexe tudo no `send_buffer` do cliente em questão dentro da `struct Client`, para depois liberar o Trigger ao gatilho do bitmask do `POLLOUT`.

---

## Zonas de Alta Cautela para o Servidor

- **`Content-Length` Exigido:** Salvo na circunstância onde um método *Chunked* seja enviado (com *Transfer-Encoding*), a falta da tabulação rigorosa da payload em tamanho fará com que o browser aguarde de maneira crônica sem assumir a interrupção temporal dos bytes.
- **I/O no formato de disco cru (Binário):** Utilize a bandeira técnica de leitura e extração `binary` dentro do `ifstream`, independentemente do que o browser solicitou. Sem a bandeira, arquivos estritamente visuais ou auditivos sairão rasgados.
- **Sinergia do Autoindex de Diretórios:** Em rotas de mapeamento como caminhos `GET /diretorio/` quando houver restrição da verificação `index.html` naquele folder, deverá ocorrer a sondagem da diretriz de condicional do block analisado sob a key `autoindex on;`. Existindo e sendo verificada, engate o `dirent.h` iterando em modo Real-Time os metadados do folder e injete com string C++ uma formtação bruta HTML despachando este HTML no buffer final como um Index recém criado.
- **Disfarce com Arquivos Padrão de Error (Directive `error_page`):** Se a lógica disser `error_page 404 /var/www/erros/desculpe.html`, o código **não deverá** transpor de forma engessada o recado de `404 Not Found` puro, mas injetará na porta do `POLLOUT`  também a payload HTML encontrada nesse folder como desfecho pro navegador do usuário sobre este bad-gateway.
