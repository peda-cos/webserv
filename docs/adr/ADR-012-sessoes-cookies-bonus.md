# ADR-012: Sessões e Cookies (Bônus)
# Status: Proposed

## Context
Bônus opcional: cookies e gerenciamento simples de sessão.

## Decision
- Gerar `Set-Cookie: session_id=...; HttpOnly; Path=/; Max-Age=...`.
- Store em memória: `std::map<std::string, Session>` com expiração.
- Sem persistência em disco.

## Consequences
- Sessões perdidas em restart.

## Alternatives Considered
- Persistência em arquivo: complexidade extra.

## Estruturas sugeridas
```cpp
struct Session {
    std::string id;
    time_t expires;
    std::map<std::string, std::string> data;
};
```

## Testes
- Cookie enviado e reconhecido.
- Expiração de sessão.

## Referências Cruzadas
- ADR-007 (Erros)
