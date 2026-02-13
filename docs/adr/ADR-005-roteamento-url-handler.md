# ADR-005: Roteamento (URL → Handler)
# Status: Proposed

## Context
É necessário mapear URL para diretiva de rota, aplicar regras de método, redirecionamentos, raiz de arquivos, CGI e upload. O enunciado não exige regex.

## Decision
- Matching por **prefixo mais longo** (sem regex).
- Selecionar `LocationConfig` cujo `path` seja prefixo do request e com maior comprimento.
- Aplicar regras na ordem: métodos → redirecionamento → static/CGI/upload.

## Consequences
- Determinismo e simplicidade.
- Sem custo de regex.

## Alternatives Considered
- Regex: não requerido e aumenta complexidade.

## Implementação (chamadas de sistema)
- Nenhuma específica; usa dados de config.

## Estruturas sugeridas
```cpp
struct RouteMatch {
    const LocationConfig* location;
    std::string effective_path;
};
```

## Testes
- Rotas com prefixos similares.
- Redirecionamento 301/302.
- Método não permitido (405).

## Referências Cruzadas
- ADR-004 (Configuração)
- ADR-006 (Arquivos estáticos)
- ADR-008 (CGI)
