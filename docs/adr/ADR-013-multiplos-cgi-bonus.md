# ADR-013: Múltiplos CGIs (Bônus)
# Status: Proposed

## Context
Bônus opcional: suportar múltiplos tipos de CGI por extensão.

## Decision
- `cgi_map` em `LocationConfig` (extensão → interpretador).
- Resolver CGI pelo sufixo do arquivo solicitado.

## Consequences
- Flexibilidade para scripts (.php, .py etc.).

## Alternatives Considered
- Um único CGI global: reduz flexibilidade.

## Testes
- Arquivo .php e .py com intérpretes distintos.

## Referências Cruzadas
- ADR-008 (Execução CGI)
