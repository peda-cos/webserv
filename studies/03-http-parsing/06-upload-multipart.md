# Uploads e Multipart Form Data — Notas de Estudo

## O Desafio Prático do Upload

Quando um cliente acessa um formulário HTML simples com campos de texto e clica em **Enviar**, o browser despacha os dados empacotados num `POST` padrão (`application/x-www-form-urlencoded`), formando uma string simples como `nome=Jonnathan&idade=30`.

Mas quando o formulário possui um botão "Escolher Arquivo" (`<input type="file">`), a dinâmica estrutural muda completamente. Textos virgens e bytes de imagem se aglutinam na mesma requisição. Para que esta fusão química flua pela rede, o HTTP exige o cabeçalho **`Content-Type: multipart/form-data`**.

Neste momento, o Body exigirá o acionamento de um Parser Especializado ("O Fatiador de Boundaries").

---

## O Conceito de Boundary (A Fronteira)

Ao avisar que a mensagem é `multipart/form-data`, o cliente obrigatoriamente forjará um **Boundary** — um separador de string único e caótico, gerado de forma pseudoaleatória pelo browser para garantir vazamento nulo caso a string "casualmente" fizesse parte do arquivo enviado.

### Exemplo Vital do Header
```http
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7xJ9A
```

A extração deste Boundary contido nos Headers é **mandatória**. Ele será a "faca" que o código C++ utilizará para fatiar o acúmulo raw (bruto) de bytes depositado no Body da Requisição.

---

## A Anatomia Bruta do Body Multipart

Na vigência de um Upload, a payload se fragmenta em Zonas independentes. Cada zona carrega a sua própria sub-header, indicando quem "nome" (`name=`) ou que tipo de dado (`filename=`) está ali embalado.

```http
------WebKitFormBoundary7xJ9A\r\n
Content-Disposition: form-data; name="texto_simples"\r\n
\r\n
Aqui vai o texto puro escrito no formulário.\r\n
------WebKitFormBoundary7xJ9A\r\n
Content-Disposition: form-data; name="meu_arquivo"; filename="logo.png"\r\n
Content-Type: image/png\r\n
\r\n
<Aqui começam milhares de bytes binários de uma foto>\r\n
------WebKitFormBoundary7xJ9A--\r\n
```

### O Tratamento dos 4 Pilares do Multipart
Ao se deparar com a extruturação em ramificações Multipart:

1. **A Marcação Hífen:** Todo separador de bloco do body começa atrelado à dois hifens textuais `--` somados do Token Boundary gerado no HEADER.
2. **Sub-Cabeçalhos:** Após cada boundary, linhas `Content-Disposition` surgem. A extração via substr do delimitador `filename="..."` indica com perfeição o exato nome físico pretendido que a camada `ofstream` salvará no Linux Host.
3. **Data Payload:** Assim que o sub-header encerra num divisor terminal de linha dupla (`\r\n\r\n`), a escrita pura binária se inicia. Ela só interrompe ao colidir mecanicamente com a ocorrência física da próxima string "Boundary" gerada.
4. **Fechamento Terminal:** O indicativo de interrupção da Payload completa inteira se demarca rigidamente pelo Boundary seguido de dois hifens ao final da string (`--Boundary--\r\n`).

---

## Estratégia de Salvamento e Resposta `201 Created`

Ao constatar um ficheiro sendo transferido com a finalidade de upload no servidor, o fluxo seguro em C++98 transitará da seguinte forma:

1. Isolar logicamente a payload pura (eliminando as amarras do Boundary, os hifens e os `\r\n`);
2. Instanciar na biblioteca a extração via System File de salvamento (`std::ofstream output(filename, std::ios::binary);`).
3. Verter incansavelmente os dados e despejá-los ao File local fechando o descritor com `.close()`.
4. Renderizar à Porta de devolução ao `POLLOUT` uma mensagem triunfante enraizada no código universal `HTTP/1.1 201 Created` (Sinalizando a geração vitoriosa no Backend).

---

## Armadilhas Críticas no Webserv (Upload Seguro)

- **Crash Binário ao Fatiar Boundaries com `.find()`:** Funções manipulatórias como `.find()` e `.substr()` originárias de `std::string` buscam quebras terminativas pelo Byte Nulo (`\0`). Porém, ao ler vídeos e Pngs, arquivos binários em sua pureza possuem centenas de caracteres `\0` corrompendo a mecânica e originando um colapso antecipado da varredura de dados no Payload C++.
  - **A Solução Mandatória:** Se a diretiva de captura envolve alvos estritos no File System (`image/png`, `video/mp4`), não tabule a extração de dados brutos utilizando iteradores genéricos de std::string. Limite ou aplique fatiamentos operando sobre varreduras rígidas numéricas de memória (`std::vector<char>`, `memcmp` ou iteradores avançados com o comprimento pré-estabelecido pelo Content-Length delimitado), nunca pelo achismo do caractere invisível.
- **Vazamento e Ocupação Bruta de RAM (Memory Leak):** Carregar um POST Upload integralmente no Heap (memória RAM) antes do Flush no disco (`std::string body = 8MB de vídeo`) ativará sem escrúpulos o Memory Tracker caindo pelo vazamento não-conseguinte do vetor ou pior (limitador RAM exaurido em instâncias paralelas).
  - **Requisito Supremo:** Desenvolver lógicas de escrita temporal *in-time*. À medida que as passadas do recv() chegam após os delimitadores do header... Despeje progressivamente no `ofstream file` de gravação de lotes por vez e invalide dados repetidos em buffer RAM.
- **Upload Bloqueado:** Não esquecer a validação precoce no script inicial! Certificar se o conf bloqueou `POST` de entrar, ou pior: Se a requisição declarou a subida de 2GB e o `limit_max_body` bloqueia nativamente transições de 5MB gerando logo nos pórticos iniciais o código de barreira `413 Payload Too Large`.
