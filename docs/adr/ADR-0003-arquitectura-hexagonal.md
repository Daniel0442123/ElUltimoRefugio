# ADR-0003 · Arquitectura hexagonal con dominio de IA puro

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RES-06, RNF-023, RNF-024, RNF-026, RF-041, RF-080 |

## Contexto

La IA es lo que se evalúa (50 % de la nota entre arquitectura de agentes e implementación de algoritmos). Debe probarse sin ventana, ejecutarse en benchmark sin gráficos y demostrarse que los agentes no tienen información omnisciente.

## Decisión

Se aplica **Ports & Adapters**. `refugio_dominio` contiene reglas, ECS, navegación, agentes y experimentos y solo depende de la biblioteca estándar. Define puertos (`IEntrada`, `IRenderizador`, `IAudio`, `IReloj`, `ICargadorMapa`, `IRepositorioPartida`, `IRepositorioConfig`, `IRegistro`, `IInfoSistema`). `refugio_adaptadores` los implementa con raylib, raygui, nlohmann/json y el SO. `app/Composicion` construye el grafo de objetos. Una comprobación de CI prohíbe incluir raylib en `src/dominio/`.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Arquitectura en capas clásica | Conocida | La lógica tiende a depender del motor; difícil probar sin ventana |
| Todo en el bucle de raylib | Rápido al inicio | Imposible ejecutar benchmark sin ventana; IA no aislada |
| Hexagonal | Dominio probado sin ventana; adaptadores intercambiables (reloj fijo, registro en memoria) | Más interfaces y una indirección virtual por llamada a puerto |

## Consecuencias

**Positivas**: Pruebas rápidas y deterministas del dominio; benchmark y optimización reutilizan exactamente el mismo código que el juego; separación visible en la sustentación.

**Negativas**: Coste de llamadas virtuales en los puertos (despreciable: se llaman una vez por frame o por evento, no en bucles internos).

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RES-06, RNF-023, RNF-024, RNF-026, RF-041, RF-080) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
