# ADR-006: Serviço de Arquivos Estáticos e Autoindex
# Status: Proposed

## Context
O servidor deve servir conteúdo estático, respeitar `root`, `index` e `autoindex`. Deve funcionar com browsers modernos.

## Decision
- Resolver path físico: `location.root + (target sem prefixo)`. Normalizar para evitar path traversal.
- Se diretório: servir `index` se configurado; caso contrário, gerar autoindex se habilitado; senão 403.
- MIME type por extensão via mapa interno.
- Range requests (RFC 9110 §14 / RFC 9112 §6) podem ser ignorados inicialmente, respondendo 200.

## Consequences
- Simplicidade e compatibilidade.
- Range opcional pode ser adicionado futuramente.

## Alternatives Considered
- Autoindex sempre ligado: viola configuração.

## Implementação (chamadas de sistema)
- `stat`, `open`, `read`, `close`, `opendir`, `readdir`, `closedir`.

## Estruturas sugeridas
```cpp
struct StaticFile {
    std::string path;
    std::string mime;
    size_t size;
};
```

## Detecção de erro sem errno
- `stat` falha → 404.
- Diretório sem index e autoindex desativado → 403.

## Testes
- Diretório com index.
- Diretório sem index e autoindex off/on.
- MIME correto para extensões comuns.

## Referências Cruzadas
- ADR-005 (Roteamento)
- ADR-007 (Erros)
