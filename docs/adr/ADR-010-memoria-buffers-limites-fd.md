# ADR-010: Memória, Buffers e Limites de FD
# Status: Proposed

## Context
O servidor não pode crashar e deve manter disponibilidade sob carga. Buffers e descritores precisam de limites claros.

## Decision
- Buffers por conexão com limites máximos (ex.: 8–32KB por leitura).
- Limites de tamanho de request e body (config). Responder 413 ao exceder.
- Se limite de FD atingido: rejeitar novas conexões com 503.

## Consequences
- Previsibilidade de memória.
- Evita OOM e degradação severa.

## Alternatives Considered
- Buffers ilimitados: risco de crash por memória.

## Implementação
- Controle interno em memória; não requer syscalls extras.

## Testes
- Estresse com muitos clientes.
- Requests grandes.

## Referências Cruzadas
- ADR-001 (Event loop)
- ADR-003 (Parsing)
