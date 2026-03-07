# Requisições em "Chunks" (Transfer-Encoding: chunked) — Notas de Estudo

## O que é e qual o Problema que resolve?

Normalmente, o cliente sabe o tamanho do arquivo que vai enviar via `POST` e anuncia isso antecipadamente no Header:
`Content-Length: 1048576` (1MB).

Porém, quando os dados são gerados "em tempo real" (como um áudio sendo gravado ou um log contínuo), o cliente não tem como prever o `Content-Length`.
A solução do HTTP/1.1 para isso é enviar os dados **fatiados em pedaços (Chunks)**. O envio termina quando um **Chunk de tamanho zero** é enviado.

O desafio do Webserv exige explicitamente que o servidor saiba desencapsular corpos com `Transfer-Encoding: chunked`.

---

## A Estrutura de um Body Chunked

O cliente omite o `Content-Length` e insere `Transfer-Encoding: chunked`.
A partir do momento em que o Header encerra em `\r\n\r\n`, o Body chega exatamente assim:

```
<Tamanho_em_Hexadecimal>\r\n
<Dados_Binarios_Desse_Tamanho>\r\n
<Tamanho_em_Hexadecimal>\r\n
<Dados_Binarios_Desse_Tamanho>\r\n
0\r\n
\r\n
```

### Exemplo Prático Visual

O cliente quer enviar a palavra `"HelloWorld!"` (11 bytes), mas fatiada em duas partes `"Hello"` e `"World!"`.

```text
POST /endpoint HTTP/1.1\r\n
Host: localhost:8080\r\n
Transfer-Encoding: chunked\r\n
\r\n
5\r\n                 ← 5 em formato Hexadecimal (Tamanho do primeiro chunk)
Hello\r\n             ← Os 5 bytes reais (A palavra "Hello")
6\r\n                 ← 6 em formato Hexadecimal (Tamanho do segundo chunk)
World!\r\n            ← Os 6 bytes reais (A palavra "World!")
0\r\n                 ← Chunk marcador final com tamanho estrito "0"
\r\n                  ← Finalização oficial da transmissão
```

---

## O Fluxo Lógico no Parsing (Desencapsulando no C++)

O servidor não deve salvar na memória esses marcadores hexadecimais ou as quebras `\r\n` estruturais. A responsabilidade do C++ é extrair unicamente o conteúdo puro (a string `"HelloWorld!"`) e juntar em um único Body legível internamente ou jogado no Request final para processamento/disco.

### Passo a Passo da Implementação do Decodificador:

Dado um `recv_buffer` que acabou de completar os Headers do HTTP:

1. **Procure o início do Body:** Separe todo o conteúdo pós `\r\n\r\n`.
2. **Leia a Linha do Tamanho:** Procure o primeiro `\r\n`. Extraia a string antes dele (ex: `1A`).
3. **Converta Hex para Inteiro:** Converta a string `1A` (Hex) para um número inteiro em C++ (`26` dec).
    - Dica de ouro: Usar `std::stringstream ss; ss << std::hex << str_len; ss >> int_len;`
4. **Verifique se é o Final:** O tamanho retornado é `0`?
    - **SIM:** O Body Chunked chegou completamente no fim. Encerre o decodificador.
    - **NÃO:** Avance.
5. **Leia a Carga Útil (Payload):** Avance os bytes da string o tamanho exato extraído (ex: avance 26 bytes à frente do `\r\n`).
6. **Maneje o `\r\n` de quebra:** Cada Payload tem obrigatoriamente um `\r\n` após seus dados. Pule também o `\r\n` que fica grudado no fundo desse chunk e inicie o laço visualizando novamente o próximo Hexadecimal no passo 2.

---

## Armadilhas Críticas no Webserv (Projeto & Performance)

- **TCP Fragmentation:** A teoria assume que o cliente mandará o chunk perfeito. Na vida real e no Socket não-bloqueante (`poll()`), os bytes são partidos pela MTU. Pode haver o resgate no primeiro `recv()` apenas da metade da palavra `"World"` e o `\r\n` nem chegou.
    - **Solução:** Nunca inicie o unchunk das strings cruamente no mesmo laço de `recv`. Guarde o buffer integral. Tente realizar no parsing a extração e conversão HEX. Se o resultado exigir ler 200 bytes do vetor de String, mas o Size da std::string tiver apenas 50 bytes disponíveis na cauda... o parser **precisa suspender a operação sem errar** (retornando `INCOMPLETO`) e deixar o `poll()` retornar na próxima iterada os bytes restantes para somá-los ao buffer do cliente até que batam o tamanho exigido.
- **Limite de Segurança Limitante:** Os loops hexadecimais em Chunked são as maiores portas de entrada de Denial Of Service (DoS). Subir o Body até a RAM sem verificar a diretriz `.conf` de `client_max_body_size` frita o servidor e causa Malloc Crash. Se em qualquer loop a soma contínua das `strings decodificadas` bater o teto do block parametrizado: retorne rigorosamente status `413 Payload Too Large`.
- **Sintaxe do Fim:** O fecho da conexão TCP da requisição Chunk não é um simples fechamento ou corte. O protocolo exige intrinsecamente observar `0\r\n\r\n` como marcadores no terminal do vetor de leitura. Mantenha as proteções de sintaxe severas para garantir 400 Bad Request em chunks desforme.
