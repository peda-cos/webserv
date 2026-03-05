# WEBSERV — Planejamento Completo de Tasks

**104 tasks · 18 features · C++98 · HTTP/1.1**

---

## Legenda & Convenções

- **ID:** número da feature (00–17) + número da task dentro da feature. Ex: `03.05` = feature 03, task 5.
- **Critério de Aceite:** condição objetiva e testável que define quando a task está concluída.
- **Obs.:** notas técnicas, armadilhas conhecidas ou referências a testes automatizados.
- As features **16 e 17 são bônus** e só serão avaliadas se todas as features 00–15 estiverem funcionais.

---

## Visão Geral

| Nº | Feature | Tasks | Bônus? | Dependências Chave |
|----|---------|:-----:|:------:|-------------------|
| 00 | Estrutura do Projeto & Makefile | 4 | Não | — |
| 01 | Parser de Configuração | 6 | Não | 00 (estrutura) |
| 02 | Event Loop & I/O Multiplexing | 7 | Não | 02 (event loop) |
| 03 | Parser de Requisição HTTP | 8 | Não | 02, estrutura HTTP |
| 04 | Construtor de Resposta HTTP | 7 | Não | 03 (request parser) |
| 05 | Roteamento de Requisições | 5 | Não | 02, 04, config |
| 06 | Método GET — Arquivos Estáticos | 5 | Não | 05, 04 |
| 07 | Método POST — Upload de Arquivos | 4 | Não | 05, 04 |
| 08 | Método DELETE | 3 | Não | 05, 04 |
| 09 | Execução de CGI | 10 | Não | 05, 03, 04 |
| 10 | Robustez & Tratamento de Erros | 5 | Não | todos os anteriores |
| 11 | Conformidade HTTP | 6 | Não | todo o projeto |
| 12 | Análise Estática & Qualidade de Código | 6 | Não | 01–09 |
| 13 | Testes Unitários C++98 | 6 | Não | 10–13 |
| 14 | Testes de Integração & Conformidade | 10 | Não | 14 (integração) |
| 15 | Testes de Stress & Resiliência | 3 | Não | 14, 15 |
| 16 | Bônus — Cookies & Sessões | 5 | **Sim** | 09 (CGI) |
| 17 | Bônus — Múltiplos CGIs | 4 | **Sim** | 09, 17.01 |

---

## 00 · Estrutura do Projeto & Makefile

### 00.01 — Criar estrutura de diretórios do projeto

**Descrição:** Criar os diretórios: `srcs/`, `srcs/server/`, `srcs/http/`, `srcs/config/`, `srcs/cgi/`, `srcs/utils/`, `includes/` e `config/`.

**Critério de Aceite:** Estrutura de pastas existe no repositório conforme descrito.

**Obs.:** Base para toda a organização do código.

---

### 00.02 — Criar o Makefile com regras obrigatórias

**Descrição:** Implementar as regras: `all` (padrão), `clean`, `fclean`, `re` e `$(NAME)`. Compilar com `c++ -std=c++98 -Wall -Wextra -Werror`. Nenhum glob (`*.cpp`) deve ser usado.

**Critério de Aceite:** `make` compila sem erros; `make clean` remove `.o`; `make fclean` remove o binário; `make re` recompila tudo do zero. `make` não revincula quando nada mudou.

**Obs.:** Qualquer glob no Makefile é violação crítica detectada pelo `check_constraints.sh`.

---

### 00.03 — Criar arquivo de configuração padrão (default.conf)

**Descrição:** Criar `config/default.conf` com ao menos um bloco `server` com `listen`, `root`, `index` e uma `location /`.

**Critério de Aceite:** `./webserv config/default.conf` inicia sem erros e aceita conexões TCP.

**Obs.:** Será usado como fixture de smoke test durante o desenvolvimento.

---

### 00.04 — Criar o README.md conforme requisitos do subject

**Descrição:** Incluir: linha itálica com logins, seção Description, seção Instructions (compilação/execução) e seção Resources (referências + uso de IA).

**Critério de Aceite:** `README.md` presente na raiz com todas as seções obrigatórias preenchidas.

**Obs.:** Obrigatório para avaliação; revisitar ao final do projeto para atualizar recursos e descrição de uso de IA.

---

## 01 · Parser de Configuração

### 01.01 — Definir as estruturas de dados de configuração

**Descrição:** Criar `LocationConfig` e `ServerConfig` em `includes/` com todos os campos: host, port, clientMaxBodySize, errorPages, root, index, autoindex, limitExcept, uploadStore e cgiPass.

**Critério de Aceite:** Os tipos compilam sem warnings em C++98.

**Obs.:** Base para os testes unitários de `test_config_parser.cpp`.

---

### 01.02 — Implementar o lexer do arquivo de configuração

**Descrição:** Tokenizar o arquivo de configuração separando palavras-chave, chaves `{` `}` e ponto-e-vírgula `;`. Ignorar comentários (`#`) e espaços.

**Critério de Aceite:** Dado `server { listen 127.0.0.1:8080; }`, o lexer produz os tokens corretos sem crash.

**Obs.:** Manter o lexer separado do parser para facilitar debug.

---

### 01.03 — Implementar o parser do bloco server

**Descrição:** Parsear diretivas de nível server: `listen`, `root`, `index`, `client_max_body_size` (com sufixos K/M/G), `error_page`.

**Critério de Aceite:** `ConfigParser::parse()` retorna um vetor de `ServerConfig` correto. Testes 1–5 do `test_config_parser` passam.

**Obs.:** `client_max_body_size 10M` deve virar `10485760` bytes.

---

### 01.04 — Implementar o parser do bloco location

**Descrição:** Parsear diretivas dentro de `location {}`: `root`, `index`, `autoindex on/off`, `limit_except`, `upload_store`, `cgi_pass`, `return`.

**Critério de Aceite:** Teste 6 do `test_config_parser` passa (todos os sub-campos de location extraídos corretamente).

**Obs.:** `cgi_pass` pode aparecer múltiplas vezes para extensões diferentes.

---

### 01.05 — Implementar detecção de erros no parser

**Descrição:** Lançar `ConfigParseException` para: arquivo vazio, bloco sem `listen`, chave não fechada e `listen` duplicado.

**Critério de Aceite:** Testes 7–10 do `test_config_parser` passam (exceções lançadas nos casos corretos).

**Obs.:** Mensagens de erro devem indicar linha e tipo do problema.

---

### 01.06 — Integrar o parser ao ponto de entrada (main)

**Descrição:** Carregar o arquivo passado como argumento `argv[1]` ou usar um caminho padrão. Exibir erro e encerrar com código não-zero se o parse falhar.

**Critério de Aceite:** `./webserv arquivo_invalido.conf` imprime mensagem de erro e retorna exit code != 0.

**Obs.:** Nenhuma exceção deve escapar do `main()` sem tratamento.

---

## 02 · Event Loop & I/O Multiplexing

### 02.01 — Criar sockets de escuta (listening sockets)

**Descrição:** Para cada bloco server, criar um socket TCP com `socket()`, `bind()` e `listen()`. Definir o socket como não-bloqueante com `fcntl(fd, F_SETFL, O_NONBLOCK)`.

**Critério de Aceite:** O servidor abre uma porta por bloco server e aceita conexões TCP.

**Obs.:** Todos os FDs devem ser não-bloqueantes antes de entrar no `poll()`.

---

### 02.02 — Implementar a instância única de poll()

**Descrição:** Criar o array `pollfd` e registrar todos os listening sockets com `POLLIN`. Implementar o loop principal chamando `poll()` uma única vez por iteração.

**Critério de Aceite:** `check_constraints.sh` reporta exatamente 1 call site de poll/epoll/kevent. O servidor não bloqueia em `accept()` ou `read()`.

**Obs.:** RESTRIÇÃO ABSOLUTA: apenas uma instância de `poll()`/`epoll()`/`kqueue()` em todo o código.

---

### 02.03 — Aceitar novas conexões (accept)

**Descrição:** Quando um listening socket sinalizar `POLLIN`, chamar `accept()` e adicionar o novo FD ao array `pollfd` com `POLLIN`. Configurar o novo FD como não-bloqueante.

**Critério de Aceite:** Múltiplos clientes conseguem conectar simultaneamente; nenhuma conexão fica bloqueada.

**Obs.:** Tratar `EAGAIN`/`EWOULDBLOCK` após `accept()` sem usar errno pós-read/write.

---

### 02.04 — Ler dados do cliente de forma não-bloqueante

**Descrição:** Quando um socket de cliente sinalizar `POLLIN`, chamar `recv()` e acumular dados em um buffer por conexão. Detectar fim de cabeçalhos (`\r\n\r\n`).

**Critério de Aceite:** Requisições HTTP chegam completas ao parser mesmo quando enviadas em múltiplos pacotes TCP.

**Obs.:** Nunca bloquear esperando dados; registrar `POLLIN` antes de cada `recv()`.

---

### 02.05 — Enviar respostas de forma não-bloqueante

**Descrição:** Quando uma resposta está pronta, registrar `POLLOUT` no FD do cliente. No próximo ciclo do `poll()`, enviar com `send()` e remover `POLLOUT` ao terminar.

**Critério de Aceite:** Respostas grandes (>4KB) são enviadas em múltiplos ciclos sem bloquear o servidor.

**Obs.:** Nunca chamar `send()` sem antes verificar `POLLOUT` via `poll()`.

---

### 02.06 — Remover conexões encerradas do poll()

**Descrição:** Detectar desconexão do cliente (`recv()` retorna 0 ou `POLLHUP`). Fechar o FD com `close()` e remover do array `pollfd`.

**Critério de Aceite:** Após desconexão do cliente, o FD não permanece no array `pollfd`. O servidor não vaza descritores.

**Obs.:** Memory e descriptor leaks são causa de instabilidade sob carga.

---

### 02.07 — Suportar múltiplas portas no mesmo poll()

**Descrição:** Registrar todos os listening sockets de todos os blocos server no mesmo array `pollfd`. Rotear `accept()` para o `ServerConfig` correto pelo FD.

**Critério de Aceite:** Com `multi_port.conf`, dois servidores distintos respondem nas portas 8402 e 8403. `test_multi_port.py` passa.

**Obs.:** Mapear FD → ServerConfig em um `std::map` para roteamento eficiente.

---

## 03 · Parser de Requisição HTTP

### 03.01 — Parsear a linha de requisição (request-line)

**Descrição:** Extrair método, URI e versão HTTP da primeira linha. Validar formato exato: `MÉTODO SP URI SP HTTP/versão CRLF`. Retornar erro 400 se houver espaços extras ou campos faltando.

**Critério de Aceite:** Testes 1, 7 e 8 do `test_http_request_parser` passam.

**Obs.:** HTTP/1.1 é a versão alvo; HTTP/1.0 pode ser aceito por compatibilidade.

---

### 03.02 — Parsear os cabeçalhos HTTP

**Descrição:** Extrair todos os cabeçalhos no formato `Nome: Valor CRLF`. Normalizar as chaves para lowercase. Detectar fim dos cabeçalhos com `CRLF CRLF`.

**Critério de Aceite:** Teste 6 do `test_http_request_parser` passa (acesso por `content-type` em lowercase). `Host` ausente retorna 400 (teste 4).

**Obs.:** Case-insensitive por RFC 7230. Duplicatas podem ser concatenadas com vírgula.

---

### 03.03 — Separar path e query string da URI

**Descrição:** Dividir a URI em path (antes de `?`) e queryString (após `?`). Armazenar ambos no struct `HttpRequest`.

**Critério de Aceite:** Teste 5 do `test_http_request_parser` passa (`/search?q=test` → `path=/search`, `queryString=q=test`).

**Obs.:** Decodificação percent-encoding (`%20` etc.) é opcional mas melhora compatibilidade.

---

### 03.04 — Ler o corpo da requisição via Content-Length

**Descrição:** Após os cabeçalhos, ler exatamente `Content-Length` bytes como corpo. Aguardar mais dados se ainda não chegaram todos.

**Critério de Aceite:** Teste 2 do `test_http_request_parser` passa (POST com `Content-Length: 13` → body correto).

**Obs.:** Não bloquear; acumular em buffer até ter `Content-Length` bytes.

---

### 03.05 — Decodificar Transfer-Encoding chunked

**Descrição:** Implementar a decodificação de corpo chunked: ler tamanho do chunk em hex, ler os dados, concatenar no corpo final, parar no chunk zero.

**Critério de Aceite:** Testes 1–4 do `test_chunked_decoder` passam. Teste 3 do `test_http_request_parser` passa.

**Obs.:** Necessário para CGI que receba dados em chunks.

---

### 03.06 — Validar tamanho máximo do corpo (client_max_body_size)

**Descrição:** Se o `Content-Length` ou o corpo acumulado exceder `clientMaxBodySize` do `ServerConfig`, retornar erro 413.

**Critério de Aceite:** Teste 9 do `test_http_request_parser` passa. `test_body_size_limit.py` passa.

**Obs.:** Verificar antes de alocar memória para o corpo completo.

---

### 03.07 — Suportar parsing incremental (dados parciais)

**Descrição:** O parser deve aceitar dados que chegam em fragmentos TCP, retornando "incompleto" até ter a requisição inteira.

**Critério de Aceite:** Teste 10 do `test_http_request_parser` passa. Servidor não trava com requisições lentas.

**Obs.:** Estado do parser deve ser preservado entre chamadas a `feed()`.

---

### 03.08 — Suportar requisições em pipeline

**Descrição:** Detectar quando o buffer contém mais de uma requisição HTTP consecutiva. Parsear apenas a primeira e devolver o restante para o próximo ciclo.

**Critério de Aceite:** Teste 11 do `test_http_request_parser` passa (pipelining retorna a primeira requisição corretamente).

**Obs.:** Relevante para conexões keep-alive e testes de conformance.

---

## 04 · Construtor de Resposta HTTP

### 04.01 — Formatar a status line da resposta

**Descrição:** Gerar `HTTP/1.1 CODE Reason-Phrase\r\n` a partir de um código de status. Manter um mapa de todos os códigos usados pelo projeto.

**Critério de Aceite:** Teste 1 do `test_http_response_builder` passa.

**Obs.:** Códigos obrigatórios: 200, 201, 204, 301, 302, 400, 403, 404, 405, 413, 500, 504.

---

### 04.02 — Adicionar cabeçalhos obrigatórios na resposta

**Descrição:** Incluir sempre: `Content-Type` (baseado na extensão do arquivo), `Content-Length` (tamanho exato do corpo) e `Connection` (close ou keep-alive).

**Critério de Aceite:** Teste 2 do `test_http_response_builder` passa. `test_headers.py` passa (Content-Length bate com o corpo).

**Obs.:** `Content-Length` incorreto causa falhas sérias em clientes HTTP.

---

### 04.03 — Serializar a resposta completa como string

**Descrição:** Concatenar status line + cabeçalhos + `\r\n` + corpo em uma única string pronta para envio via `send()`.

**Critério de Aceite:** Teste 3 do `test_http_response_builder` passa (`Content-Length: 5` para body `hello`).

**Obs.:** A string serializada deve ser exatamente o que vai para o socket.

---

### 04.04 — Gerar páginas de erro padrão (default error pages)

**Descrição:** Para todo código de erro emitido pelo servidor, gerar um HTML mínimo quando nenhuma `error_page` customizada estiver configurada.

**Critério de Aceite:** Teste 4 do `test_http_response_builder` passa. `GET /inexistente` retorna 404 com corpo HTML.

**Obs.:** Obrigatório pelo subject: "default error pages if none are provided".

---

### 04.05 — Servir páginas de erro customizadas

**Descrição:** Quando a configuração tiver `error_page CODE /caminho`, ler o arquivo correspondente e servi-lo como corpo do erro.

**Critério de Aceite:** Teste 5 do `test_http_response_builder` passa. `test_error_pages.py` passa (corpo contém `Custom 404`).

**Obs.:** Fallback para página padrão se o arquivo customizado não existir.

---

### 04.06 — Gerar respostas de redirecionamento (301/302)

**Descrição:** Para location com diretiva `return CODE URL`, gerar resposta com `Location: URL` no cabeçalho e corpo mínimo.

**Critério de Aceite:** Teste 6 do `test_http_response_builder` passa. `test_redirects.py` passa (301 e 302 com `Location` correto).

**Obs.:** Não seguir o redirect; apenas emiti-lo.

---

### 04.07 — Inferir MIME type pela extensão do arquivo

**Descrição:** Manter um mapa de extensões → MIME types cobrindo ao menos: html, htm, css, js, png, jpg, jpeg, gif, ico, txt, pdf, json, svg.

**Critério de Aceite:** `test_headers.py` passa (Content-Type correto para `.html`, `.txt` e `.css`). `image/png` para `.png`.

**Obs.:** Extensão desconhecida → `application/octet-stream`.

---

## 05 · Roteamento de Requisições

### 05.01 — Selecionar o ServerConfig pelo FD de origem

**Descrição:** Mapear o FD do listening socket ao `ServerConfig` correspondente para que a requisição seja processada com a configuração correta.

**Critério de Aceite:** Requisições na porta 8402 usam configuração do primeiro server; porta 8403, do segundo.

**Obs.:** Em ambientes de produção virtual hosts usariam o cabeçalho `Host`; aqui o critério é a porta.

---

### 05.02 — Encontrar a LocationConfig mais específica para a URI

**Descrição:** Dado o path da URI, percorrer as locations em ordem e selecionar a que tem o prefixo mais longo que casa com o path.

**Critério de Aceite:** `/upload/foto.jpg` seleciona `location /upload` antes de `location /`. Path `/` seleciona `location /` quando nenhuma outra casa.

**Obs.:** Sem suporte a regex; apenas correspondência de prefixo literal.

---

### 05.03 — Verificar se o método HTTP é permitido na location

**Descrição:** Se a location tiver `limit_except`, rejeitar métodos não listados com 405 Method Not Allowed.

**Critério de Aceite:** DELETE em location com `limit_except GET` retorna 405. `test_status_codes.py` (405) passa.

**Obs.:** Teste 9 do `test_method_handlers` cobre este comportamento.

---

### 05.04 — Resolver o path físico do arquivo no disco

**Descrição:** Combinar o `root` da location com o path da URI para obter o caminho absoluto no sistema de arquivos. Verificar path traversal (`../`).

**Critério de Aceite:** URI `/kapouet/toto` com `root /tmp/www` resolve para `/tmp/www/toto`. `/../../../etc/passwd` é rejeitado com 403.

**Obs.:** Usar `realpath()` ou verificação manual para evitar directory traversal.

---

### 05.05 — Decidir entre arquivo, diretório e CGI

**Descrição:** Após resolver o path físico: se a extensão estiver em `cgiPass` → CGI; se for diretório → tentar index/autoindex; se for arquivo regular → servir estático; caso contrário → 404.

**Critério de Aceite:** `.py` com `cgi_pass` configurado → lança CGI. `/dir/` sem index e `autoindex off` → 403.

**Obs.:** Usar `stat()` para distinguir arquivo de diretório antes de abrir.

---

## 06 · Método GET — Arquivos Estáticos

### 06.01 — Servir um arquivo estático com GET

**Descrição:** Abrir o arquivo com `open()`, ler com `read()` e incluir o conteúdo no corpo da resposta 200. Definir `Content-Type` pela extensão.

**Critério de Aceite:** `test_get_static.py` passa (HTML, CSS e PNG retornam 200 com Content-Type correto e corpo íntegro).

**Obs.:** `read()` em arquivo de disco não precisa de `poll()`; somente sockets/pipes são sujeitos a essa regra.

---

### 06.02 — Retornar 404 para arquivos inexistentes

**Descrição:** Se `stat()` ou `open()` falhar porque o arquivo não existe, gerar resposta 404 com página de erro padrão ou customizada.

**Critério de Aceite:** `GET /inexistente.html` retorna 404. `test_delete.py` (`test_delete_missing_file`) e `test_status_codes.py` (404) passam.

**Obs.:** Diferenciar "não existe" (404) de "sem permissão" (403).

---

### 06.03 — Servir o arquivo de índice de um diretório

**Descrição:** Se a URI aponta para um diretório e a location tem `index` configurado, tentar servir `root + '/' + index`. Retornar 404 se o index não existir.

**Critério de Aceite:** `GET /` com `index index.html` retorna o conteúdo de `index.html` com status 200.

**Obs.:** Teste 3 do `test_method_handlers` cobre este caso.

---

### 06.04 — Gerar listagem de diretório (autoindex)

**Descrição:** Se a URI aponta para um diretório, não há index configurado, e `autoindex on` está ativo, gerar HTML com lista de arquivos usando `opendir`/`readdir`.

**Critério de Aceite:** `test_autoindex.py` passa: `GET /` retorna 200 com HTML listando `testfile1.txt`, `testfile2.txt` e `testfile3.html`.

**Obs.:** Incluir links `href` relativos para cada entrada. Ignorar `.` e `..`.

---

### 06.05 — Retornar 403 quando diretório não pode ser listado

**Descrição:** Se a URI aponta para um diretório, não há index e `autoindex off`, retornar 403 Forbidden.

**Critério de Aceite:** `test_status_codes.py` (403) passa. Teste 5 do `test_method_handlers` passa.

**Obs.:** `autoindex off` é o padrão quando não especificado.

---

## 07 · Método POST — Upload de Arquivos

### 07.01 — Receber e armazenar o corpo de uma requisição POST simples

**Descrição:** Para POST em location com `upload_store`, salvar o corpo da requisição como arquivo no diretório configurado. Retornar 201 Created.

**Critério de Aceite:** `test_post_upload.py` passa: 201 é retornado e ao menos um arquivo aparece em `UPLOAD_DIR`.

**Obs.:** Nome do arquivo pode ser gerado (UUID ou timestamp) quando não especificado.

---

### 07.02 — Parsear Content-Disposition de multipart/form-data

**Descrição:** Identificar o boundary no `Content-Type`, dividir o corpo em partes, extrair o `filename` de `Content-Disposition` de cada parte.

**Critério de Aceite:** Upload multipart salva arquivo com o nome original ou equivalente. `test_post_upload.py` passa com multipart.

**Obs.:** O boundary é único por requisição; extraí-lo do cabeçalho `Content-Type`.

---

### 07.03 — Rejeitar corpo maior que client_max_body_size com 413

**Descrição:** Verificar o `Content-Length` antes de ler o corpo. Se exceder o limite, retornar 413 imediatamente.

**Critério de Aceite:** `test_body_size_limit.py` (oversized) retorna 413. `test_oom_resilience.py` (oversized) retorna 413 em todas as 20 requisições.

**Obs.:** Verificar `Content-Length` no cabeçalho antes de ler qualquer byte do corpo.

---

### 07.04 — Retornar 405 quando POST não é permitido na location

**Descrição:** Se `limit_except` não incluir POST na location alvo, retornar 405 Method Not Allowed.

**Critério de Aceite:** POST em location com `limit_except GET` retorna 405.

**Obs.:** Consistente com a regra geral de verificação de métodos (task 05.03).

---

## 08 · Método DELETE

### 08.01 — Remover um arquivo existente com DELETE

**Descrição:** Resolver o path físico do arquivo, chamar `unlink()` e retornar 204 No Content sem corpo.

**Critério de Aceite:** `test_delete.py` (`test_delete_existing_file`) passa: 204 retornado e arquivo removido do disco.

**Obs.:** Teste 7 do `test_method_handlers` cobre este comportamento.

---

### 08.02 — Retornar 404 para DELETE de arquivo inexistente

**Descrição:** Se o arquivo não existir no path resolvido, retornar 404 Not Found.

**Critério de Aceite:** `test_delete.py` (`test_delete_missing_file`) passa: 404 retornado.

**Obs.:** Teste 8 do `test_method_handlers` cobre este comportamento.

---

### 08.03 — Retornar 405 quando DELETE não é permitido na location

**Descrição:** Se `limit_except` não incluir DELETE, retornar 405.

**Critério de Aceite:** `test_status_codes.py` (405) passa para DELETE em `/restrict`.

**Obs.:** Consistente com a regra de verificação de métodos.

---

## 09 · Execução de CGI

### 09.01 — Definir as variáveis de ambiente CGI obrigatórias

**Descrição:** Implementar `CgiEnv::build()` que popula `REQUEST_METHOD`, `CONTENT_TYPE`, `CONTENT_LENGTH`, `PATH_INFO`, `QUERY_STRING`, `SERVER_PROTOCOL` e `SCRIPT_FILENAME` a partir de um `HttpRequest`.

**Critério de Aceite:** Todos os 8 testes de `test_cgi_env` passam.

**Obs.:** O CGI espera EOF como fim do corpo; o servidor deve fechar o pipe de escrita após enviar o corpo.

---

### 09.02 — Criar processo filho com fork() + execve()

**Descrição:** Dentro de `srcs/cgi/`, fazer `fork()`. No filho: `chdir()` para o diretório do script, configurar o ambiente com as variáveis CGI, chamar `execve()` com o interpretador correto. No pai: registrar os pipes no `poll()`.

**Critério de Aceite:** `check_constraints.sh` reporta `fork()` apenas em `srcs/cgi/`. Script Python simples é executado com sucesso.

**Obs.:** `fork()` fora de `srcs/cgi/` é violação crítica.

---

### 09.03 — Criar e gerenciar pipes de comunicação com o CGI

**Descrição:** Criar dois pipes: um para stdin do CGI (corpo da requisição) e outro para stdout (resposta). Registrar ambos no `poll()` loop.

**Critério de Aceite:** Corpo da requisição POST chega corretamente ao CGI via stdin. Saída do CGI é lida do pipe de stdout.

**Obs.:** FDs de pipe devem ser não-bloqueantes e registrados no `poll()` do event loop principal.

---

### 09.04 — Enviar o corpo da requisição para o stdin do CGI

**Descrição:** Escrever o corpo da requisição no pipe de escrita para o CGI. Fechar o FD de escrita após enviar todo o corpo (sinaliza EOF ao CGI).

**Critério de Aceite:** CGI Python que lê stdin recebe o corpo correto. Chunked bodies são de-chunked antes de enviar.

**Obs.:** Para chunked: de-chunkar primeiro (task 03.05) e enviar o corpo final decodificado.

---

### 09.05 — Ler a saída do CGI e montar a resposta HTTP

**Descrição:** Ler stdout do CGI acumulando em buffer até EOF. Separar cabeçalhos CGI do corpo. Montar resposta HTTP com os cabeçalhos do CGI mais os cabeçalhos HTTP obrigatórios.

**Critério de Aceite:** `test_cgi_python.py` passa: 200, `Content-Type: application/json`, body JSON válido com `method:GET`.

**Obs.:** CGI pode retornar `Content-Length`; se não retornar, EOF marca fim da resposta.

---

### 09.06 — Aguardar o fim do processo CGI com waitpid()

**Descrição:** Após ler EOF do pipe de stdout, chamar `waitpid()` para recolher o processo filho e evitar processos zumbi.

**Critério de Aceite:** `ps aux` não mostra processos zumbi após execução de CGI. Múltiplas requisições CGI consecutivas não acumulam zumbis.

**Obs.:** Usar `WNOHANG` em modo não-bloqueante se necessário.

---

### 09.07 — Implementar timeout do CGI e retornar 504

**Descrição:** Monitorar o tempo de execução do CGI. Se exceder o timeout configurado (ex: 5s), matar o processo filho com `kill()` e retornar 504 Gateway Timeout.

**Critério de Aceite:** `test_cgi_timeout_504` e `test_504_gateway_timeout` passam: CGI que dorme forever recebe kill e servidor retorna 504.

**Obs.:** Usar timestamp de início e verificar elapsed em cada ciclo do `poll()`.

---

### 09.08 — Retornar 500 quando execve() falha

**Descrição:** Se `execve()` falhar (interpretador não encontrado, permissão negada etc.), retornar 500 Internal Server Error ao cliente.

**Critério de Aceite:** `test_status_codes.py` (500) passa: `/bad-cgi/hello.py` com interpretador inexistente retorna 500.

**Obs.:** O processo filho deve sinalizar a falha ao pai antes de encerrar.

---

### 09.09 — Suportar CGI Python (.py)

**Descrição:** Configurar `cgi_pass .py /usr/bin/python3` e verificar que scripts Python são executados corretamente via shebang ou interpretador direto.

**Critério de Aceite:** `test_cgi_python.py` passa. `test_python_executed` (bonus) passa.

**Obs.:** O script deve ser executável (`chmod +x`) ou o interpretador deve ser passado diretamente ao `execve()`.

---

### 09.10 — Suportar CGI PHP (.php)

**Descrição:** Configurar `cgi_pass .php /usr/bin/php-cgi` e verificar que scripts PHP retornam saída correta. Teste pulado se `php-cgi` não estiver instalado.

**Critério de Aceite:** `test_cgi_php.py` passa (quando `php-cgi` disponível). `test_php_executed` (bonus) passa.

**Obs.:** `php-cgi` tem comportamento ligeiramente diferente de `python3`; testar headers CGI separadamente.

---

## 10 · Robustez & Tratamento de Erros

### 10.01 — Tratar requisições malformadas com 400

**Descrição:** Detectar requisições HTTP inválidas (linha de requisição corrompida, cabeçalhos malformados) e responder com 400 Bad Request antes de fechar a conexão.

**Critério de Aceite:** `test_status_codes.py` (400) passa: `BROKEN GARBAGE\r\n\r\n` retorna 400.

**Obs.:** O servidor nunca deve travar ou ter undefined behavior com entrada inválida.

---

### 10.02 — Nunca crashar sob nenhuma circunstância

**Descrição:** Envolver todos os pontos de entrada de dados externos (sockets, arquivos, CGI) em tratamento de erros. Falhas em alocação de memória devem ser tratadas sem `abort()`.

**Critério de Aceite:** O servidor permanece vivo durante todo o `test_rapid_connect.py` (500 conexões abertas/fechadas rapidamente).

**Obs.:** Regra do subject: "must not crash under any circumstances, even if it runs out of memory".

---

### 10.03 — Verificar que nenhuma requisição fica pendurada indefinidamente

**Descrição:** Implementar timeout de conexão: conexões que não enviam dados completos dentro de um tempo limite devem ser fechadas.

**Critério de Aceite:** Conexões TCP abertas sem envio de dados são fechadas após o timeout. O servidor não acumula conexões ociosas.

**Obs.:** Usar timestamp de última atividade por FD e verificar no loop do `poll()`.

---

### 10.04 — Sobreviver a 100 conexões simultâneas

**Descrição:** Garantir que o servidor responde corretamente a 100 threads fazendo `GET /` ao mesmo tempo sem drops, travamentos ou crash.

**Critério de Aceite:** `test_concurrent.py` passa: todos os 100 threads recebem 200 dentro do timeout.

**Obs.:** A solução com `poll()` não-bloqueante deve lidar com isso naturalmente.

---

### 10.05 — Sobreviver a corpos grandes e oversized repetidos

**Descrição:** Enviar 20 requisições com corpo de 900 bytes (válido) e 20 com 2048 bytes (inválido, acima do limite) sem crash.

**Critério de Aceite:** `test_oom_resilience.py` passa: servidor sobrevive e ainda responde a `GET /` após todas as requisições.

**Obs.:** Liberar buffers de corpo após processar cada requisição.

---

## 11 · Conformidade HTTP

### 11.01 — Garantir que Content-Length bate com o corpo real

**Descrição:** Em toda resposta, calcular o tamanho exato do corpo antes de serializar e incluir o valor correto em `Content-Length`.

**Critério de Aceite:** `test_headers.py` (`test_content_length_matches_body`, `_txt`) passam.

**Obs.:** `Content-Length` incorreto quebra clientes HTTP que re-usam conexões.

---

### 11.02 — Incluir Content-Type em toda resposta 200

**Descrição:** Garantir que toda resposta bem-sucedida inclui `Content-Type` não vazio baseado na extensão do arquivo ou no tipo de conteúdo gerado.

**Critério de Aceite:** `test_headers.py` (`test_content_type_on_200`, `test_content_type_values`) passam.

**Obs.:** autoindex HTML → `text/html`. CGI → usar o `Content-Type` que o CGI retornar.

---

### 11.03 — Não usar Transfer-Encoding chunked nas respostas

**Descrição:** O servidor nunca deve incluir `Transfer-Encoding: chunked` nas respostas (o cliente pode enviar, mas o servidor não é obrigado a usar).

**Critério de Aceite:** `test_headers.py` (`test_no_chunked_transfer_encoding`) passa.

**Obs.:** Simplifica a implementação: sempre usar `Content-Length`.

---

### 11.04 — Incluir Connection: close ou keep-alive válido

**Descrição:** Se o servidor incluir o cabeçalho `Connection`, o valor deve ser `close` ou `keep-alive`. Implementar pelo menos `Connection: close`.

**Critério de Aceite:** `test_headers.py` (`test_connection_header_valid`) passa.

**Obs.:** `Connection: close` é o comportamento mais simples e seguro para começar.

---

### 11.05 — Retornar Content-Length correto em respostas de erro

**Descrição:** Respostas de erro (4xx, 5xx) também devem ter `Content-Length` preciso, especialmente 404.

**Critério de Aceite:** `test_headers.py` (`test_content_length_on_error`) passa.

**Obs.:** Calcular o tamanho da página de erro antes de montar a resposta.

---

### 11.06 — Retornar todos os códigos de status obrigatórios

**Descrição:** Verificar que o servidor emite corretamente: 200, 201, 204, 301, 302, 400, 403, 404, 405, 413, 500 e 504.

**Critério de Aceite:** `test_status_codes.py` passa completamente.

**Obs.:** Cada código tem um caso de teste específico na suíte de conformance.

---

## 12 · Análise Estática & Qualidade de Código

### 12.01 — Corrigir todas as violações de C++98

**Descrição:** Remover: `nullptr` (usar `NULL` ou `0`), `auto`, range-for, lambdas, `std::thread`, `std::mutex`, `#include <thread>/<mutex>/<chrono>`, initializer lists (`= {}`).

**Critério de Aceite:** `check_constraints.sh` (`NO_CPP11_SYNTAX`) não reporta violações. Compilação com `-std=c++98 -Wall -Wextra -Werror` sem erros.

**Obs.:** Usar `NULL`, `std::map`, iteradores e loops `for` tradicionais.

---

### 12.02 — Garantir um único call site de poll()/epoll()/kqueue()

**Descrição:** Auditar todo o código em `srcs/` para garantir que existe exatamente uma chamada a `poll()`, `epoll_wait()` ou `kevent()`.

**Critério de Aceite:** `check_constraints.sh` (`SINGLE_POLL_INSTANCE`) não reporta violações.

**Obs.:** Violação leva a nota 0. Verificar arquivos header também.

---

### 12.03 — Garantir que fork() existe apenas em srcs/cgi/

**Descrição:** Remover qualquer chamada a `fork()` fora de `srcs/cgi/`. Refatorar se necessário.

**Critério de Aceite:** `check_constraints.sh` (`FORK_ONLY_IN_CGI`) não reporta violações.

**Obs.:** Violação leva a nota 0.

---

### 12.04 — Remover uso de errno imediatamente após read/write

**Descrição:** Auditar o código para garantir que `errno` não é lido na linha imediatamente seguinte a `read()`, `write()`, `recv()` ou `send()` para controlar o fluxo.

**Critério de Aceite:** `check_constraints.sh` (`NO_ERRNO_AFTER_READWRITE`) não reporta violações.

**Obs.:** Tratar o retorno de read/write diretamente, não errno.

---

### 12.05 — Substituir #pragma once por include guards #ifndef

**Descrição:** Em todos os headers `.hpp` e `.h`, usar padrão `#ifndef FOO_HPP / #define FOO_HPP / ... / #endif` em vez de `#pragma once`.

**Critério de Aceite:** `check_constraints.sh` (`NO_PRAGMA_ONCE`) não reporta violações.

**Obs.:** `#pragma once` não é padrão em C++98.

---

### 12.06 — Eliminar globs do Makefile

**Descrição:** Verificar que o Makefile não contém `*.cpp`, `*.o`, `*.hpp` ou `$(wildcard ...)`. Listar todos os arquivos `.cpp` explicitamente.

**Critério de Aceite:** `check_constraints.sh` (`NO_MAKEFILE_GLOBS`) não reporta violações.

**Obs.:** Cada novo arquivo `.cpp` adicionado deve ser listado manualmente no Makefile.

---

## 13 · Testes Unitários C++98

### 13.01 — Fazer passar todos os testes do test_config_parser

**Descrição:** Substituir os stubs em `test_config_parser.cpp` pelas includes reais de `ConfigParser` e rodar o binário de teste. Todos os 10 testes devem passar.

**Critério de Aceite:** Binário `test_config_parser` executa com `10 passed, 0 failed`.

**Obs.:** Os stubs retornam vetor vazio; a implementação real deve ser importada.

---

### 13.02 — Fazer passar todos os testes do test_http_request_parser

**Descrição:** Integrar a implementação real do `HttpRequestParser` nos 11 testes. Todos devem passar.

**Critério de Aceite:** Binário `test_http_request_parser` executa com `11 passed, 0 failed`.

**Obs.:** Testes cobrem: GET, POST, chunked, 400, query string, case-insensitive, pipeline.

---

### 13.03 — Fazer passar todos os testes do test_http_response_builder

**Descrição:** Integrar a implementação real do `HttpResponseBuilder` nos 6 testes. Todos devem passar.

**Critério de Aceite:** Binário `test_http_response_builder` executa com `6 passed, 0 failed`.

**Obs.:** Testes cobrem: status line, headers obrigatórios, Content-Length, páginas de erro, redirect.

---

### 13.04 — Fazer passar todos os testes do test_method_handlers

**Descrição:** Integrar a implementação real dos handlers GET/POST/DELETE nos 9 testes. Todos devem passar.

**Critério de Aceite:** Binário `test_method_handlers` executa com `9 passed, 0 failed`.

**Obs.:** Testes dependem do sistema de arquivos real em `/tmp/`.

---

### 13.05 — Fazer passar todos os testes do test_cgi_env

**Descrição:** Integrar a implementação real de `CgiEnv::build()` nos 8 testes. Todos devem passar.

**Critério de Aceite:** Binário `test_cgi_env` executa com `8 passed, 0 failed`.

**Obs.:** Verificar especialmente `PATH_INFO` (extra path após o script) e `QUERY_STRING`.

---

### 13.06 — Fazer passar todos os testes do test_chunked_decoder

**Descrição:** Integrar a implementação real do `ChunkedDecoder` nos 6 testes. Todos devem passar.

**Critério de Aceite:** Binário `test_chunked_decoder` executa com `6 passed, 0 failed`.

**Obs.:** Teste de truncagem (`TruncatedChunkAwaitsMore`) valida o parsing incremental.

---

## 14 · Testes de Integração & Conformidade

### 14.01 — Fazer passar test_get_static.py

**Descrição:** Iniciar o servidor com `single_server.conf` e verificar que HTML, CSS e PNG são servidos corretamente com Content-Type e corpo corretos.

**Critério de Aceite:** Todos os 3 testes de `test_get_static.py` passam.

**Obs.:** Primeiro teste end-to-end; diagnostica problemas no event loop e no response builder.

---

### 14.02 — Fazer passar test_delete.py

**Descrição:** Verificar que DELETE de arquivo existente retorna 204 e remove o arquivo, e DELETE de inexistente retorna 404.

**Critério de Aceite:** Ambos os testes de `test_delete.py` passam.

---

### 14.03 — Fazer passar test_post_upload.py

**Descrição:** Verificar upload multipart: POST retorna 201 e arquivo aparece em `UPLOAD_DIR`.

**Critério de Aceite:** `test_post_upload.py` passa.

**Obs.:** Criar `UPLOAD_DIR` antes de iniciar o servidor.

---

### 14.04 — Fazer passar test_multi_port.py

**Descrição:** Verificar que dois blocos server em portas distintas servem conteúdo diferente.

**Critério de Aceite:** `test_multi_port.py` passa: porta 8402 retorna `PORT_A_CONTENT`, porta 8403 retorna `PORT_B_CONTENT`.

**Obs.:** Ambas as portas devem estar no mesmo `poll()` loop.

---

### 14.05 — Fazer passar test_cgi_python.py e test_cgi_php.py

**Descrição:** Verificar execução de scripts `.py` e `.php` via CGI com resposta JSON/texto correta.

**Critério de Aceite:** `test_cgi_python.py` passa. `test_cgi_php.py` passa (ou é pulado se `php-cgi` ausente).

**Obs.:** Scripts CGI devem ser executáveis e retornar cabeçalhos válidos.

---

### 14.06 — Fazer passar test_autoindex.py

**Descrição:** Verificar que `autoindex on` gera HTML com lista de arquivos do diretório.

**Critério de Aceite:** `test_autoindex.py` passa: corpo contém `testfile1.txt`, `testfile2.txt` e `testfile3.html`.

**Obs.:** HTML gerado deve ser válido e incluir hrefs.

---

### 14.07 — Fazer passar test_redirects.py

**Descrição:** Verificar que `return 301` e `return 302` geram respostas com `Location` correto.

**Critério de Aceite:** `test_redirects.py` passa.

---

### 14.08 — Fazer passar test_error_pages.py

**Descrição:** Verificar que a página de erro customizada (404) é servida quando configurada.

**Critério de Aceite:** `test_error_pages.py` passa: corpo contém `Custom 404`.

---

### 14.09 — Fazer passar test_body_size_limit.py

**Descrição:** Verificar que corpo acima do limite retorna 413 e corpo abaixo é aceito.

**Critério de Aceite:** `test_body_size_limit.py` passa.

---

### 14.10 — Fazer passar a suíte de conformance (test_headers.py e test_status_codes.py)

**Descrição:** Verificar todos os cabeçalhos obrigatórios e todos os códigos de status do projeto.

**Critério de Aceite:** `test_headers.py` passa (6 testes). `test_status_codes.py` passa (12 testes incluindo 504).

**Obs.:** 504 requer implementação do timeout do CGI (task 09.07).

---

## 15 · Testes de Stress & Resiliência

### 15.01 — Fazer passar test_concurrent.py (100 conexões simultâneas)

**Descrição:** 100 threads fazem `GET /` ao mesmo tempo. Todas devem receber 200 dentro de 5 segundos.

**Critério de Aceite:** `test_concurrent.py` passa sem nenhum erro ou timeout.

**Obs.:** Diagnosticar com `siege -c 100 -t 10S` ou `ab -c 100 -n 1000` antes de rodar o teste formal.

---

### 15.02 — Fazer passar test_oom_resilience.py

**Descrição:** 20 POSTs com corpo válido e 20 com corpo oversized não crasham o servidor.

**Critério de Aceite:** `test_oom_resilience.py` passa: servidor sobrevive e responde `GET /` ao final.

**Obs.:** Liberar buffers de requisição após cada resposta enviada.

---

### 15.03 — Fazer passar test_rapid_connect.py

**Descrição:** 500 conexões TCP abertas e imediatamente fechadas não crasham o servidor.

**Critério de Aceite:** `test_rapid_connect.py` passa: <10% de connection errors e servidor ainda responde `GET /`.

**Obs.:** Tratar graciosamente conexões que fecham antes de enviar qualquer dado.

---

## 16 · Bônus — Cookies & Sessões

### 16.01 — Repassar o cabeçalho Cookie ao CGI via HTTP_COOKIE

**Descrição:** Ao popular as variáveis de ambiente do CGI, incluir `HTTP_COOKIE` com o valor do cabeçalho `Cookie` da requisição, se presente.

**Critério de Aceite:** `test_cookies.py` (`test_cookie_forwarded_to_cgi`) passa: CGI recebe o cookie via ambiente.

**Obs.:** Variáveis `HTTP_*` seguem a convenção CGI/1.1 para cabeçalhos da requisição.

---

### 16.02 — Repassar Set-Cookie do CGI para a resposta HTTP

**Descrição:** Ao parsear os cabeçalhos de resposta do CGI, preservar `Set-Cookie` e incluí-lo nos cabeçalhos HTTP da resposta ao cliente.

**Critério de Aceite:** `test_cookies.py` (`test_set_cookie_on_first_request`) passa: resposta contém `Set-Cookie` com `session_id`.

**Obs.:** CGI pode emitir múltiplos `Set-Cookie`; todos devem ser repassados.

---

### 16.03 — Verificar atributos do cookie (HttpOnly e Path)

**Descrição:** Garantir que o `Set-Cookie` emitido pelo CGI de sessão inclui os atributos `HttpOnly` e `Path=/session`.

**Critério de Aceite:** `test_cookies.py` (`test_httponly_attribute`, `test_path_attribute`) passam.

**Obs.:** O servidor repassa os atributos do CGI; não os modifica.

---

### 16.04 — Não crashar com cookie malformado

**Descrição:** Requisição com `Cookie: ;;;===` não deve crashar o servidor; qualquer código HTTP válido é aceitável.

**Critério de Aceite:** `test_cookies.py` (`test_malformed_cookie_no_crash`) passa: `server_is_alive()` retorna `True`.

**Obs.:** Validação mínima; não parsear cookies no servidor, apenas repassar.

---

### 16.05 — Verificar gerenciamento de sessão com visit counter

**Descrição:** Usar o script `session.py`: primeira visita cria sessão com UUID v4, visitas subsequentes com o mesmo cookie incrementam o contador. Duas sessões simultâneas recebem IDs distintos.

**Critério de Aceite:** `test_session_management.py` passa completamente (5 testes incluindo concurrent sessions e visit counter).

**Obs.:** A lógica de sessão está no CGI; o servidor apenas repassa cookies corretamente.

---

## 17 · Bônus — Múltiplos CGIs

### 17.01 — Rotear .py para python3 e .php para php-cgi na mesma location

**Descrição:** Suportar múltiplas diretivas `cgi_pass` em uma location e selecionar o interpretador baseado na extensão do arquivo solicitado.

**Critério de Aceite:** `test_multi_cgi.py` (`test_python_executed`, `test_php_executed`) passam.

**Obs.:** O mapa `cgiPass` já suporta múltiplas entradas; implementar a seleção por extensão.

---

### 17.02 — Retornar 403/404 para extensões CGI não configuradas

**Descrição:** Requisição para `/cgi-bin/script.rb` quando `.rb` não está em `cgiPass` deve retornar 403, 404 ou 500.

**Critério de Aceite:** `test_multi_cgi.py` (`test_unconfigured_extension_error`) passa.

**Obs.:** Não tentar executar arquivos sem handler CGI configurado.

---

### 17.03 — Encaminhar QUERY_STRING para múltiplos tipos de CGI

**Descrição:** Garantir que query strings chegam corretamente a scripts Python e PHP.

**Critério de Aceite:** `test_multi_cgi.py` (`test_both_receive_query_string`) passa: `?foo=bar` aparece no body.

**Obs.:** `QUERY_STRING` já deve estar sendo populado por `CgiEnv::build()`.

---

### 17.04 — Executar requisições .php e .py simultaneamente

**Descrição:** Duas threads fazem GET para `.php` e `.py` ao mesmo tempo; ambas recebem 200.

**Critério de Aceite:** `test_multi_cgi.py` (`test_concurrent_php_python`) passa.

**Obs.:** Cada CGI roda em processo filho separado via `fork()`; `poll()` gerencia os pipes.
