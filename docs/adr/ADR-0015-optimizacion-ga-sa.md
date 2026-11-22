# ADR-0015 · Optimización offline con algoritmo genético y recocido simulado

| Campo | Valor |
|---|---|
| Estado | Propuesta |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-085, RF-086, StR-17 |

## Contexto

El objetivo general incluye problemas de optimización. Los pesos de utilidad y steering son muchos y ajustarlos a mano es lento y poco reproducible.

## Decisión

Se implementa en `experimentos/` un **algoritmo genético** (torneo k = 3, cruce uniforme, mutación gaussiana σ = 0,1, elitismo de 2) y **recocido simulado** (enfriamiento geométrico α = 0,995). La aptitud se evalúa con simulaciones sin ventana con guion fijo y semillas: combina tiempo hasta el contacto, cobertura y una penalización por celdas sobrecargadas y por dificultad excesiva (victorias del guion < 20 %). Los resultados se guardan en JSON y se copian a `config/ia.json`. No se ejecuta durante la partida.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Ajuste manual | Sin código | Lento y no reproducible |
| Búsqueda en rejilla | Simple | Explosión combinatoria con > 6 parámetros |
| Aprendizaje por refuerzo | Potente | Fuera de alcance; la guía no exige aprendizaje (GUIA §4) |
| GA + SA offline | Explicables, comparables entre sí, reproducibles con semilla | Coste de cómputo de las simulaciones (minutos) |

## Consecuencias

**Positivas**: Parámetros justificados con datos; contenido adicional de optimización para el informe.

**Negativas**: Riesgo de sobreajuste al guion de jugadora; se mitiga con varios guiones y semillas.

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RF-085, RF-086, StR-17) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
