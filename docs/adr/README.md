# Registro de decisiones de arquitectura (ADR)

> Formato: Michael Nygard extendido con la sección de alternativas de MADR. Los ADR son inmutables; un cambio crea un ADR nuevo que sustituye al anterior, y en el antiguo solo se actualiza el campo «Estado». ADR-0018 a ADR-0020 registran decisiones que se tomaron durante la implementación (0.1.0 a 0.16.0) y que se documentaron en la revisión del 2026-11-25.

| ADR | Título | Estado | Requisitos |
|---|---|---|---|
| [ADR-0001](ADR-0001-registrar-decisiones.md) | Registrar decisiones de arquitectura con ADR | Aceptada | NEG-06, StR-19 |
| [ADR-0002](ADR-0002-cpp20-raylib.md) | C++20 con raylib 5 y raygui | Aceptada; raygui sustituido por ADR-0020 | RES-01, RES-02, RNF-001 a RNF-008, RNF-009, RNF-028 |
| [ADR-0003](ADR-0003-arquitectura-hexagonal.md) | Arquitectura hexagonal con dominio de IA puro | Aceptada | RES-06, RNF-023, RNF-024, RNF-026, RF-041, RF-080 |
| [ADR-0004](ADR-0004-ecs-bus-eventos.md) | ECS ligero propio y bus de eventos interno | Sustituida por ADR-0018 | RNF-002, RNF-005, RNF-006, RNF-030 |
| [ADR-0005](ADR-0005-bucle-paso-fijo.md) | Bucle de juego con paso fijo e IA escalonada | Aceptada; sin grupos ni interpolación | RNF-002, RNF-018, RNF-020, RF-040, RF-075 |
| [ADR-0006](ADR-0006-grafo-rejilla-heuristica-octil.md) | Grafo implícito 8-conexo y heurística octil | Aceptada | RF-020, RF-021, RF-022, RF-024, RF-031, RNF-019 |
| [ADR-0007](ADR-0007-astar-iterativo-flow-fields.md) | A* iterativo con presupuesto y flow fields | Aceptada | RF-026, RF-028, RF-029, RNF-002, RNF-003, RNF-030 |
| [ADR-0008](ADR-0008-modelo-decision-hibrido.md) | FSM para infectados; Utility + GOAP + BT para el superviviente | Aceptada; sin Blackboard | RF-044, RF-054, RF-060 a RF-064, RNF-002, RNF-027 |
| [ADR-0009](ADR-0009-coordinacion-contract-net-alertas.md) | Contract Net y propagación recursiva acotada de alertas | Aceptada | RF-047, RF-049, RF-051, RF-056, RF-084, RNF-027 |
| [ADR-0010](ADR-0010-quadtree.md) | Quadtree como partición espacial | Aceptada | RF-030, RNF-002, RNF-027, RNF-030 |
| [ADR-0011](ADR-0011-persistencia-local.md) | Persistencia solo local: JSON y binario con CRC | Aceptada; concretada en ADR-0016 | RF-090 a RF-093, RNF-007, RNF-016, RNF-017, RNF-021, RNF-022 |
| [ADR-0012](ADR-0012-determinismo-pcg32.md) | Determinismo con PCG32 y hash de estado | Aceptada | RF-086, RNF-020, RF-084 |
| [ADR-0013](ADR-0013-mapas-tiled-json.md) | Mapas Tiled en JSON con nlohmann/json | Sustituida por ADR-0019 | RF-001, RF-031, RES-03, RNF-016 |
| [ADR-0014](ADR-0014-estrategia-calidad.md) | Catch2, sanitizadores, Tracy y CI | Aceptada; modificada por ADR-0020 | RNF-015, RNF-016, RNF-024, RNF-025, RNF-009, RNF-028 |
| [ADR-0015](ADR-0015-optimizacion-ga-sa.md) | Optimización offline con algoritmo genético y recocido simulado | Aceptada | RF-085, RF-086, StR-17 |
| [ADR-0016](ADR-0016-guardado-por-repeticion.md) | Guardar la partida como inicio + entradas y repetirla al cargar | Aceptada | RF-090, RF-091, RF-093, RNF-007, RNF-017, RNF-021 |
| [ADR-0017](ADR-0017-arte-pixel-iluminacion.md) | Arte pixel generado por código con luz 2D y sombras | Aceptada | RF-016, RF-071, RNF-006, RNF-012 |
| [ADR-0018](ADR-0018-estado-en-partida-sin-ecs.md) | Estado de la simulación en `Partida` con arreglos por tipo, sin ECS | Aceptada; sustituye a ADR-0004 | RF-040, RF-041, RNF-002, RNF-005, RNF-006, RNF-020, RNF-030 |
| [ADR-0019](ADR-0019-mapas-generados.md) | Mapas generados por código con semilla, mapas de Tiled opcionales (0.18.0) y lector JSON propio | Aceptada; sustituye a ADR-0013 | RF-001, RF-006, RF-031, RF-048, RF-080, RF-086, RES-03, IS-01, RNF-016 |
| [ADR-0020](ADR-0020-herramientas-reales.md) | Ajustar la interfaz y la estrategia de calidad a las herramientas usadas | Aceptada; modifica ADR-0002 y ADR-0014 | RES-02, RES-10, RNF-001 a RNF-005, RNF-009, RNF-016, RNF-024, RNF-025 |
| [ADR-0021](ADR-0021-ventana-adaptable-tipografia.md) | Ventana adaptable con interfaz escalada y tipografía Inter incrustada | Aceptada; modifica ADR-0017 | RF-016, RF-070, RNF-012 |

## Plantilla para un ADR nuevo

```markdown
# ADR-nnnn · Título en forma de decisión

| Campo | Valor |
|---|---|
| Estado | Propuesta / Aceptada / Rechazada / Obsoleta / Sustituida por ADR-nnnn |
| Fecha | AAAA-MM-DD |
| Decisores | |
| Requisitos relacionados | |

## Contexto
## Decisión
## Alternativas consideradas
## Consecuencias
## Verificación
```
