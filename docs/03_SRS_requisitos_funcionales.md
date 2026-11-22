# 03 · Especificación de Requisitos del Software (SRS) · Requisitos funcionales

> Estructura basada en ISO/IEC/IEEE 29148:2018, cláusula 9.6 (contenido del SRS).
> Cada requisito funcional se redacta con **EARS** y lleva los atributos de 29148:2018 §5.2.8
> (identificación, prioridad, fuente, justificación, criticidad, estabilidad, riesgo, método de verificación y estado).

| Campo | Valor |
|---|---|
| Documento | SRS-REFUGIO |
| Versión | 0.1.0 |
| Fecha | 2026-11-21 |
| Estado | Borrador |
| Requisitos no funcionales | En [04_requisitos_no_funcionales.md](04_requisitos_no_funcionales.md) |

---

## 1. Introducción

### 1.1 Propósito

Definir de forma verificable el comportamiento del software **El último refugio** para guiar el diseño,
la implementación y las pruebas, y para que el docente pueda comprobar el cumplimiento de la guía del proyecto.

### 1.2 Alcance

El software es un ejecutable de escritorio (`refugio`) con dos modos: **gráfico** (juego, panel de monitoreo y
comparación en vivo) y **sin ventana** (benchmark y optimización). El alcance de negocio está en
[01_vision_StRS.md](01_vision_StRS.md) §1.2.

### 1.3 Perspectiva del producto

Sistema autónomo, sin dependencias de red. Interactúa con el sistema operativo (archivos, ventana, entrada,
OpenGL 3.3) a través de raylib. Su núcleo es un **dominio de IA puro** en C++ aislado por puertos (ver arc42 §5).

### 1.4 Funciones del producto (resumen)

| Grupo | Funciones | RF |
|---|---|---|
| F1 Juego base | Mapa, movimiento, ruido, objetos, refugio, extracción, señuelo, barricada, HUD, tutorial | RF-001 a RF-017 |
| F2 Navegación | Grafo, Dijkstra, A\*, métricas, replanificación, presupuesto, flow fields, steering, quadtree | RF-020 a RF-031 |
| F3 Infectados | Ciclo de agente, percepción, memoria, FSM, exploración, alertas recursivas, Contract Net, tácticas | RF-040 a RF-056 |
| F4 Superviviente | Utility AI, GOAP, árbol de comportamiento, integridad del refugio | RF-060 a RF-064 |
| F5 Monitoreo | Panel, superposiciones, registro de planes y mensajes, comparación, control de simulación | RF-070 a RF-075 |
| F6 Experimentación | Benchmark, metadatos, optimalidad, escenarios dinámicos, aislado/coordinado, optimización, semillas | RF-080 a RF-086 |
| F7 Persistencia | Guardar, cargar, configuración, archivos corruptos | RF-090 a RF-093 |

### 1.5 Restricciones, supuestos y dependencias

Ver StRS §3.3 (RES-01 a RES-10) y §6.1 (SUP-01 a SUP-04).

### 1.6 Plantillas EARS utilizadas

| Tipo | Plantilla | Uso |
|---|---|---|
| **Ubicuo** | El ‹sistema› deberá ‹respuesta›. | Propiedades siempre verdaderas. |
| **Evento** | **Cuando** ‹disparador›, el ‹sistema› deberá ‹respuesta›. | Reacción a un evento. |
| **Estado** | **Mientras** ‹estado›, el ‹sistema› deberá ‹respuesta›. | Comportamiento durante un estado. |
| **Opcional** | **Donde** ‹característica esté incluida›, el ‹sistema› deberá ‹respuesta›. | Funciones configurables. |
| **No deseado** | **Si** ‹condición no deseada›, **entonces** el ‹sistema› deberá ‹respuesta›. | Errores y situaciones anómalas. |
| **Complejo** | Combinación de las anteriores (Mientras … cuando …). | Casos con estado y evento. |

### 1.7 Leyenda de atributos

* **Prio.**: MoSCoW. **Crit.**: criticidad para la nota o el funcionamiento (Alta / Media / Baja).
* **Estab.**: probabilidad de cambio (Estable / Media / Volátil). **Riesgo**: técnico (Alto / Medio / Bajo).
* **Verif.**: I inspección, A análisis, D demostración, P prueba, seguido del caso de prueba.
* **Estado** inicial de todos los requisitos: `Propuesto`.

---

## 2. Parámetros del sistema

Los umbrales se definen como parámetros con nombre en `config/ia.json` para poder ajustarlos sin cambiar
requisitos (STB §03 «Libertad de desarrollo»). Los requisitos citan el parámetro y su valor por defecto.

| Parámetro | Valor por defecto | Unidad | Usado en |
|---|---|---|---|
| `TICK_SIM_HZ` | 60 | Hz | RF-075, ADR-0005 |
| `TICK_IA_HZ` | 10 | Hz | RF-040 |
| `VEL_SIGILO` / `VEL_CAMINAR` / `VEL_CORRER` | 1.5 / 3.0 / 6.0 | celdas/s | RF-002 |
| `RUIDO_SIGILO` / `RUIDO_CAMINAR` / `RUIDO_CORRER` | 1 / 3 / 6 | celdas | RF-003 |
| `RUIDO_PUERTA_METALICA` | 10 | celdas | RF-006 |
| `T_INSTALACION` | 8 | s | RF-007 |
| `T_EXTRACCION` | 60 | s | RF-009 |
| `SALUD_MAX` / `CURA_MEDICAMENTO` | 100 / 35 | puntos | RF-011, RF-015 |
| `INTEGRIDAD_REFUGIO_MAX` | 100 | puntos | RF-012, RF-063 |
| `SENUELO_ALCANCE` / `SENUELO_RADIO` / `SENUELO_DURACION` / `SENUELO_PERIODO` | 8 / 8 / 10 / 1 | celdas / celdas / s / s | RF-013 |
| `VISION_RADIO` / `VISION_ANGULO` (explorador) | 7 / 110 | celdas / grados | RF-041 |
| `VISION_RADIO` / `VISION_ANGULO` (perseguidor) | 5 / 90 | celdas / grados | RF-041 |
| `MEMORIA_CAPACIDAD` | 16 | evidencias | RF-043 |
| `MEMORIA_SEMIVIDA` | 10 | s | RF-043 |
| `COM_RADIO` | 5 | celdas | RF-047 |
| `ALERTA_PROF_MAX` | 3 | saltos | RF-047 |
| `ALERTA_ATENUACION` | 0.7 | factor por salto | RF-047 |
| `MAX_AGENTES_POR_CELDA` | 2 | agentes | RF-051 |
| `FLOWFIELD_UMBRAL` | 8 | agentes con el mismo destino | RF-028 |
| `PRESUPUESTO_NODOS_TICK` | 4000 | nodos expandidos por tick de IA | RF-026 |
| `DANIO_ZOMBI` | 10 | puntos/s | RF-055 |
| `FACTOR_DESVIO_BARRICADA` | 2.0 | × costo original | RF-052 |

---

## 3. Requisitos de interfaces externas

### 3.1 Interfaz de usuario

| ID | Requisito |
|---|---|
| IU-01 | Resolución lógica 1280 × 720 escalada sin deformación a la ventana (letterbox). |
| IU-02 | HUD con salud, inventario, objetivo actual y temporizador de extracción. |
| IU-03 | Panel de monitoreo (raygui) acoplado a la derecha, 360 px de ancho, conmutable con F1. |
| IU-04 | Controles por defecto: WASD mover, Mayús correr, Ctrl sigilo, E interactuar, Q señuelo, B barricada, F1 panel, F2 algoritmo, F3 comparar, F4 aislado/coordinado, Espacio pausa, N paso, +/- velocidad, F5/F9 guardar/cargar. |

### 3.2 Interfaces de software

| ID | Interfaz | Formato |
|---|---|---|
| IS-01 | Mapas de Tiled | JSON (formato de mapa Tiled 1.10+), capas: `suelo`, `colision`, `costo`, `objetos`, `spawns`, `zonas`. |
| IS-02 | Configuración | `config/juego.json`, `config/ia.json`, `config/controles.json`. |
| IS-03 | Benchmark / optimización | Entrada JSON; salida CSV (UTF-8, separador `,`, cabecera de metadatos con `#`) y JSON. |
| IS-04 | Partidas guardadas | Binario versionado con cabecera mágica `REFG`, versión, longitud y CRC32. |
| IS-05 | Registros | Texto plano, una línea por evento, campos separados por `\t`. |

### 3.3 Interfaces de hardware y comunicaciones

* Teclado y ratón obligatorios; mando opcional (raylib Gamepad).
* **No hay interfaces de comunicaciones**: el software no abre sockets (RNF-022).

---

## 4. Requisitos funcionales

### F1 · Juego base

#### RF-001 · Cargar mapa de Tiled
> **Cuando** se inicie una escena, el sistema deberá cargar el mapa Tiled JSON indicado y construir las capas de suelo, colisión, costo, objetos, puntos de aparición y zonas.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | RES-03 | HU-01 | P · CP-001 | Alta | Estable | Bajo | Propuesto |

Justificación: el mapa es la fuente del grafo de navegación y de los elementos de cada escena.

#### RF-002 · Modos de movimiento de Mara
> **Mientras** la jugadora mantenga una dirección de movimiento, el sistema deberá desplazar a Mara a `VEL_SIGILO` si el modo es sigilo, `VEL_CORRER` si el modo es carrera y `VEL_CAMINAR` en otro caso.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Estado | M | STB Esc. 1 | HU-01 | P · CP-002 | Media | Estable | Bajo | Propuesto |

#### RF-003 · Ruido por movimiento
> **Mientras** Mara se desplace, el sistema deberá publicar cada 0,5 s un evento `Ruido` con origen en la celda de Mara y radio `RUIDO_SIGILO`, `RUIDO_CAMINAR` o `RUIDO_CORRER` según su modo.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Estado | M | STB §01 | HU-01 | P · CP-003 | Alta | Estable | Bajo | Propuesto |

Justificación: el ruido es el canal principal de percepción de los infectados.

#### RF-004 · Colisiones
> El sistema deberá impedir que Mara y los agentes ocupen celdas marcadas como no transitables en la capa de colisión o por barricadas.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | STB §01 | HU-01 | P · CP-004 | Media | Estable | Bajo | Propuesto |

#### RF-005 · Recoger objeto
> **Cuando** la jugadora pulse interactuar a 1 celda o menos de un objeto recogible, el sistema deberá añadir el objeto al inventario y retirarlo del mapa.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB §01 | HU-02 | P · CP-005 | Alta | Estable | Bajo | Propuesto |

#### RF-006 · Puerta metálica
> **Cuando** se abra una puerta metálica, el sistema deberá publicar un evento `Ruido` de radio `RUIDO_PUERTA_METALICA` con origen en la puerta.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB Esc. 2 | HU-02 | P · CP-006 | Media | Estable | Bajo | Propuesto |

#### RF-007 · Instalar batería y activar radio
> **Cuando** la jugadora mantenga interactuar junto a la radio durante `T_INSTALACION` con batería, combustible y al menos un medicamento en el inventario, el sistema deberá marcar la radio como activada y publicar el evento `RadioActivada`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB §01, Esc. 4 | HU-03 | P · CP-007 | Alta | Estable | Bajo | Propuesto |

#### RF-008 · Interrupción de la instalación
> **Si** Mara recibe daño o suelta interactuar durante la instalación, **entonces** el sistema deberá interrumpirla y reiniciar su progreso a 0.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| No deseado | M | STB Esc. 4 | HU-03 | P · CP-008 | Media | Media | Bajo | Propuesto |

#### RF-009 · Ventana de extracción
> **Cuando** se publique `RadioActivada`, el sistema deberá abrir la zona de extracción durante `T_EXTRACCION` segundos y mostrar un temporizador y la ruta de escape en el HUD.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB Esc. 5 | HU-04 | P · CP-009 | Alta | Estable | Bajo | Propuesto |

#### RF-010 · Victoria
> **Mientras** la zona de extracción esté abierta, **cuando** Mara entre en ella, el sistema deberá terminar la partida con resultado `Victoria`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Complejo | M | STB §01 | HU-04 | P · CP-010 | Alta | Estable | Bajo | Propuesto |

#### RF-011 · Derrota por salud
> **Si** la salud de Mara llega a 0, **entonces** el sistema deberá terminar la partida con resultado `Derrota: Mara ha caído`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| No deseado | M | STB §01 | HU-05 | P · CP-011 | Alta | Estable | Bajo | Propuesto |

#### RF-012 · Derrota por refugio
> **Si** la integridad del refugio llega a 0, **entonces** el sistema deberá terminar la partida con resultado `Derrota: refugio inutilizado`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| No deseado | M | STB §01 | HU-05 | P · CP-012 | Alta | Estable | Bajo | Propuesto |

#### RF-013 · Señuelo sonoro
> **Cuando** la jugadora lance un señuelo hacia una celda, el sistema deberá colocarlo en la celda transitable más lejana de la línea de lanzamiento a `SENUELO_ALCANCE` celdas o menos, y publicar un evento `Ruido` de radio `SENUELO_RADIO` cada `SENUELO_PERIODO` durante `SENUELO_DURACION`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB §03 Prueba de aceptación | HU-06 | P · CP-013 | Alta | Estable | Bajo | Propuesto |

#### RF-014 · Barricada
> **Cuando** la jugadora coloque una barricada en una celda adyacente transitable y libre de agentes, el sistema deberá marcarla como no transitable en el grafo con una durabilidad de 100 puntos y publicar el evento `CambioDeMapa` con la celda afectada.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB §03 Prueba de aceptación | HU-07 | P · CP-014 | Alta | Estable | Medio | Propuesto |

#### RF-015 · Usar medicamento
> **Cuando** la jugadora use un medicamento que no esté reservado para la radio, el sistema deberá sumar `CURA_MEDICAMENTO` a la salud sin superar `SALUD_MAX`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | S | Acuerdo del equipo | HU-02 | P · CP-015 | Baja | Media | Bajo | Propuesto |

#### RF-016 · HUD de objetivos
> El sistema deberá mostrar en todo momento la salud, el inventario, el siguiente objetivo de la misión y, si existe, el temporizador de extracción.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | STB §01 Experiencia | HU-01, HU-02 | D · CP-016 | Media | Media | Bajo | Propuesto |

#### RF-017 · Tutorial
> **Donde** el tutorial esté activado, el sistema deberá mostrar en la Escena 1 las indicaciones de mover, sigilo, ruido de carrera y recoger, en ese orden, y retirar cada una cuando la jugadora realice la acción.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Opcional | S | STB Esc. 1 | HU-20 | D · CP-017 | Baja | Media | Bajo | Propuesto |

---

### F2 · Navegación y búsqueda

#### RF-020 · Grafo de costos
> El sistema deberá representar cada mapa como un grafo G = (V, E, w) donde V son las celdas transitables, E conecta cada celda con sus 8 vecinas transitables sin cortar esquinas bloqueadas, y w(u,v) = d(u,v) · m(v) con d = 1 en ortogonal, d = √2 en diagonal y m(v) ≥ 1 el multiplicador de terreno de la capa `costo`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | GUIA §5 Grafos; STB §03 | HU-16 | A + P · CP-020 | Alta | Estable | Bajo | Propuesto |

Justificación: define vértices, aristas y costos no negativos exigidos por la guía. Con m ≥ 1 todos los pesos son ≥ 1 > 0, condición de corrección de Dijkstra. |V| ≤ W·H y |E| ≤ 8|V|.

#### RF-021 · Dijkstra
> **Cuando** un agente solicite una ruta con el algoritmo Dijkstra, el sistema deberá calcular el camino de costo mínimo usando una cola de prioridad de montículo binario sobre el grafo de RF-020.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §5 | HU-16 | P · CP-021 | Alta | Estable | Bajo | Propuesto |

Complejidad esperada: O((V + E) log V) en tiempo, O(V) en espacio (GUIA §5).

#### RF-022 · A\* con heurística octil
> **Cuando** un agente solicite una ruta con el algoritmo A\*, el sistema deberá calcularla con f(n) = g(n) + h(n), donde h es la distancia octil h(n) = (dx + dy) + (√2 − 2) · min(dx, dy) multiplicada por m_min = 1, sin reabrir nodos cerrados.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §5; STB §03 | HU-16 | A + P · CP-022 | Alta | Estable | Bajo | Propuesto |

Justificación de admisibilidad y consistencia: en una rejilla 8-conexa sin obstáculos y con m = 1, la distancia
octil es exactamente el costo mínimo; los obstáculos y m(v) ≥ 1 solo pueden aumentar el costo real, luego
h(n) ≤ h\*(n) (**admisible**). Para una arista (n, n') con costo w ≥ d(n, n') y |h(n) − h(n')| ≤ d(n, n') por la
desigualdad triangular de la métrica octil, se cumple h(n) ≤ w(n, n') + h(n') (**consistente**). Con h consistente,
la primera vez que un nodo se cierra su g es óptimo, por eso no es necesaria la reapertura.

#### RF-023 · Métricas de búsqueda
> **Cuando** termine una búsqueda, el sistema deberá devolver la ruta, su costo, el número de nodos expandidos, el número de nodos generados, el tiempo de búsqueda en microsegundos, el tamaño máximo de la lista abierta y la memoria pico de las estructuras de búsqueda en bytes.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §6, §7 | HU-14, HU-16 | P · CP-023 | Alta | Estable | Bajo | Propuesto |

#### RF-024 · Algoritmo intercambiable sobre el mismo grafo
> El sistema deberá ejecutar Dijkstra y A\* a través de la misma interfaz de búsqueda (`IBuscadorRuta`) sobre la misma instancia de grafo y la misma función de costos.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | STB §03 | HU-16 | I + P · CP-024 | Alta | Estable | Bajo | Propuesto |

#### RF-025 · Replanificación ante cambios del mapa
> **Cuando** se publique `CambioDeMapa`, el sistema deberá invalidar las rutas y flow fields que atraviesen las celdas afectadas y encolar una nueva búsqueda para cada agente afectado en el mismo tick de IA.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §4; STB §03 | HU-07, HU-11 | P · CP-025 | Alta | Estable | Medio | Propuesto |

Nota: el agente solo «sabe» del cambio si la celda está en su campo de percepción o si le llega por mensaje. Si no, lo descubre al chocar con la barricada (RF-052), lo que respeta la no omnisciencia.

#### RF-026 · Presupuesto de búsqueda por tick
> **Mientras** haya solicitudes de ruta en cola, el sistema deberá ejecutar las búsquedas de forma incremental (A\* iterativo con reanudación) sin superar `PRESUPUESTO_NODOS_TICK` nodos expandidos por tick de IA, atendiendo las solicitudes por prioridad y antigüedad.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Estado | M | NEG-07 | HU-23 | P · CP-026 | Alta | Estable | Medio | Propuesto |

#### RF-027 · Sin ruta
> **Si** la búsqueda agota la lista abierta sin alcanzar el destino, **entonces** el sistema deberá devolver el resultado `SinRuta` con sus métricas, y el agente deberá seleccionar otro comportamiento en el siguiente tick de IA.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| No deseado | M | GUIA §4 | HU-23 | P · CP-027 | Alta | Estable | Bajo | Propuesto |

#### RF-028 · Flow field compartido
> **Cuando** `FLOWFIELD_UMBRAL` agentes o más tengan el mismo destino, el sistema deberá calcular un único campo de flujo mediante Dijkstra inverso desde el destino y guiar a esos agentes con él en lugar de rutas individuales.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | S | NEG-07 | HU-23 | P · CP-028 | Media | Media | Medio | Propuesto |

#### RF-029 · Steering y boids
> El sistema deberá combinar para cada agente en movimiento las fuerzas de seguir ruta, separación, alineación, cohesión y evitación de obstáculos con pesos configurables, limitando velocidad y aceleración máximas.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | STB Esc. 3 | HU-10 | P · CP-029 | Media | Media | Medio | Propuesto |

#### RF-030 · Consulta espacial con quadtree
> El sistema deberá resolver las consultas de vecinos por radio (percepción, comunicación y separación) mediante un quadtree reconstruido o actualizado cada tick de simulación.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | NEG-07 | HU-10, HU-23 | P · CP-030 | Media | Estable | Bajo | Propuesto |

Complejidad: construcción O(n log n); consulta O(log n + k) promedio, con k vecinos devueltos. La inserción y la consulta son **recursivas** con profundidad máxima 8.

#### RF-031 · Costos de terreno
> El sistema deberá leer de la capa `costo` del mapa un multiplicador m(v) ∈ [1, 10] por celda, y rechazar el mapa si algún valor es menor que 1.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | GUIA §5 | HU-16 | P · CP-031 | Media | Estable | Bajo | Propuesto |

---

### F3 · Agentes infectados

#### RF-040 · Ciclo del agente
> El sistema deberá ejecutar para cada agente, en cada tick de IA (`TICK_IA_HZ`), las etapas percepción → actualización de memoria → planificación → decisión → ejecución → evaluación, en ese orden.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | GUIA §4; STB §03 | HU-08 | I + P · CP-040 | Alta | Estable | Bajo | Propuesto |

#### RF-041 · Percepción visual local sin omnisciencia
> El sistema deberá entregar a cada agente únicamente las entidades dentro de su cono de visión (`VISION_RADIO`, `VISION_ANGULO`) con línea de visión libre calculada con Bresenham, los eventos `Ruido` dentro de su radio y los mensajes recibidos; ningún agente deberá leer el estado del mundo por otro medio.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | STB §03 Percepción; Prueba de aceptación | HU-08 | I + P · CP-041 | Alta | Estable | Medio | Propuesto |

Verificación por inspección: el módulo de decisión solo recibe un objeto `Percepcion` construido por el puerto `IPercepcion`; no tiene acceso al registro ECS (ver arc42 §8.3).

#### RF-042 · Percepción auditiva
> **Cuando** se publique un evento `Ruido`, el sistema deberá notificarlo a cada agente cuya distancia al origen sea menor o igual al radio, restando 2 celdas de alcance por cada pared que cruce la línea entre el origen y el agente.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB §01 | HU-08, HU-09 | P · CP-042 | Alta | Estable | Bajo | Propuesto |

#### RF-043 · Memoria de evidencias
> El sistema deberá mantener por agente una memoria de capacidad `MEMORIA_CAPACIDAD` con evidencias (tipo, celda, tick, confianza, emisor) cuya confianza decae con semivida `MEMORIA_SEMIVIDA`, reemplazando la de menor confianza cuando esté llena.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | STB §03 Memoria; GUIA §5 | HU-08, HU-09, HU-11 | P · CP-043 | Alta | Estable | Bajo | Propuesto |

#### RF-044 · Máquina de estados del infectado
> El sistema deberá controlar a cada infectado con una máquina de estados con los estados `Explorar`, `Investigar`, `Perseguir`, `Interceptar`, `Buscar`, `Atacar` y `AtacarBarricada`, y registrar cada transición con su motivo.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | GUIA §4 | HU-08 | P · CP-044 | Alta | Estable | Bajo | Propuesto |

Tabla de transiciones (resumen):

| Desde | Condición | Hacia |
|---|---|---|
| Explorar | ruido o alerta con utilidad > umbral | Investigar |
| Explorar / Investigar | avistamiento de Mara | Perseguir o Interceptar (RF-054) |
| Investigar | llega al origen sin evidencia nueva | Buscar (8 s) → Explorar |
| Perseguir | pierde la visión | Buscar en la última posición conocida |
| Perseguir / Interceptar | distancia ≤ 1 | Atacar |
| cualquiera | ruta bloqueada sin alternativa (RF-052) | AtacarBarricada |

#### RF-045 · Exploración sin información global
> **Mientras** un infectado esté en `Explorar`, el sistema deberá elegir su siguiente destino entre las celdas frontera de su propio mapa de visitas, puntuando menos visitas y menor distancia, sin usar información de otros agentes ni del mundo.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Estado | M | STB Esc. 1 | HU-08 | P · CP-045 | Alta | Media | Medio | Propuesto |

Nota: el mapa de visitas es una rejilla de bits de baja resolución (bloques de 4 × 4 celdas) para limitar memoria (RNF-005).

#### RF-046 · Investigar ruido
> **Cuando** un infectado en `Explorar` perciba un ruido, el sistema deberá cambiarlo a `Investigar` y planificar una ruta a la celda de origen del ruido en el siguiente tick de IA.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB Esc. 2 | HU-09 | P · CP-046 | Alta | Estable | Bajo | Propuesto |

#### RF-047 · Propagación recursiva acotada de alertas
> **Cuando** un infectado registre una evidencia de Mara con confianza ≥ 0,6 y el modo coordinado esté activo, el sistema deberá propagar una alerta por el grafo de comunicación (agentes a `COM_RADIO` o menos) mediante una función recursiva con caso base «profundidad = `ALERTA_PROF_MAX`», un conjunto de visitados que impide reenviar a un agente ya alertado y una confianza multiplicada por `ALERTA_ATENUACION` en cada salto.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB §03 Recursividad; GUIA §5 | HU-09 | A + P · CP-047 | Alta | Estable | Medio | Propuesto |

Especificación de la recursión:

```text
propagar(agente a, alerta m, prof p, visitados S):
    si p == ALERTA_PROF_MAX: retornar                 // caso base 1: límite de profundidad
    nuevos ← ∅
    para cada vecino b en vecinosCom(a):               // consulta al quadtree
        si b ∈ S: continuar                            // caso base 2: ya visitado
        S ← S ∪ {b}; nuevos ← nuevos ∪ {b}
        entregar(b, m con confianza · ALERTA_ATENUACION^(p+1), salto p+1)
    para cada b en nuevos:
        propagar(b, m, p + 1, S)                       // progreso: p crece estrictamente
```

Se marcan primero todos los vecinos directos y después se recurre: así cada agente recibe la alerta con el
menor salto posible desde quien lo alertó, y se evita que un recorrido en profundidad puro llegue a un
vecino cercano por un camino largo y corte su propagación. Implementación: `src/dominio/coordinacion/PropagadorAlertas.cpp`.

* **Progreso**: p aumenta en 1 en cada llamada y está acotado por `ALERTA_PROF_MAX`; además |S| crece y está acotado por el número de agentes n, luego la recursión termina.
* **Tiempo**: O(n + Σ vecinos) ≤ O(n · k) con k vecinos promedio por consulta.
* **Pila**: profundidad ≤ `ALERTA_PROF_MAX` + 1 marcos, O(D) en espacio de pila; S usa O(n) bits (bitset).

#### RF-048 · Roles de infectado
> El sistema deberá crear infectados con el rol `Explorador` (visión amplia, velocidad 2 celdas/s, emite alertas) o `Perseguidor` (visión estrecha, velocidad 4 celdas/s), configurados por su prototipo en `config/ia.json`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | STB §01 Personajes | HU-08 | P · CP-048 | Media | Estable | Bajo | Propuesto |

#### RF-049 · Coordinación con Contract Net
> **Cuando** un infectado detecte a Mara en modo coordinado, el sistema deberá actuar como gestor anunciando las tareas `Perseguir`, `Interceptar(punto)` y `Flanquear(acceso)` a los agentes alertados, recibir sus pujas (costo estimado de ruta por heurística octil) durante 1 tick de IA y adjudicar cada tarea a la puja de menor costo, una tarea por agente.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §5 Comunicación; STB Esc. 3, 5 | HU-10, HU-12 | P · CP-049 | Alta | Media | Alto | Propuesto |

#### RF-050 · Reevaluación ante señuelos
> **Cuando** un infectado perciba un ruido de señuelo, el sistema deberá recalcular la utilidad de su objetivo actual y del señuelo y, si la del señuelo es mayor, cambiar su objetivo en 5 ticks de IA (500 ms) o menos y registrar un `CambioDePlan` con ambas utilidades.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB Esc. 4; Prueba de aceptación | HU-11 | P · CP-050 | Alta | Estable | Medio | Propuesto |

Utilidad de un objetivo: U = c · e^(−edad/τ) · 1/(1 + dist/10), con c la confianza, τ = `MEMORIA_SEMIVIDA`/ln 2 y dist en celdas.

#### RF-051 · Distribución de la aproximación
> **Cuando** varios infectados tengan el mismo objetivo, el sistema deberá asignarles celdas de llegada distintas entre los accesos libres alrededor del objetivo, con `MAX_AGENTES_POR_CELDA` como máximo por celda, minimizando la suma de distancias.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB Esc. 3 | HU-10 | P · CP-051 | Alta | Media | Medio | Propuesto |

La asignación se resuelve con un algoritmo voraz por costo (O(n · s log s), s = número de accesos), con opción de Húngaro O(n³) para n ≤ 16.

#### RF-052 · Respuesta a barricadas
> **Si** la ruta de un infectado queda bloqueada por una barricada percibida, **entonces** el sistema deberá replanificar y, si el costo de la nueva ruta supera `FACTOR_DESVIO_BARRICADA` veces el original o no existe ruta, cambiar a `AtacarBarricada` reduciendo su durabilidad 10 puntos/s.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| No deseado | M | STB §03 Prueba de aceptación | HU-07, HU-11 | P · CP-052 | Alta | Estable | Medio | Propuesto |

#### RF-053 · Interceptación
> **Cuando** un infectado tenga la tarea `Interceptar`, el sistema deberá estimar la posición futura de Mara a partir de sus dos últimas evidencias (velocidad y rumbo) y elegir como destino el nodo de su ruta prevista al que el infectado llegue antes que ella.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB Esc. 5 | HU-12 | P · CP-053 | Media | Media | Medio | Propuesto |

#### RF-054 · Selección de táctica
> **Cuando** un infectado reevalúe su situación, el sistema deberá elegir entre `Perseguir`, `Interceptar` y `Explorar` según la utilidad de cada táctica calculada con la confianza, edad y rumbo de sus evidencias, y registrar la táctica y su puntuación.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB Esc. 5 | HU-12 | P · CP-054 | Alta | Media | Medio | Propuesto |

#### RF-055 · Ataque
> **Mientras** un infectado en `Atacar` esté a 1 celda o menos de Mara, el sistema deberá restarle `DANIO_ZOMBI` puntos de salud por segundo.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Estado | M | STB §01 | HU-05 | P · CP-055 | Media | Estable | Bajo | Propuesto |

#### RF-056 · Modo aislado o coordinado
> **Donde** el modo `aislado` esté activado, el sistema deberá desactivar la propagación de alertas (RF-047) y el Contract Net (RF-049), conservando idénticos el resto de comportamientos y la secuencia aleatoria.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Opcional | M | STB §03 | HU-18 | P · CP-056 | Alta | Estable | Bajo | Propuesto |

---

### F4 · Superviviente de la radio

#### RF-060 · Selección de meta por utilidad
> **Cuando** el superviviente reevalúe sus metas (cada tick de IA), el sistema deberá puntuar las metas `RefugioSeguro`, `AyudarInstalacion`, `PonerseASalvo` y `Vigilar` con curvas de utilidad normalizadas en [0, 1] y elegir la de mayor puntuación, con una histéresis de 0,1 para evitar oscilaciones.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | S | STB §01 Personajes | HU-13 | P · CP-060 | Media | Media | Medio | Propuesto |

#### RF-061 · Planificación GOAP
> **Cuando** cambie la meta del superviviente, el sistema deberá generar con GOAP la secuencia de acciones de menor costo que satisfaga la meta a partir del estado actual, con búsqueda A\* regresiva y profundidad máxima de 6 acciones.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | S | GUIA §4 Planificación | HU-13 | P · CP-061 | Media | Media | Alto | Propuesto |

#### RF-062 · Ejecución con árbol de comportamiento
> El sistema deberá ejecutar cada acción del plan del superviviente mediante un árbol de comportamiento con nodos secuencia, selector, decorador y hoja, que devuelven `Exito`, `Fallo` o `EnCurso`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | S | GUIA §4 Ejecución | HU-13 | P · CP-062 | Media | Estable | Bajo | Propuesto |

#### RF-063 · Integridad del refugio
> **Mientras** un infectado golpee la puerta del refugio, el sistema deberá restar 5 puntos de integridad por segundo, y **cuando** el superviviente repare, sumar 8 puntos por segundo sin superar `INTEGRIDAD_REFUGIO_MAX`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Complejo | M | STB §01 | HU-05, HU-13 | P · CP-063 | Alta | Media | Bajo | Propuesto |

#### RF-064 · Replanificación del superviviente
> **Si** una acción del plan devuelve `Fallo` o sus precondiciones dejan de cumplirse, **entonces** el sistema deberá descartar el plan y generar uno nuevo en el siguiente tick de IA, registrando el motivo.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| No deseado | S | GUIA §4 | HU-13 | P · CP-064 | Media | Estable | Bajo | Propuesto |

---

### F5 · Monitoreo

#### RF-070 · Panel del agente seleccionado
> **Mientras** el panel esté visible y haya un agente seleccionado, el sistema deberá mostrar su objetivo, estado, percepción relevante (entidades vistas, ruidos oídos, mensajes recibidos), ruta, algoritmo, heurística, costo, nodos expandidos y tiempo de búsqueda, actualizados en cada tick de IA.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Estado | M | GUIA §6; STB §03 | HU-14 | D + P · CP-070 | Alta | Estable | Bajo | Propuesto |

#### RF-071 · Superposiciones de depuración
> **Donde** se activen las superposiciones, el sistema deberá dibujar sobre el mapa la ruta, los nodos expandidos y en frontera de la última búsqueda, el flow field, los conos de visión, los radios de ruido y las aristas de comunicación de las alertas.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Opcional | M | GUIA §6 | HU-14 | D · CP-071 | Media | Media | Bajo | Propuesto |

#### RF-072 · Registro de cambios de plan
> **Cuando** un agente cambie de estado, objetivo o plan, el sistema deberá registrar el tick, el id del agente, el plan anterior, el plan nuevo y el motivo, en el panel y en `logs/ia_<semilla>.log`.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §6 | HU-11, HU-15 | P · CP-072 | Alta | Estable | Bajo | Propuesto |

#### RF-073 · Registro de mensajes entre agentes
> **Cuando** un agente envíe un mensaje (alerta, anuncio, puja o adjudicación), el sistema deberá registrar el tick, el emisor, el receptor, el tipo, el salto y la confianza.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §6 | HU-09, HU-15 | P · CP-073 | Alta | Estable | Bajo | Propuesto |

#### RF-074 · Comparación de algoritmos en vivo
> **Cuando** el docente pulse F3 con un agente seleccionado, el sistema deberá ejecutar su última consulta con Dijkstra y con A\* sobre el mismo grafo y mostrar una tabla con costo, nodos expandidos, tiempo y memoria de cada uno; **cuando** pulse F2, deberá alternar el algoritmo activo de todos los agentes.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §6 «Comparar Dijkstra y A\* con datos reales» | HU-16 | D + P · CP-074 | Alta | Estable | Bajo | Propuesto |

#### RF-075 · Control de la simulación
> El sistema deberá permitir pausar, avanzar exactamente un tick y ejecutar a velocidades 0,25×, 0,5×, 1×, 2× y 4× sin alterar el resultado determinista de la simulación.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | S | GUIA §8 Demostración | HU-22 | P · CP-075 | Media | Estable | Bajo | Propuesto |

---

### F6 · Experimentación y optimización

#### RF-080 · Benchmark sin ventana
> **Cuando** se ejecute `refugio --bench <config>`, el sistema deberá, sin abrir ventana, ejecutar Dijkstra y A\* sobre los mismos mapas, orígenes, destinos y costos para cada tamaño configurado (al menos 32², 64², 128² y 256²), con al menos 5 repeticiones por condición, y escribir un CSV con una fila por medición.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §7; STB §03 | HU-17 | P · CP-080 | Alta | Estable | Bajo | Propuesto |

Columnas mínimas: `algoritmo, mapa, ancho, alto, densidad_obstaculos, semilla, repeticion, origen_x, origen_y, destino_x, destino_y, costo, longitud, nodos_expandidos, nodos_generados, max_abierta, tiempo_us, memoria_pico_bytes, replanificaciones`.

#### RF-081 · Metadatos del experimento
> **Cuando** se genere un archivo de resultados, el sistema deberá escribir en su cabecera el modelo de CPU, núcleos, RAM, sistema operativo, compilador y opciones, versión y commit del juego, fecha, semilla maestra y archivo de configuración usado.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | GUIA §7 | HU-17 | I + P · CP-081 | Alta | Estable | Bajo | Propuesto |

#### RF-082 · Verificación de optimalidad
> **Cuando** termine el benchmark, el sistema deberá comparar el costo de A\* con el de Dijkstra en cada consulta y reportar como error toda diferencia mayor que 1e-6.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB §03 «comparar optimalidad» | HU-17 | P · CP-082 | Alta | Estable | Bajo | Propuesto |

#### RF-083 · Escenarios dinámicos
> **Donde** el escenario de benchmark defina eventos de cambio (barricadas o costos que cambian en un tick dado), el sistema deberá aplicarlos durante la ejecución y registrar las replanificaciones, el costo final y los nodos totales de cada algoritmo.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Opcional | M | GUIA §7 «Probar cambios del entorno» | HU-17 | P · CP-083 | Media | Media | Medio | Propuesto |

#### RF-084 · Experimento aislados frente a coordinados
> **Cuando** se ejecute `refugio --coord-exp <semilla> --reps <n>`, el sistema deberá simular sin ventana n partidas en modo aislado y n en modo coordinado con las mismas semillas y un guion de jugadora fijo, y registrar por simulación el tiempo hasta la primera detección, el tiempo hasta el contacto, la cobertura explorada, los mensajes enviados y las celdas sobrecargadas.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | M | STB §03 | HU-18 | P · CP-084 | Alta | Estable | Medio | Propuesto |

#### RF-085 · Optimización de parámetros
> **Cuando** se ejecute `refugio --optimize ga|sa --config <archivo>`, el sistema deberá optimizar los pesos de utilidad y steering con un algoritmo genético (selección por torneo, cruce uniforme, mutación gaussiana, elitismo) o con recocido simulado (enfriamiento geométrico), evaluando la aptitud con simulaciones sin ventana, y escribir el mejor conjunto de parámetros y la evolución de la aptitud en JSON.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | S | Objetivo general «optimización» | HU-21 | P · CP-085 | Media | Media | Medio | Propuesto |

#### RF-086 · Aleatoriedad con semilla
> El sistema deberá derivar todo valor aleatorio de un generador PCG32 inicializado con una semilla maestra configurable y registrada, con un flujo independiente por subsistema.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | M | GUIA §7; STB §03 | HU-17 | P · CP-086 | Alta | Estable | Bajo | Propuesto |

---

### F7 · Persistencia y configuración

#### RF-090 · Guardar partida
> **Cuando** la jugadora guarde en una ranura, el sistema deberá serializar el estado completo de la simulación y del generador aleatorio a `saves/slotN.sav` escribiendo primero un archivo temporal y renombrándolo después.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | S | Acuerdo del equipo | HU-19 | P · CP-090 | Media | Estable | Bajo | Propuesto |

#### RF-091 · Cargar partida
> **Cuando** la jugadora cargue una ranura válida, el sistema deberá restaurar el estado de modo que el hash del estado coincida con el del momento del guardado.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Evento | S | Acuerdo del equipo | HU-19 | P · CP-091 | Media | Estable | Bajo | Propuesto |

#### RF-092 · Configuración local
> El sistema deberá leer al iniciar `config/juego.json`, `config/ia.json` y `config/controles.json`, y guardar en ellos los cambios hechos desde el menú de opciones.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| Ubicuo | S | Acuerdo del equipo | HU-19 | P · CP-092 | Baja | Estable | Bajo | Propuesto |

#### RF-093 · Archivos corruptos o inválidos
> **Si** un archivo de partida, configuración o mapa no supera la validación (cabecera, CRC32, esquema o rangos), **entonces** el sistema deberá mostrar o registrar un mensaje de error descriptivo, usar los valores por defecto cuando existan y continuar sin cerrarse.

| Tipo | Prio. | Fuente | HU | Verif. | Crit. | Estab. | Riesgo | Estado |
|---|---|---|---|---|---|---|---|---|
| No deseado | M | RNF-016 | HU-19 | P · CP-093 | Alta | Estable | Bajo | Propuesto |

---

## 5. Requisitos de datos (modelo lógico)

| Entidad | Atributos principales | Persistencia |
|---|---|---|
| Mapa | ancho, alto, tiles[], colision bitset, costo[] (uint8), objetos, zonas | Tiled JSON (lectura) |
| Agente | id, rol, posición, velocidad, estado FSM, objetivo, ruta, memoria[16], visitas bitset | Partida |
| Evidencia | tipo {Avistamiento, Ruido, Alerta, Señuelo, Barricada}, celda, tick, confianza (float), emisor | Partida |
| Mensaje | tipo {Alerta, Anuncio, Puja, Adjudicacion}, emisor, receptor, salto, confianza, carga | Registro |
| ResultadoBusqueda | algoritmo, ruta, costo, expandidos, generados, maxAbierta, tiempo_us, memoria_bytes | CSV / panel |
| Partida | versión, semilla, tick, estado PRNG, entidades, inventario, integridad, temporizadores | `.sav` |

## 6. Verificación

* Todos los RF tienen al menos un caso de prueba (ver [05_casos_prueba.md](05_casos_prueba.md)).
* Los RF de tipo «A» (análisis) requieren además una sección en el documento de análisis de complejidad.
* La cobertura RF → CP se comprueba en la [matriz de trazabilidad](06_matriz_trazabilidad.md).

## 7. Resumen de requisitos funcionales

| Grupo | Total | M | S | C |
|---|---|---|---|---|
| F1 Juego base | 17 | 15 | 2 | 0 |
| F2 Navegación | 12 | 11 | 1 | 0 |
| F3 Infectados | 17 | 17 | 0 | 0 |
| F4 Superviviente | 5 | 1 | 4 | 0 |
| F5 Monitoreo | 6 | 5 | 1 | 0 |
| F6 Experimentación | 7 | 6 | 1 | 0 |
| F7 Persistencia | 4 | 1 | 3 | 0 |
| **Total** | **68** | **56** | **12** | **0** |
