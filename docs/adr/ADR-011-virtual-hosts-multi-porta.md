# ADR-011: Virtual Hosts e Multi-Porta
# Status: Proposed

## Context
O enunciado exige múltiplas portas; virtual host é opcional, mas desejável para compatibilidade.

## Decision
- Múltiplos listeners por `ServerConfig`.
- Seleção de servidor via (listen_port + Host header).
- Caso Host não corresponda, usar o primeiro server como default.

## Consequences
- Suporte básico de virtual hosts.
- Compatível com browsers que sempre enviam Host.

## Alternatives Considered
- Ignorar Host: simplifica, mas reduz compatibilidade.

## Implementação (chamadas de sistema)
- Reutiliza `bind/listen/accept` da ADR-002.

## Testes
- Dois servidores em portas distintas.
- Host header mapeando server_name.

## Referências Cruzadas
- ADR-002 (Sockets)
- ADR-004 (Config)
