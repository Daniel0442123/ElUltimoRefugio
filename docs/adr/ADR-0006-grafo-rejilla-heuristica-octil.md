# ADR-0006 · Grafo implícito 8-conexo y heurística octil

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-020, RF-021, RF-022, RF-024, RF-031, RNF-019 |

## Contexto

La guía exige definir vértices, aristas y costos, usar pesos no negativos y justificar la admisibilidad y la consistencia de la heurística (GUIA §5; STB §03). Los mapas son rejillas de tiles de 32 × 32 px.

## Decisión

El grafo es **implícito** sobre la rejilla: V = celdas transitables, aristas a 8 vecinos sin cortar esquinas, w = d · m(v) con d ∈ {1, √2} y m ∈ [1, 10]. No se almacenan listas de adyacencia: `vecinos(v)` se calcula con desplazamientos. Heurística **octil** con factor m_min = 1, admisible y consistente (demostración en RF-022). Dijkstra se implementa como A\* con h ≡ 0 sobre las mismas estructuras.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Grafo explícito con listas de adyacencia | Generaliza a cualquier grafo | Hasta 8 aristas × 65 536 nodos en memoria; más lento |
| 4-conexo con Manhattan | Más simple | Rutas en zigzag poco naturales |
| Navmesh | Rutas suaves, menos nodos | Complejo de construir y de explicar; la guía pide grafos de costos claros |
| Rejilla 8-conexa implícita + octil | Memoria O(V) en bits/bytes; heurística exacta sin obstáculos | Más nodos que un navmesh |

## Consecuencias

**Positivas**: Memoria mínima, comparación justa Dijkstra/A\* (misma estructura), prueba formal sencilla de admisibilidad y consistencia.

**Negativas**: Las rutas requieren suavizado (lo aporta el steering, ADR-0007).

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RF-020, RF-021, RF-022, RF-024, RF-031, RNF-019) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
