# ADR-0013 · Mapas Tiled en JSON con nlohmann/json

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-001, RF-031, RES-03, RNF-016 |

## Contexto

Se necesitan mapas de varias escenas y tamaños, editables visualmente, con capas de colisión, costo y objetos.

## Decisión

Los mapas se diseñan en **Tiled** y se exportan a **JSON**; `CargadorTiled` los lee con **nlohmann/json 3.11** y los valida (tamaño ≤ 10 MB, capas obligatorias, m ∈ [1, 10]). Los mapas del benchmark se generan proceduralmente con semilla con el mismo formato lógico.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Formato propio en texto | Sin dependencia | Sin editor visual |
| Tiled TMX (XML) | Formato nativo | Requiere parser XML |
| Tiled JSON | Editor libre; JSON simple de validar | nlohmann/json es una cabecera grande (solo afecta al tiempo de compilación del adaptador) |

## Consecuencias

**Positivas**: Diseño de niveles rápido; la dependencia JSON queda confinada al adaptador.

**Negativas**: Tiempo de compilación del adaptador mayor (≈ 3 s).

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RF-001, RF-031, RES-03, RNF-016) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
