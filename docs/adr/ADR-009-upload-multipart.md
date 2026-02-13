# ADR-009: Upload e Multipart/form-data
# Status: Proposed

## Context
Upload é obrigatório. Requests `multipart/form-data` devem ser parseados e persistidos em diretório configurado, com limite de tamanho.

## Decision
- Detectar `Content-Type: multipart/form-data; boundary=...`.
- Parser incremental, gravando em arquivo temporário no diretório de upload (`open`/`write`).
- Enforce `client_max_body_size` com 413.

## Consequences
- Parser mais complexo, porém seguro.
- Evita carregar arquivos grandes em memória.

## Alternatives Considered
- Buffer em memória: risco de OOM.

## Implementação (chamadas de sistema)
- `open`, `write`, `close`, `stat`.

## Estruturas sugeridas
```cpp
struct UploadState {
    std::string boundary;
    std::string filename;
    int fd;
    size_t bytes_written;
};
```

## Detecção de erro sem errno
- Boundary ausente → 400.
- Excedeu tamanho → 413.

## Testes
- Upload pequeno.
- Upload grande (limite).
- Boundary inválido.

## Referências Cruzadas
- ADR-003 (Parsing)
- ADR-004 (Config)
