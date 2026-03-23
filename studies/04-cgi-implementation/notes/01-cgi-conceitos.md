# CGI — Conceitos Fundamentais

## O que é CGI?

**CGI (Common Gateway Interface)** é um **protocolo de delegação** que permite ao servidor web executar programas externos e devolver seus resultados aos clientes.

### A Analogia do Restaurante

```
Cliente solicita: "Quero ver meu saldo bancário atualizado"

Garçom (Servidor) pensa:
- Isso é um HTML/asset? Não → pega direto
- Isso é código dinâmico? Sim → delega à "cozinha" (programa externo)

Garçom chama o Chef (program externo):
- Passa os dados do cliente para o Chef via stdin
- Aguarda o Chef processar
- Recolhe a resposta do Chef via stdout
- Devolve para o cliente
```

---

## Por que CGI?

### Antes (sem CGI):
```cpp
// Hardcoded — impossível fazer páginas dinâmicas
if (request_url == "/home.html") {
    send_file("home.html");
}
```

### Com CGI:
```bash
GET /cgi-bin/script.py?user=joao
  ↓ (CGI passa para script.py)
script.py recebe: user=joao
  ↓ (script processa database)
script.py escreve: <html>Saldo: R$ 1000</html>
  ↓ (CGI lê saída)
Servidor devolve HTML gerado
```

---

## Os Atores no Drama CGI

### 1. **Servidor Web** 
- Recebe requisição HTTP
- **Detecta**: "Isso exige CGI"
- **Delega**: Cria processo filho, passa dados
- **Aguarda**: até o filho terminar
- **Coleta**: saída do filho, empacota como resposta HTTP

### 2. **Script/Programa Externo** (`.php`, `.py`, binário C++, etc.)
- **Lê ambiente**: Variáveis de ambiente (`REQUEST_METHOD`, `QUERY_STRING`, etc.)
- **Processa**: Faz a lógica (query database, cálculo, etc.)
- **Escreve**: Resultado em `stdout`
- **Encerra**: Servidor coleta `exit code`

### 3. **Cliente** (Browser)
- Permanece ignorante de tudo isso
- Recebe resposta HTTP como se fosse estática

---

## Fluxo Típico — Visão de Alto Nível

```
┌─────────────────────────────────────────────────────────────┐
│ CLIENTE ENVIA REQUISIÇÃO GET /cgi-bin/calc.py?a=5&b=3      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│ SERVIDOR HTTP                                               │
│ 1. Parse requisição HTTP                                     │
│ 2. Match URL → arquivo no disk?                              │
│    NÃO → Continua... É CGI?                                 │
│ 3. Sim, é CGI → fork()                                       │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│ PROCESSO FILHO (criado por fork())                           │
│ 1. Setup pipes: redireciona stdin/stdout                     │
│ 2. Executa: exec("python", "calc.py")                        │
│ 3. Resultado impresso: "8" (a + b)                           │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│ PROCESSO PAI (servidor)                                      │
│ 1. Lê stdout do filho: "8"                                   │
│ 2. Aguarda término: waitpid()                                │
│ 3. Monta resposta HTTP: HTTP/1.1 200 OK; body="8"            │
│ 4. Envia para cliente                                        │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│ CLIENTE RECEBE RESPOSTA HTTP                                 │
│ Status: 200 OK                                               │
│ Body: 8                                                      │
│ Browser exibe: 8                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Quem Detecta que é CGI?

### Estratégia 1: Por Extensão (Simples)
```
URL: /cgi-bin/script.py
     └─ terminação .py → Executar como Python
```

**Problema**: Requer mapeamento de extensão → intérprete

### Estratégia 2: Por Diretório
```
URL: /cgi-bin/meu-script
     └─ Diretório /cgi-bin/ marcado em config como "CGI"
     └─ Arquivo = executável ou script
     └─ Execute como: /cgi-bin/meu-script
```

Configuração no `.conf`:
```nginx
location /cgi-bin/ {
    # Todos os arquivos aqui são tratados como CGI
}
```

### Estratégia 3: Por Permissão (Mais Seguro)
```
Se arquivo tem permissão executável (x bit),
execute como CGI. Senão, servir como estático.
```

---

## Casos de Uso Reais — CGI no Servidor Web

| Caso | Exemplo | Resultado |
|------|---------|-----------|
| **Query String** | `/calc.py?a=5&b=3` | Variável env `QUERY_STRING="a=5&b=3"` |
| **POST Upload** | `POST /upload.py` com arquivo | Arquivo em `/tmp`, variável `CONTENT_LENGTH` |
| **Formulário** | `POST /login.php` com usuário/senha | Dados no stdin, script faz query BD |
| **Dinâmica Total** | `/banco/saldo.py` | Script conecta BD, retorna HTML gerado |

---

## Resumo Top-Down

```
┌─ CGI = "delegar execução a programa externo"
├─ Server detecta: URL é CGI?
├─ Server executa: fork() → exec("programa")
├─ Server passa dados: via environment variables + stdin
├─ Server aguarda: waitpid() até término
├─ Server coleta: stdout do programa
└─ Server envia: stdout como HTTP response body
```

---

**Mentalidade**: CGI é apenas **IPC (Inter-Process Communication)** formatado como HTTP. Tudo sobre pipes e fork vale para qualquer cenário.
