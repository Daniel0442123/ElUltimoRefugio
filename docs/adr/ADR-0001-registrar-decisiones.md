# ADR-0001 · Registrar decisiones de arquitectura con ADR

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | NEG-06, StR-19 |

## Contexto

El proyecto será evaluado en una sustentación (GUIA §8, 10 % de la nota) y el docente pedirá justificar elecciones de algoritmos, patrones y tecnologías. Las decisiones tomadas en conversaciones o en chats se pierden y no quedan enlazadas a los requisitos.

## Decisión

Cada decisión con impacto en la estructura, en un atributo de calidad o en una restricción se registra como un ADR en `docs/adr/` con el formato de este documento (Nygard extendido con alternativas al estilo MADR). Los ADR son inmutables: un cambio se registra como un ADR nuevo que **sustituye** al anterior, y el antiguo pasa a estado `Sustituida por ADR-nnnn`.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Sin registro formal | Cero esfuerzo | Decisiones olvidadas; imposible justificar en la sustentación |
| Wiki de GitHub | Fácil de editar | Fuera del repositorio; no versionada con el código |
| ADR en el repositorio | Versionado, revisado en PR, enlazable | Disciplina del equipo |

## Consecuencias

**Positivas**: Trazabilidad de decisiones hacia los RNF; material directo para la sustentación.

**Negativas**: Requiere escribir un ADR por decisión relevante (≈ 30 min cada uno).

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (NEG-06, StR-19) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
