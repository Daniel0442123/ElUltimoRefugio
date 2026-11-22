# ADR-0010 · Quadtree como partición espacial

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-030, RNF-002, RNF-027, RNF-030 |

## Contexto

Percepción, comunicación y separación requieren consultas de vecinos por radio para cada agente. Con fuerza bruta el coste es O(n²) por tick.

## Decisión

Se usa un **quadtree** de puntos (capacidad 8 por hoja, profundidad máxima 8) reconstruido cada tick sobre un pool de nodos. Inserción y consulta son **recursivas**.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Fuerza bruta | Trivial | O(n²): 10 000 comparaciones con 100 agentes, 40 000 con 200 |
| Rejilla uniforme (spatial hash) | O(1) promedio, muy simple | Ineficiente con densidades muy desiguales (hordas concentradas) |
| Quadtree | Se adapta a la densidad; O(log n + k); ejemplo adicional de recursión | Reconstrucción por tick O(n log n) |

## Consecuencias

**Positivas**: Consultas sublineales; segundo uso de recursión con profundidad acotada para la documentación.

**Negativas**: Ligeramente más lento que una rejilla en distribuciones uniformes.

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RF-030, RNF-002, RNF-027, RNF-030) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
