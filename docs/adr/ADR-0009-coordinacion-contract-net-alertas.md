# ADR-0009 · Contract Net y propagación recursiva acotada de alertas

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-047, RF-049, RF-051, RF-056, RF-084, RNF-027 |

## Contexto

El storyboard exige que los agentes transmitan alertas, distribuyan aproximaciones y elijan perseguir, explorar o interceptar, y que la recursividad se use como «propagación acotada de alertas en el grafo de comunicación, con conjunto de visitados» (STB §03). También exige comparar aislados y coordinados.

## Decisión

Las alertas se propagan con una **función recursiva** `propagar(agente, alerta, p, S)` con caso base `p == ALERTA_PROF_MAX` (3) y conjunto de visitados `S` (bitset), atenuando la confianza 0,7 por salto. El agente que detecta a Mara actúa como gestor de un **Contract Net** de una ronda: anuncia `Perseguir`, `Interceptar` y `Flanquear`, recibe pujas con costo octil y adjudica la menor. Un `AsignadorAccesos` reparte celdas de llegada con un máximo de 2 por celda. Un interruptor global (`modo aislado`) desactiva ambos mecanismos.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Pizarra global de la horda | Simple | Equivale a omnisciencia; viola STB §03 |
| Propagación por inundación iterativa (BFS) | Sin riesgo de pila | No cumple el requisito explícito de recursividad |
| Subastas combinatorias | Asignación óptima | Coste exponencial; innecesario para m ≤ 16 |
| Recursión acotada + Contract Net | Cumple STB §03; comunicación local; explicable | Pila de profundidad 4 (trivial); una ronda de mensajes por detección |

## Consecuencias

**Positivas**: Requisito de recursividad cumplido con análisis de pila sencillo (O(D)); coordinación observable; comparación aislado/coordinado con un solo interruptor.

**Negativas**: Tráfico de mensajes a vigilar con hordas grandes (RT-04).

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RF-047, RF-049, RF-051, RF-056, RF-084, RNF-027) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
