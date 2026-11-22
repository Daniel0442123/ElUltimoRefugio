# ADR-0008 · FSM para infectados; Utility + GOAP + BT para el superviviente

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-044, RF-054, RF-060 a RF-064, RNF-002, RNF-027 |

## Contexto

Los infectados son muchos y simples, y su comportamiento debe ser legible en el panel (estado actual). El superviviente es uno solo, con metas variadas que requieren secuencias de acciones (recoger tablas, reparar, ayudar).

## Decisión

Los **infectados** usan una **FSM** (patrón State) con 7 estados y transiciones dirigidas por la utilidad de sus evidencias (RF-044, RF-054). El **superviviente** usa **Utility AI** para elegir la meta, **GOAP** (A\* regresivo, profundidad ≤ 6) para planificar la secuencia de acciones y un **árbol de comportamiento** (Composite) para ejecutarlas. Ambos comparten información a través de un **Blackboard**.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| BT para todos | Uniforme | Más costoso por agente; menos legible para 100 infectados |
| GOAP para todos | Muy flexible | Coste O(b^D) por agente; excesivo para la horda |
| Solo FSM | Barato | El superviviente quedaría guionizado; poca planificación visible |
| Híbrido | Cada técnica donde rinde mejor; muestra variedad algorítmica | Tres mecanismos que mantener |

## Consecuencias

**Positivas**: Bajo coste para la horda, planificación rica para el aliado, más contenido algorítmico para la evaluación.

**Negativas**: Más código y pruebas; riesgo RT-01 si GOAP se retrasa.

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RF-044, RF-054, RF-060 a RF-064, RNF-002, RNF-027) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
