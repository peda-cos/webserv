# ADR-007: Tratamento de Erros e Páginas Customizadas
# Status: Proposed

## Context
O enunciado exige status codes corretos e páginas de erro padrão quando não configuradas. RFC 9110 §15 define semântica de códigos.

## Decision
- Mapear erros para páginas customizadas definidas em config; fallback para HTML padrão embutido.
- Tabela mínima: 400, 403, 404, 405, 413, 500, 501, 503.
- Encerrar conexão em erro grave de parsing ou timeout.

## Consequences
- Respostas consistentes e previsíveis.

## Alternatives Considered
- Responder sempre 500 genérico: viola requisito de precisão.

## Implementação (chamadas de sistema)
- Reutiliza `open/read` de arquivos estáticos para páginas customizadas.

## Detecção de erro sem errno
- Determinada por regras de parser e validações (não por errno).

## Referências
- RFC 9110, Seção 15 (Status Codes).

## Testes
- Método não permitido → 405.
- Body acima do limite → 413.
- Arquivo inexistente → 404.

## Referências Cruzadas
- ADR-003 (Parser)
- ADR-006 (Static)
