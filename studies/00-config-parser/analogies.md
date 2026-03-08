# Analogias: Parser de Configuração

Este documento traduz os conceitos técnicos de arquitetura e roteamento de requisições em analogias do mundo real para consolidar o aprendizado.

## 1. O Prédio Comercial (Server vs. Location)

**Contexto:** Roteamento de virtual hosts e blocos de configuração do NGINX.

- **O Terreno (SO/IP):** O servidor físico (sua máquina) rodando o Webserv. O endereço IP é o "endereço da rua".
- **O Prédio Comercial (`ServerConfig`):** Cada bloco `server {}` no arquivo `.conf` representa um prédio diferente construído nesse terreno. Você pode ter um prédio para a empresa A (`site1.com`) e outro para a empresa B (`site2.com`).
  - *Atributos:* O porteiro verifica regras globais, como o endereço e a porta de entrada, limites máximos de carga permitida no prédio (`client_max_body_size`) e mantém as placas padrão de aviso caso alguma sala não exista (`error_page`).
- **As Salas/Departamentos (`LocationConfig`):** Dentro do prédio, os visitantes querem ir para cômodos específicos mapeados pela URL, como `/upload` ou `/admin`. Cada cômodo impõe suas próprias regras de entrada:
  - `limit_except`: O segurança da porta da sala. Ele diz se você só pode "Olhar e pedir" (GET), ou "Deixar caixas" (POST), ou "Destruir papéis" (DELETE).
  - `root`: A prateleira física real no HD do computador onde os papéis daquela sala estão guardados (ex: `/var/www/uploads`).
  - `return`: O aviso na porta dizendo "Esta sala se mudou. Vá para o URL X" (Redirecionamento).

**Quando usar essa analogia:** Ao interpretar a configuração e no momento de processar uma requisição HTTP ("Qual prédio o cliente quer? -> Qual sala ele tentou entrar? -> Ele tem permissão?").
