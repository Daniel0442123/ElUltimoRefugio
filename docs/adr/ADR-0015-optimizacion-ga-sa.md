# ADR-0015 · Optimización offline con algoritmo genético y recocido simulado

| Campo | Valor |
|---|---|
| Estado | Aceptada (implementada en 0.13.0) |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-085, RF-086, StR-17 |

## Contexto

El objetivo general incluye problemas de optimización. Los pesos de utilidad y steering son muchos y ajustarlos a mano es lento y poco reproducible.

## Decisión

Se implementa en `experimentos/` un **algoritmo genético** (torneo k = 3, cruce uniforme, mutación gaussiana σ = 0,1, elitismo de 2) y **recocido simulado** (enfriamiento geométrico α = 0,995). La aptitud se evalúa con simulaciones sin ventana con guion fijo y semillas: combina tiempo hasta el contacto, cobertura y una penalización por celdas sobrecargadas y por dificultad excesiva (victorias del guion < 20 %). Los resultados se guardan en JSON y se copian a `config/ia.json`. No se ejecuta durante la partida.

### Implementación (0.13.0)

- **Genes**: los 11 campos de la sección `"pesos"` de `config/ia.json` (steering: seguir, separación, alineación, cohesión y anticipación; tácticas: utilidad de explorar, peso de interceptar e histéresis; perseguidor: radio de visión, velocidad y daño por segundo). Cada gen vive en [0, 1] y se traduce a un rango de búsqueda que contiene el valor por defecto (`experimentos::genes()`).
- **Evaluación**: el experimento de HU-18 en modo coordinado con el guion fijo de Mara, 8 partidas de 90 s por candidato y las mismas semillas para todos (números aleatorios comunes). Las simulaciones se reparten entre hilos y cada una escribe en su posición, así que el resultado no depende del número de hilos.
- **Aptitud**: `presión + cobertura − 0,5 · celdas sobrecargadas por tick de IA − 4 · (max(0; 0,2 − v) + max(0; v − 0,4))`, con v la tasa de victorias del guion. Se añadió el límite superior del 40 %: con solo el inferior, la cobertura (que crece con la duración de la partida) llevaba a una horda que dejaba ganar al guion el 62 % de las veces.
- **GA**: los pesos actuales son el primer individuo; los dos élites pasan sin volver a evaluarse, así que la aptitud del mejor no baja nunca. **SA**: empieza en los pesos actuales; el vecino perturba todos los genes con σ = 0,1; la temperatura inicial es 0,05.
- **Salida**: `results/opt_<ga|sa>_<semilla>.json` con metadatos, configuración, métricas de los pesos iniciales y de los mejores, la sección `"pesos"` lista para copiar y la evolución por generación o por iteración. Copiarla a `config/ia.json` es un paso manual.
- **Hallazgo**: con la semilla 11 los dos algoritmos llegan a una aptitud de ≈ 1,39 (los pesos iniciales dan 0,42), pero lo consiguen bajando la visión y la velocidad del perseguidor por debajo de la de Mara caminando, porque el guion no esquiva. Es el sobreajuste previsto en las consecuencias; por eso los pesos optimizados no sustituyen a los del juego sin probarlos con personas o con más guiones.

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
