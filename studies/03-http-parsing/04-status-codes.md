# Códigos de Status (HTTP Status Codes) — Notas de Estudo

## O que são e para que servem?

A primeira coisa que o servidor Webserv deve enviar na **Response** (Resposta) é o Status Code, localizado na *Status Line*.
Eles são um número de 3 dígitos (ex: `200`, `404`) acompanhados de uma *Reason Phrase* (mensagem curta em inglês, ex: `OK`, `Not Found`).

Eles dizem imediatamente ao Navegador/Cliente:
*"Tudo certo com o que você pediu?"* ou *"Deu ruim, e a culpa é sua"* ou *"Deu ruim, e a culpa é do meu Webserv"*.

---

## As 5 Famílias de Status

A centena inicial do código classifica o tipo de resposta:

| Família | Significado Prático |
|---|---|
| **1xx** | Informativo: *"Recebi, mas a conexão ainda está em progresso".* |
| **2xx** | Sucesso: *"O que você pediu foi recebido, compreendido e aceito!"* |
| **3xx** | Redirecionamento: *"Você precisa fazer mais alguma coisa ou ir para outro lugar."* |
| **4xx** | Erro do Cliente: *"O erro ocorreu por conta do que o usuário enviou (sintaxe, tamanho, permição)."* |
| **5xx** | Erro do Servidor: *"A culpa é interna do Webserv (código quebrou, PHP explodiu)."* |

---

## Tabela Crítica de Erros do Webserv (Projeto Oculto)

Para o projeto Webserv, o Projeto não detalha todos os erros. Mas para aprovação nos testes, o avaliador exigirá o engate correto destes específicos:

### 2xx — Sucesso
- **200 OK**: O padrão rotineiro. Arquivo GET entregue com sucesso, ou Autoindex gerado com sucesso.
- **201 Created**: Usado obrigatoriamente quando um upload (método `POST`) no servidor salva um novo arquivo físico no disco com sucesso.
- **204 No Content**: Usado quando o método `DELETE` tem êxito absoluto apagando o ficheiro e não tem corpo na resposta (body vazio).

### 3xx — Redirecionamento
- **301 Moved Permanently**: O Webserv usa quando o cliente solicita um diretório (`GET /imagens`), mas esquece a barra no final (trailing slash). O servidor responde `301` e força um `Location: /imagens/` nos Headers.

### 4xx — Culpa do Cliente
- **400 Bad Request**: A sintaxe não está no padrão HTTP. Cliente enviou lixo TCP sem headers legíveis ou não enviou o **Host** obrigatório.
- **403 Forbidden**: O arquivo solicitado até existe no disco, ou o diretório alvo até existe. Mas ao tentar acessar com o C++, o `stat()` ou o `ifstream` falha por causa do `chmod` (Sistema Operacional não dá direito de leitura) OR se a rota é um folder e a configuração `.conf` indica `autoindex off;`.
- **404 Not Found**: A função `stat` do disco acusa fisicamente erro indicando que a concatenação Root + Path de fato **não existe** no File System do Linux Host.
- **405 Method Not Allowed**: O cliente enviou um verbo perfeitamente formatado, mas o método especificado (`POST`) foi bloqueado limitativamente pelas chaves da diretiva `limit_except` do respectivo Request URL no arquivo de Configuração (`.conf`).
- **413 Payload Too Large (ou Entity Too Large)**: Falha fundamental do parsing em blocos `POST`. O Content-Length dos cabeçalhos acusou um peso, ou o acúmulo binário do Body ultrapassou estritamente, em número inteiro, a constrição informada em bytes na flag de ambiente `client_max_body_size` do Bloco Local do Conf.

### 5xx — Culpa Interna do Webserv
- **500 Internal Server Error**: Exceção interna (ex. `malloc()` crashou, não há FDs livres, um throw no construtor que o sistema tentou engolir na string).
- **504 Gateway Timeout**: Quando o C++ engatar `execve()` num script CGI do PHP-FPM, este status precisa ser monitorado no processador de timer para estourar a resposta caso o script sofra Loop Infinito, não devolvendo a resposta à Pipe interprocessos num tempo cabível.

---

## Error Pages Customizadas (Obrigatório do Projeto)

Se ocorre um erro (ex. `404`), em vez de mandar um mini texto frio do código (`<h1>404 erro</h1>`), o Webserv deve ser capaz de vasculhar na variável de ambiente do local analisado (arquivo `default.conf`). 

Se tiver o comando `error_page 404 /meu-erro.html;`:
1. O Processamento lê de novo do disco o index físico (`/meu-erro.html`).
2. Mas despacha a Resposta encrustando status `404 Not Found`.
3. Mesmo com a página colorida na tela do Browser, no F12 Network o servidor avisa pro Client que ocorreu `404`.

## Estruturação no Código C++

Um Switch simplificado ou std::map é crucial para centralizar os templates HTML de erros padrões criados preventivamente na memória do projeto.

```cpp
std::string get_default_error_html(int status_code) {
    std::ostringstream ss;
    ss << "<html><body><h1>Erro " << status_code << "</h1></body></html>";
    return ss.str();
}

std::string build_error_response(int status, const std::string& msg) {
    std::ostringstream out;
    std::string body = get_default_error_html(status);
    
    out << "HTTP/1.1 " << status << " " << msg << "\r\n";
    out << "Content-Type: text/html\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "Connection: close\r\n\r\n";
    out << body;
    return out.str();
}
```
