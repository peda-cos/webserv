# Objetivo do Projeto Webserv — Notas de Estudo

## O que é / Para que serve

Construir um **Servidor HTTP em C++98** que responde a browsers reais — eficiente, não-bloqueante, em uma única thread.

---

## A Analogia — O Restaurante

Imagine que a internet é uma grande praça de alimentação e a missão é construir e gerir o seu próprio **Restaurante (o Servidor Web)**:

| Elemento do Restaurante | Equivalente no Webserv |
|---|---|
| Os Clientes | Navegadores Web (Chrome, Firefox, Safari) |
| O Pedido do cliente ao garçom | Requisição HTTP — `"Me traga a página inicial!"` |
| O Prato entregue | Resposta HTTP — o arquivo HTML, a imagem, etc. |
| O Cardápio/Regulamento | Arquivo de Configuração — porta, rotas válidas, regras de recusa |

---

## Contexto Histórico — O Porquê dos Requisitos

**O servidor antigo (Apache prefork / modelo clássico):**
Quando um novo usuário se conectava, o servidor "contratava um garçom exclusivo" — criando uma Thread ou um novo Processo via `fork()` apenas para aquele cliente. Com 10.000 usuários simultâneos, o servidor tentava criar 10.000 garçons, esgotava a memória e travava tudo. Esse é o famoso problema **C10k**.

**O servidor moderno (nginx / este projeto):**
Em vez de milhares de garçons, usa **UM único garçom superpoderoso**. Esse garçom não fica parado do lado da mesa esperando o cliente decidir se quer batata-frita *(isso seria uma operação bloqueante)*. Ele fica no centro do salão observando todas as mesas simultaneamente e só vai até uma mesa quando o cliente já levantou a mão. Isso é o **I/O Não-Bloqueante com multiplexação**.

**O subject exige que você construa exatamente esse servidor moderno** — em C++98, uma única thread, usando `poll()` (ou equivalentes: `epoll_wait`, `select`, `kqueue`) para atender múltiplos clientes de forma 100% não-bloqueante.

---

## Os 5 Departamentos — Caminho das Pedras

> Não tente misturar os assuntos. Estude um departamento por vez.

### 1. A Leitura do Manual — Parsing do `.conf`
Antes de abrir as portas, o programa lê um arquivo de configuração estruturado que dita: `"escute na porta 8080"`, `"o limite de upload é 5MB"`, `"quando falhar mande o arquivo 404.html"`.

### 2. Abrindo as Portas — A Camada de Sockets
Interação direta com o Sistema Operacional puro: `socket()`, `bind()`, `listen()`. É abrir os "ouvidos" do computador para que mensagens da rede cheguem ao programa.

### 3. O Coração do Servidor — O Event Loop com `poll()`
O pilar nevrálgico. Um loop infinito onde o garçom usa `poll()` para monitorar mil conexões ao mesmo tempo, perguntando continuamente: *"Quem tem dados para eu ler? Para quem as portas de envio já estão livres para eu escrever?"*

### 4. A Conversa — Processamento do HTTP
Quando o cliente envia dados brutos em texto (a Requisição), o programa "fatia" o texto e entende: quer ler uma imagem (GET)? Quer enviar dados de login (POST)? Quer deletar um arquivo (DELETE)? Depois constrói e devolve uma resposta formatada para o navegador.

### 5. A Cozinha — O CGI (Common Gateway Interface)
Se pedem um HTML ou imagem, o servidor só pega na prateleira. Se o browser pede a execução de um script dinâmico (`.php`, `.py`), o servidor não entende Python — ele delega à "cozinha" externa, que processa, retorna o HTML para o garçom e o garçom finaliza a entrega.

---

## As Grandes Pegadinhas — Regras do Subject

### Bloqueio Fatal — Nota = 0
A regra mais cruel do subject: usar funções de leitura/escrita em um momento em que elas pararem a execução aguardando dados ainda não disponíveis. Se o único garçom parar em uma mesa, todas as outras congelam.

### Ser Indestrutível — Nunca Travar
Clientes são imprevisíveis: enviam lixo, desconectam o Wi-Fi no meio do download, pedem caminhos estranhos. O servidor nunca encerra o processo por erro do cliente — responde com graciosidade: `400 Bad Request` ou `500 Internal Server Error`.