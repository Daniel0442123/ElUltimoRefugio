# ADR-0018 · Estado de la simulación en `Partida` con arreglos por tipo, sin ECS

| Campo | Valor |
|---|---|
| Estado | Aceptada (registra lo implementado desde 0.2.0) |
| Fecha | 2026-11-25 |
| Decisores | Equipo de desarrollo |
| Sustituye a | [ADR-0004](ADR-0004-ecs-bus-eventos.md) |
| Requisitos relacionados | RF-040, RF-041, RNF-002, RNF-005, RNF-006, RNF-020, RNF-030 |

## Contexto

ADR-0004 preveía un ECS propio con *sparse sets* y un bus de eventos con colas tipadas. Al construir el
prototipo (0.2.0) el juego resultó tener pocos tipos de entidad, todos conocidos de antemano: Mara, hasta unos
cientos de infectados, un superviviente, tres objetos, cuatro señuelos y ocho barricadas. Lo que importaba era
que el paso de simulación no asignara memoria (RNF-006) y fuera determinista (RNF-020).

## Decisión

El estado vive en `juego::Partida` (`src/dominio/juego/Partida.hpp`): un `std::vector<Infectado>` con capacidad
fija, arreglos `std::array` para señuelos y barricadas, y estructuras para Mara y el superviviente. Todo se
reserva al construir la partida.

* Los «sistemas» son métodos privados que `paso()` llama en orden fijo: movimiento de Mara, acciones, señuelos,
  tick de IA cada `TICK_SIM_HZ / TICK_IA_HZ` pasos (percepción y decisión de cada infectado, cola de rutas,
  superviviente), movimiento de la horda y fin de partida.
* Los eventos `Ruido` se acumulan en un vector con capacidad reservada y se consumen en el siguiente tick de IA;
  `CambioDeMapa` es un contador de versión del grafo que invalida rutas y búsquedas a medias.
* La percepción de cada infectado se construye con `agentes::Percepcion` (visión por cono y Bresenham, oído) y
  la decisión solo usa esa percepción, la memoria del agente y los mensajes recibidos (RF-041).
* Registros (transiciones, planes, mensajes) en anillos de capacidad fija y en `RegistroIa`.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| ECS propio (ADR-0004) | Desacoplamiento entre sistemas, iteración contigua | ≈ 600 líneas de infraestructura sin beneficio con tan pocos tipos de entidad; más difícil de seguir en la sustentación |
| EnTT | Maduro | Dependencia grande |
| Arreglos por tipo dentro de `Partida` | Simple, contiguo, cero asignaciones, orden fijo evidente | `Partida.cpp` es grande y concentra muchas responsabilidades |

## Consecuencias

**Positivas**: el orden del ciclo se lee en un solo método; CP-106 confirma 0 asignaciones por paso; el
determinismo se comprueba con la huella de estado (CP-075, CP-084, CP-086, CP-091).

**Negativas**: la no omnisciencia (RF-041) depende de la disciplina del código y de las pruebas, no de que el
módulo de decisión no pueda ver el mundo. CP-041 cubre la visión, pero su parte de inspección «el módulo no
incluye el registro ECS» ya no aplica. `Partida.cpp` tiene unas 1350 líneas (más 350 del superviviente); si crece más, se debe separar la
horda en su propia clase.

## Verificación

CP-106 (asignaciones), CP-120 (determinismo, parcial), CP-041 y CP-201 (no omnisciencia).
