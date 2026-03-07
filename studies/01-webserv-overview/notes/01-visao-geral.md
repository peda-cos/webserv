# Visão Geral do Projeto Webserv

**Objetivo Central**: Construir um Servidor HTTP em C++ 98 capaz de lidar com múltiplos clientes simultaneamente sem travar, utilizando apenas uma única thread e multiplexação de I/O (`poll()`, `select`, `epoll` ou `kqueue`).

## 1. O Arquivo de Configuração (`.conf`)
- **Servirá para:** Definir as regras e limites do servidor antes dele iniciar.
- **O que fará:** Especificará em quais portas o servidor deve escutar, limites máximos de tamanho de corpo (upload), rotas válidas, caminhos de páginas de erro padrão (ex: `404.html`) e se permite listagem de diretórios.

## 2. A Camada de Sockets (Abertura de Portas)
- **Servirá para:** Conectar o servidor à rede interligando-o ao Sistema Operacional.
- **O que fará:** Abrirá as portas do servidor no SO (`socket()`, `bind()`, `listen()`) para receber as requisições HTTP que chegam das portas mapeadas na configuração.

## 3. Multiplexação de I/O (O Coração do Servidor)
- **Servirá para:** Atender paralelamente a todos os clientes, sem parar, usando apenas **uma única thread**.
- **Como atuará:** Rodará um loop infinito usando a função `poll()` (ou `epoll`, `kqueue`). O `poll()` vigiará todas as conexões simultaneamente e avisará o algoritmo apenas quando uma conexão específica tiver dados prontos para leitura ou estiver livre para o envio de dados.

## 4. Processamento do Protocolo HTTP
- **Servirá para:** Interpretar o desejo do cliente, executar a ação e enviar a resposta.
- **O que fará:** Lerá o texto bruto do cliente, fará o fatiamento (*parsing*) para descobrir o método usado (`GET`, `POST`, `DELETE`) e o URI (caminho) desejado. Logo após, montará uma Resposta HTTP formatada com os dados corretos e o Status Code correspondente (`200 OK`, `404 Not Found`, etc).

## 5. Execução de CGI (Common Gateway Interface)
- **Servirá para:** Gerar e lidar com conteúdo dinâmico (scripts).
- **O que fará:** Usará um subprocesso (`fork()` e `execve()`) para passar a requisição para um binário externo (como o interpretador PHP ou Python). Lerá o que a resposta desse binário calculou devolverá isso ao cliente encapsulado em blocos HTTP.

## Regras Fatais (Limitações)
- **NUNCA FAZER:** 
    - Operações bloqueantes escondidas (`read`/`write` e não-revisadas).
    - O servidor não pode cruzar os braços numa lentidão do cliente. Toda comunicação deve ser rápida.
- **NUNCA DEIXAR:** 
    - O programa encerrar de forma inesperada ou crashar por culpa de entrada mal formada do cliente.
    - O servidor precisa ser extremamente resiliente e blindado (tratar `errno` e retornar status de erros limpos).
