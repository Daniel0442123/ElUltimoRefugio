# ADR-0004 · ECS ligero propio y bus de eventos interno

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RNF-002, RNF-005, RNF-006, RNF-030 |

## Contexto

Se simulan más de 100 agentes con componentes heterogéneos y muchos eventos (ruidos, cambios de mapa, mensajes). Se necesita iteración cache-friendly, cero asignaciones en el bucle (RNF-006) y bajo acoplamiento entre sistemas.

## Decisión

Se implementa un **ECS mínimo** con entidades de 32 bits (índice + generación) y componentes en *sparse sets*. Los sistemas se ejecutan en orden fijo (arc42 §8.4). La comunicación entre sistemas usa un **bus de eventos** (Observer) con colas tipadas de capacidad fija y doble búfer por tick. Los objetos efímeros salen de **Object Pools**.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Jerarquía de herencia de GameObject | Simple de entender | Mala localidad de caché; herencia rígida |
| EnTT | Maduro y rápido | Dependencia grande con plantillas complejas; más difícil de explicar y depurar en la sustentación |
| ECS propio mínimo | Pequeño, explicable, a medida | Menos funciones (sin consultas complejas ni grupos) |

## Consecuencias

**Positivas**: Datos contiguos, iteración rápida, cero asignaciones en el bucle, sistemas desacoplados.

**Negativas**: Código de infraestructura propio que mantener (≈ 600 líneas); deuda aceptada en arc42 §11.

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RNF-002, RNF-005, RNF-006, RNF-030) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
