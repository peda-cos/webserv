# Tratamento de Diretórios e Autoindex — Notas de Estudo

## O Desafio: E se o caminho não for um arquivo?

Uma requisição para `/imagens/logo.png` busca claramente um arquivo. Mas e quando o cliente solicita apenas `/imagens/` ou acessa a raiz `/`?

O Webserv precisa invariavelmente distinguir entre um **Arquivo Físico** e um **Diretório**. Sendo um diretório, o fluxo da construção da resposta HTTP muda de forma radical.

---

## Passo 1: Como identificar um Diretório em C++98?

A syscall `stat()` fornece as propriedades de qualquer caminho resolvido no disco.

```cpp
#include <sys/stat.h>

struct stat st;
if (stat("/var/www/site/imagens", &st) == 0) {
    if (S_ISDIR(st.st_mode)) {
        // O ALVO É UM DIRETÓRIO!
    } else if (S_ISREG(st.st_mode)) {
        // O ALVO É UM ARQUIVO COMUM!
    }
}
```

---

## Passo 2: A Regra do `index`

Quando um diretório é solicitado (ex: `GET /`), a convenção web dita a tentativa de entregar um "arquivo padrão" de navegação pacífica. O arquivo `.conf` definirá o nome desse alvo através da diretiva `index`.

**Exemplo no `.conf`:**
```nginx
location / {
    index index.html index.php;
}
```

**Lógica de Implementação Mínima:**
1. A requisição HTTP aponta para `/`.
2. A concatenação lógica forma o alvo da leitura do `ifstream` para `/var/www/site/`.
3. A verificação via `stat()` acusa a presença de diretório.
4. Faça a busca por `/var/www/site/index.html`. Sendo `true`, anexe a carga do payload na requisição para o desfecho.
5. Se não existir, busque `/var/www/site/index.php`. Sendo `true`, envie ao engate CGI para intercepção.
6. Se **nenhum** arquivo padrão for rastreado localmente na raiz daquela rota... acione o recurso de Autoindex!

---

## Passo 3: O Funcionamento do `autoindex`

Se o `.conf` do Bloco define a diretiva `autoindex on;` e o gatilho da leitura varreu sem sucesso todas as opções passadas na flag de `index`, o servidor fica obrigado a **fabricar** de maneira bruta e procedural na memória RAM um buffer HTML listando a árvore imediata real daquela pasta.

Se o arquivo de origem indicar `autoindex off;`, simplesmente interrompa a conexão na cara do cliente despachando um envelope de status com a diretriz de `403 Forbidden` (Acesso Proibido).

### Como ler pastas no disco em C/C++? (`opendir` e `readdir`)

A biblioteca nativa `<dirent.h>` permite abrir a estrutura de arquivos e fazer a tabulação entrada por entrada.

```cpp
#include <dirent.h>

DIR* dir = opendir("/var/www/site/imagens");
if (dir != NULL) {
    struct dirent* entry;
    std::string html_list = "";
    
    // Faça a leitura das entradas vinculadas ao interior da pasta alvo
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        
        // Pode-se pular links auto-referenciais "." e ".." 
        if (name == "." || name == "..") continue;
        
        // Construa o Link procedural encapsulando via <a> numa string-buffer do html para enviar no envio.
        html_list += "<a href=\"" + name + "\">" + name + "</a><br>\n";
    }
    // CRÍTICO: Sempre zere os descritores (close).
    closedir(dir);
}
```

---

## O Fluxo Completo de Resposta (Arquivo vs. Diretório)

```text
1. Caminho traduzido: /var/www/site/caminho_requisitado
2. Syscall de stat() avalia como:
   ├── ARQUIVO:  Instancie logica do ifstream, aloque extensão em Mime-Type e retorne de sucesso: 200 OK.
   └── DIRETÓRIO:
         ├── Existe algum parâmetro `index` configurado e presente neste root do disco?
         │    └── SIM: Resolva, acione Mime-Type e despache: 200 OK.
         └── NÃO:
               ├── O arquivo .conf contém a verificação `autoindex on` mapeado neste location?
               │    ├── SIM: Acione o parser dirent.h abrindo a opendir(), e despache como 200 OK.
               │    └── NÃO: Negue o acesso de inspeção local e responda abortivamente: 403 Forbidden.
```

---

## Armadilhas Ocultas no Tratamento de Diretórios do Webserv

- **A Temível Barra Final (Trailing Slash):**
  Se a string do target HTTP é instanciada para um acesso de pasta puramente como `GET /imagens` (falta de barra) mas o disco do servidor possui a verificação formal que é DIRETORIO, a melhor alternativa segura sem conflitos nos Mimes é formatar uma resposa HTTP que despache de maneira invisível pela rede técnica do Browser **`Redirect - 301 Moved Permanently`** com destinação ao alvo com a barra anexada e fixada de `/imagens/`.
  Qual é a fundamentação disto no subject? Se o browser receber do Autoindex de links sem barra a renderização procedural do servidor e o root renderizar na tabulação de `a href` um clique pro ficheiro `desenho.png`, no HTML real de client-side ele enviaria uma repetição de path quebrada indo bater diretamente no `/desenho.png` (raiz fictícia). Forçando nativamente a barra o cliente pede tecnicamente ao Webserv o URI estrito: `"GET /imagens/desenho.png HTTP/1.1"`.

- **Vazamentos Passivos (Leak FD Limit com `DIR*`):**
  Ao processar o `opendir()` na árvore estendida e a geração procedural acorrer nas instâncias auto-formatadas em C++98, se qualquer percurso ou falha acidental nas verificações abortar o percurso procedural de listagens sem que antes uma função explícita e irremediável ocorra e faça valer invocar formalmente a `closedir(dir)`, a longo prazo e devido aos limitadores do OS e das proteções severas do script do servidor virtual, o seu projeto passará invariavelmente do número restrito de Files Descriptors estourando a barreira real e morrendo em silent seg-ault durante a batida nativa do `poll()`. Todo index processado internamente (quer com erro em sub-branches quer não) exige a injeção terminativa de close nas instâncias manipuladas do `DIR *`.
