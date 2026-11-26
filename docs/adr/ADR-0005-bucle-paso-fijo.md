# ADR-0005 · Bucle de juego con paso fijo e IA escalonada

| Campo | Valor |
|---|---|
| Estado | Aceptada; aplicada sin repartir la IA en 6 grupos ni interpolar el render (ver arc42 §6.1) |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RNF-002, RNF-018, RNF-020, RF-040, RF-075 |

## Contexto

Los experimentos y la comparación aislado/coordinado exigen reproducibilidad (RNF-020). Un paso variable hace que los resultados dependan de los FPS del equipo. Además, la IA de 100 agentes en un solo frame produce picos.

## Decisión

Se usa un **Game Loop con paso fijo** de 1/60 s y acumulador; el render interpola con `alfa`. La IA se ejecuta a 10 Hz repartiendo los agentes en 6 grupos (un grupo por tick). La velocidad de simulación (0,25× a 4×), la pausa y el paso a paso modifican solo el acumulador.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Paso variable | Simple | No determinista; inestable con dt grandes |
| Paso fijo sin escalonar la IA | Determinista | Picos de CPU cada 6 ticks |
| Paso fijo + IA escalonada | Determinista y carga uniforme | La reacción de un agente puede retrasarse hasta 100 ms (aceptable según RNF-018) |

## Consecuencias

**Positivas**: Determinismo, carga uniforme, control de simulación trivial para la demo.

**Negativas**: Latencia de decisión de hasta 1 tick de IA; requiere interpolación en el render.

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RNF-002, RNF-018, RNF-020, RF-040, RF-075) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
