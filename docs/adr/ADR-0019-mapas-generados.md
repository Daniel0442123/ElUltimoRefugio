# ADR-0019 · Mapas generados por código con semilla y lector JSON propio

| Campo | Valor |
|---|---|
| Estado | Aceptada (registra lo implementado desde 0.1.0) |
| Fecha | 2026-11-25 |
| Decisores | Equipo de desarrollo |
| Sustituye a | [ADR-0013](ADR-0013-mapas-tiled-json.md) |
| Requisitos relacionados | RF-001, RF-031, RF-080, RF-086, RES-03, RNF-016 |

## Contexto

ADR-0013 preveía mapas diseñados en Tiled y leídos con nlohmann/json. El benchmark necesitaba desde el principio
mapas de 32² a 256² generados con semilla (RF-080), y el juego necesitaba un mapa con calles, edificios y
escombros que cambie con la semilla para probar a los agentes en situaciones distintas. Los únicos JSON que lee
el programa son la configuración y los experimentos.

## Decisión

* La ciudad del juego la genera `juego::crearEscenarioAleatorio` (32 × 22 celdas, 10 infectados, objetos, radio,
  almacén y extracción) y los mapas del benchmark `experimentos::GeneradorMapas`, ambos con PCG32 y semilla.
  Los escombros dan el multiplicador de terreno m ∈ [1, 10] (RF-031).
* Los JSON se leen con un lector propio (`dominio/nucleo/Json`, ≈ 260 líneas) que devuelve errores con la
  posición y el campo, en lugar de nlohmann/json.
* RF-001 (cargar mapas de Tiled) queda **pendiente**. Si se implementa, será un adaptador que construya el mismo
  `Escenario` que hoy produce el generador, así que el dominio no cambia.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Tiled + nlohmann/json (ADR-0013) | Diseño visual de niveles | Sin escenas fijas no aporta; dependencia que el dominio no puede incluir (RNF-023) |
| Generación con semilla | Infinitos mapas reproducibles; el mismo código sirve al juego, al benchmark y a los experimentos | Los niveles no se diseñan a mano; no hay escenas del storyboard separadas |

## Consecuencias

**Positivas**: un mapa distinto por semilla, sin archivos de mapa que validar; el lector JSON propio cabe en el
dominio y pasa el corpus de CP-116.

**Negativas**: RF-001 y el formato IS-01 no están implementados; las cinco escenas del storyboard son momentos de
una misma partida (ver StRS §4.4).

## Verificación

CP-031 (costos), CP-080 (benchmark con mapas generados), CP-116 (corpus de JSON inválidos). CP-001 queda pendiente.
