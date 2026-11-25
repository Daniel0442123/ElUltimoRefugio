# ADR-0007 · A* iterativo con presupuesto y flow fields

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-026, RF-028, RF-029, RNF-002, RNF-003, RNF-030 |

## Contexto

En la Escena 3 decenas de infectados piden ruta casi a la vez. Resolverlas todas en un tick produce picos y viola RNF-002 y RNF-003.

## Decisión

Las búsquedas son **reanudables**: `EstadoBusqueda` guarda abierta, g, padre y cerrado y avanza hasta agotar un presupuesto global de `PRESUPUESTO_NODOS_TICK` nodos por tick, con cola de prioridad por urgencia y antigüedad. Cuando `FLOWFIELD_UMBRAL` o más agentes comparten destino, se calcula un **flow field** con Dijkstra inverso y se comparte. El movimiento final se suaviza con **steering y boids** (separación, alineación, cohesión, evitación).

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| A* completo e inmediato | Simple | Picos de CPU con hordas |
| Hilos de trabajo | Usa varios núcleos | Rompe el determinismo y complica la depuración; los equipos objetivo tienen 2 núcleos |
| HPA* jerárquico | Muy rápido en mapas grandes | Complejidad alta y rutas subóptimas: afecta la comparación de optimalidad |
| Iterativo + flow fields | Carga acotada y amortizada; óptimo | Latencia de algunas rutas de varios ticks |

## Consecuencias

**Positivas**: Tiempo de IA acotado por tick; coste amortizado en hordas; mantiene la optimalidad para la comparación.

**Negativas**: Algunos agentes esperan 1–10 ticks su ruta (se mueven por steering hacia el objetivo mientras tanto).

## Implementación (0.15.0)

- `BuscadorMejorPrimero::iniciar()` y `avanzar(maxExpansiones)` reparten una búsqueda en trozos; el presupuesto se comprueba antes de extraer de la abierta, así ningún nodo queda a medias, y el resultado (ruta, costo, orden de expansión) es idéntico al de la búsqueda de una vez.
- `navegacion::PlanificadorRutas` atiende la cola: perseguir, interceptar y atacar una barricada tienen prioridad 2, investigar y buscar 1 y explorar 0; cada 5 ticks de IA de espera (0,5 s) una solicitud sube un nivel, así ninguna espera para siempre. Solo hay una búsqueda a medias a la vez (memoria O(V)).
- Un `CambioDeMapa` (RF-025) o F2 reinician la búsqueda a medias; si el objetivo del agente cambió mientras se buscaba, recibe la ruta para ir avanzando y se encola otra.
- Medido (CP-026): 100 solicitudes en una ciudad de 128 × 128 se resuelven en 5 ticks sin pasar de 4000 nodos por tick, con unos 0,6 ms por tick (Xeon 2,3 GHz, Release). En una partida de 100 infectados en 128 × 128 la mayor espera fue de 4 ticks de IA.

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RF-026, RF-028, RF-029, RNF-002, RNF-003, RNF-030) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
