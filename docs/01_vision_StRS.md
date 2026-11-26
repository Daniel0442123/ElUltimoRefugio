# 01 · Visión y Especificación de Requisitos de las Partes Interesadas (StRS)

> Estructura basada en ISO/IEC/IEEE 29148:2018, cláusula 9.3 (contenido del StRS), en versión ligera.
> Compatible con la plantilla de la edición 2011 (IEEE 29148-2011, Anexo A).

| Campo | Valor |
|---|---|
| Documento | StRS-REFUGIO |
| Versión | 0.2.0 (estado real del software 0.16.0) |
| Fecha | 2026-11-25 |
| Estado | Línea base revisada contra la implementación |
| Fuentes | `00_guia_proyecto_final.pdf` (en adelante **GUIA**), `02_refugio.pdf` (en adelante **STB**) |

---

## 1. Introducción

### 1.1 Propósito del negocio

El curso de Algoritmos Computacionales exige un proyecto final basado en retos en el que se
**diseñe, implemente y valide** un videojuego cuyos personajes autónomos perciban su entorno,
planifiquen, decidan, ejecuten y adapten su comportamiento (GUIA §1). Lo que se evalúa es la
solución algorítmica y su validación, no la escritura manual del código (GUIA §1).

### 1.2 Alcance del negocio

**El último refugio** es un prototipo 2D de vista cenital. Una epidemia ha dejado una ciudad
incomunicada. Mara, la jugadora, debe reunir **medicamentos, combustible y una batería** para activar
la radio de un refugio y llegar al punto de extracción. Los infectados detectan sonidos y comparten
señales locales, de modo que una huida ruidosa puede atraer a una horda (STB §01).

Dentro del alcance:

* El juego jugable de cinco escenas (STB §02) con victoria y derrota. *(0.16.0: una sola partida continua en una
  ciudad generada con semilla, donde aparecen los comportamientos de las cinco escenas; ver §8.)*
* Agentes autónomos: infectados exploradores, perseguidores y el superviviente de la radio.
* Navegación con **Dijkstra** y **A\*** sobre el mismo grafo, más técnicas de apoyo (flow fields, steering).
* Un procedimiento **recursivo** acotado: propagación de alertas con conjunto de visitados (STB §03).
* Coordinación multiagente y comparación de agentes **aislados frente a coordinados** con la misma semilla (STB §03).
* **Panel de monitoreo** con objetivo, estado, percepción, ruta, algoritmo, heurística, costo, nodos expandidos y tiempo (GUIA §6).
* **Experimentación** reproducible con varios tamaños de mapa, al menos cinco repeticiones y semillas registradas (GUIA §7).
* Documentación de agentes, grafos, heurísticas, recursividad y análisis de complejidad (GUIA §8).

Fuera del alcance (versión 1.0):

* Arte comercial, audio profesional, multijugador, servicios en red o en la nube, tiendas o cuentas de usuario.
* Aprendizaje automático en línea. La guía aclara que la autonomía no lo exige (GUIA §4).
* Un modelo de lenguaje dentro del juego (opcional según GUIA §7; se registra como posible extensión, ver §6.3).

### 1.3 Visión del producto

> **Para** el docente y los evaluadores del curso, **que** necesitan comprobar que el equipo domina
> algoritmos de búsqueda, recursividad, heurísticas y arquitecturas de agentes, **El último refugio** es
> un videojuego 2D ligero **que** hace visibles y medibles las decisiones autónomas de sus agentes en un
> entorno que cambia. **A diferencia de** un juego convencional con IA guionizada, **nuestro producto**
> expone en un panel cada percepción, plan, ruta y mensaje, y permite comparar Dijkstra frente a A\* y
> agentes aislados frente a coordinados con datos reales y reproducibles, todo en equipos de pocos recursos.

### 1.4 Contexto del negocio

* Proyecto académico, equipo pequeño, presupuesto nulo y herramientas libres.
* Se permite usar IA generativa y programación agéntica durante todo el desarrollo (GUIA §1, §7).
* Se entrega: juego funcional con instrucciones, repositorio, documentación, análisis teórico y
  experimental, registros de pruebas, demostración en vivo y sustentación (GUIA §8).
* Criterios de evaluación propuestos por el docente (GUIA §9):

| Criterio | Peso | Dónde se atiende |
|---|---|---|
| Arquitectura de agentes inteligentes | 25 % | NEG-01, EP-03, EP-04, arc42 §5 y §8 |
| Implementación de algoritmos | 25 % | NEG-02, EP-02, RF-020 a RF-031 |
| Autonomía y adaptación | 20 % | NEG-03, EP-03, RF-040 a RF-056 |
| Complejidad y rendimiento | 15 % | NEG-04, EP-06, RNF-001 a RNF-008 |
| Funcionalidad y experiencia de juego | 5 % | NEG-05, EP-01 |
| Sustentación y documentación | 10 % | NEG-06, este conjunto de documentos |

### 1.5 Definiciones, siglas y abreviaturas

| Término | Definición |
|---|---|
| Agente | Entidad que percibe, recuerda, planifica, decide, ejecuta y evalúa de forma autónoma (GUIA §4). |
| Mara | Personaje controlado por la jugadora. |
| Infectado explorador | Agente zombi de percepción amplia que explora y difunde alertas. |
| Infectado perseguidor | Agente zombi rápido que persigue o intercepta. |
| Superviviente de la radio | Agente aliado que mantiene el refugio. |
| Evidencia | Registro en memoria de una observación (ruido, avistamiento, alerta) con posición, tiempo y confianza. |
| Señuelo sonoro | Objeto lanzado por Mara que emite ruido para atraer agentes. |
| Barricada | Obstáculo que bloquea o encarece una celda del grafo. |
| Omnisciencia | Acceso de un agente a información del mundo que no obtuvo por percepción o mensajes. Está prohibida. |
| Grafo de costos | G = (V, E, w): V celdas transitables, E adyacencias, w ≥ 0 costo de moverse. |
| Nodos expandidos | Número de vértices extraídos de la cola de prioridad y procesados por la búsqueda. |
| Heurística admisible | h(n) ≤ costo real mínimo de n al objetivo. |
| Heurística consistente | h(n) ≤ w(n, n') + h(n') para toda arista. |
| Tick | Paso fijo de simulación. |
| Semilla | Valor inicial del generador pseudoaleatorio que hace reproducible una ejecución. |
| StRS / SRS | Especificación de requisitos de partes interesadas / del software. |
| EARS | Easy Approach to Requirements Syntax. |
| ECS | Entity-Component-System. |

### 1.6 Partes interesadas

| ID | Parte interesada | Interés principal | Influencia |
|---|---|---|---|
| STK-01 | Docente del curso | Que se cumplan los requisitos algorítmicos y se demuestre con datos reales. | Alta (define y evalúa) |
| STK-02 | Jurado / evaluadores de la sustentación | Entender rápido las decisiones de los agentes y la arquitectura. | Alta |
| STK-03 | Equipo de desarrollo | Alcance realista, código mantenible, pruebas automatizadas y buena nota. | Alta |
| STK-04 | Jugador de prueba | Un juego comprensible y con tensión, que corra en su equipo. | Media |
| STK-05 | Equipo con pocos recursos (persona que ejecuta en un portátil modesto) | Bajo consumo de CPU, memoria y disco, sin instalar servicios. | Media |

---

## 2. Referencias

| ID | Documento |
|---|---|
| REF-01 | Fundación Universitaria de Popayán. *Guía general · Proyecto final: Videojuegos con inteligencia artificial agéntica* (00_guia_proyecto_final.pdf). |
| REF-02 | Fundación Universitaria de Popayán. *Storyboard · Apocalipsis zombi: El último refugio* (02_refugio.pdf). |
| REF-03 | ISO/IEC/IEEE 29148:2018. *Systems and software engineering — Life cycle processes — Requirements engineering*. |
| REF-04 | ISO/IEC/IEEE 29148:2011 e IEEE 29148-2011 (edición anterior, referencia). |
| REF-05 | ISO/IEC 25010:2023. *Product quality model* (y edición 2011 como referencia de nombres). |
| REF-06 | Mavin, A. et al. *Easy Approach to Requirements Syntax (EARS)*, IEEE RE 2009. |
| REF-07 | Starke, G. *arc42 Template*, v8. · Brown, S. *The C4 model for visualising software architecture*. |
| REF-08 | Hart, Nilsson, Raphael (1968), A\*. · Dijkstra (1959). · Smith (1980), Contract Net Protocol. |

---

## 3. Requisitos de negocio

### 3.1 Objetivos de negocio (NEG)

| ID | Objetivo | Métrica de éxito | Fuente |
|---|---|---|---|
| NEG-01 | Demostrar una arquitectura de agentes completa: entorno → percepción → memoria → planificación → decisión → ejecución → evaluación → actualización. | Cada agente expone las 7 etapas en el panel y en el registro; revisión del docente sin observaciones mayores. | GUIA §4 |
| NEG-02 | Implementar y comparar Dijkstra y A\* sobre el mismo grafo de costos no negativos, con heurística admisible justificada. | 100 % de consultas del benchmark con costo A\* = costo Dijkstra; informe con ≥ 4 tamaños × ≥ 5 repeticiones. | GUIA §5, §7; STB §03 |
| NEG-03 | Mostrar autonomía y adaptación ante cambios del entorno sin omnisciencia. | Prueba de aceptación del señuelo y la barricada superada (CP-050, CP-052, CP-201). | STB §03 |
| NEG-04 | Analizar complejidad temporal y espacial, incluida la pila de la recursión, y medir rendimiento real. | Sección de análisis con O() por algoritmo y tablas experimentales con semilla, equipo, tiempo, costo, nodos y memoria. | GUIA §5, §7 |
| NEG-05 | Entregar un juego funcional y jugable de principio a fin. | Partida completa (5 escenas) ganable y perdible; instrucciones de ejecución. | GUIA §8; STB §01 |
| NEG-06 | Entregar documentación y sustentación de calidad. | Documentos de este directorio aprobados; demo en vivo ≤ 15 min con cambio de comportamiento visible. | GUIA §8, §9 |
| NEG-07 | Ejecutar en equipos de pocos recursos de forma local, sin red. | Cumple RNF-001 a RNF-008 en el equipo de referencia mínimo. | Acuerdo con STK-03/STK-05 |

### 3.2 Modelo de negocio y entorno operativo

* Producto de uso académico, sin distribución comercial.
* Se ejecuta en el equipo del evaluador o del equipo, de forma local, desde un `.zip` portable.
* La persistencia es solo en archivos locales (configuración, partidas, registros y resultados de experimentos).

### 3.3 Restricciones de negocio y de diseño

| ID | Restricción | Origen |
|---|---|---|
| RES-01 | Lenguaje **C++20** (compatible con C++17 en el dominio), compilado con CMake. | Acuerdo del equipo (ADR-0002) |
| RES-02 | Biblioteca gráfica **raylib 5** y **raygui** para la interfaz. *(0.16.0: raylib 5.5; raygui no se usa, ADR-0020.)* | ADR-0002 |
| RES-03 | Mapas creados en **Tiled** y exportados a JSON, leídos con **nlohmann/json**. *(0.16.0: sustituida; mapas generados por código y lector JSON propio, ADR-0019.)* | ADR-0013 |
| RES-04 | Pixel art de **32 × 32 px** por tile. *(0.16.0: sprites de 16 × 16 px dibujados al doble en celdas de 32 px, generados por código, ADR-0017.)* | Acuerdo del equipo |
| RES-05 | Sin backend, sin nube y sin conexiones de red. | NEG-07 |
| RES-06 | El dominio de IA no puede depender de raylib (arquitectura hexagonal). | ADR-0003 |
| RES-07 | Debe existir al menos un procedimiento recursivo con caso base, progreso y límite. | GUIA §5; STB §03 |
| RES-08 | Dijkstra y A\* deben ejecutarse sobre **el mismo grafo** con costos **no negativos**. | STB §03 |
| RES-09 | El código y los datos de experimentos deben ser reproducibles (semillas registradas). | GUIA §7; STB §03 |
| RES-10 | Herramientas libres: Catch2, Tracy, ASan/UBSan, Valgrind, Doxygen, PlantUML, Git y GitHub Actions. *(0.16.0: se usan Catch2, ASan/UBSan, PlantUML, Git y GitHub Actions; Tracy, Valgrind y Doxygen no, ADR-0020.)* | Acuerdo del equipo |

---

## 4. Descripción del sistema en su contexto

### 4.1 Interfaces del sistema

* **Jugador**: teclado y ratón (mando opcional), pantalla 1280 × 720 o mayor.
* **Docente / evaluador**: panel de monitoreo dentro del juego y archivos CSV / log en disco.
* **Equipo**: línea de comandos (`refugio --bench`, `refugio --optimize`), archivos de configuración JSON, mapas Tiled.
* **Sistema operativo**: sistema de archivos local, OpenGL 3.3 mediante raylib.

### 4.2 Modos de operación

| Modo | Descripción | Usuarios |
|---|---|---|
| Juego | Partida normal con las 5 escenas. | STK-04 |
| Juego + monitoreo | Partida con el panel y las superposiciones de depuración activas. | STK-01, STK-02 |
| Comparación en vivo | Alterna Dijkstra/A\* y aislado/coordinado durante la partida. | STK-01, STK-02 |
| Benchmark (sin ventana) | Ejecuta experimentos y escribe CSV. | STK-03, STK-01 |
| Optimización (sin ventana) | Ajusta parámetros con algoritmo genético o recocido simulado. | STK-03 |

### 4.3 Perfil de usuarios

| Usuario | Conocimientos | Frecuencia | Necesidad |
|---|---|---|---|
| Jugador | Juegos 2D básicos | Ocasional | Controles claros y tutorial corto. |
| Docente | Experto en algoritmos | Evaluación y sustentación | Ver datos internos sin leer el código. |
| Desarrollador | C++ intermedio | Diaria | Compilar, probar y perfilar con un comando. |

### 4.4 Storyboard operativo (escenarios de uso)

Tomado de STB §02 y refinado. Cada escena se convierte en una o más historias de usuario. En 0.16.0 las escenas
no son niveles separados sino momentos de una misma partida; la última columna dice dónde se ve cada una.

| Escena | Acción del jugador | Comportamiento inteligente esperado | HU | En 0.16.0 |
|---|---|---|---|---|
| 1 · Ciudad silenciosa | Tutorial de desplazamiento silencioso por calles desiertas. | Los infectados exploran sin información global. | HU-01, HU-08, HU-20 | Inicio de la partida: tutorial y horda explorando con su mapa de visitas. |
| 2 · La farmacia | Recoge medicamentos; una puerta metálica genera ruido. | Los agentes cercanos investigan el origen y transmiten alertas. | HU-02, HU-09 | Al recoger objetos. No hay puerta metálica: el ruido de Mara o de un señuelo dispara la investigación y las alertas. |
| 3 · La horda | Cruza calles con obstáculos y grupos de infectados. | Los zombis distribuyen aproximaciones y evitan concentrarse en una celda. | HU-10 | Cuando varios infectados la persiguen: Contract Net, reparto de accesos y campo de flujo. |
| 4 · El refugio | Instala la batería mientras los infectados se aproximan. | La horda reconsidera objetivos ante señuelos sonoros. | HU-03, HU-06, HU-07, HU-11, HU-13 | En el refugio: instalar hace ruido, el superviviente repara y ayuda, señuelos y barricadas. |
| 5 · Extracción | Una señal abre una ruta de escape temporal. | Los agentes eligen perseguir, explorar o interceptar según evidencias. | HU-04, HU-12 | Tras activar la radio: 60 s para llegar a la extracción; los infectados eligen táctica. |

---

## 5. Requisitos de las partes interesadas (StR)

| ID | Requisito de parte interesada | Parte | Prioridad | Fuente | Se refina en |
|---|---|---|---|---|---|
| StR-01 | El jugador necesita controlar a Mara con movimiento normal, carrera y sigilo, donde cada modo produce distinto ruido. | STK-04 | M | STB §01, Esc. 1 | HU-01 |
| StR-02 | El jugador necesita recoger medicamentos, combustible y batería y llevarlos al refugio para activar la radio. | STK-04 | M | STB §01 | HU-02, HU-03 |
| StR-03 | El jugador necesita ganar al activar la radio y llegar a la extracción, y perder si su salud o la integridad del refugio llegan a cero. | STK-04 | M | STB §01 | HU-04, HU-05 |
| StR-04 | El jugador necesita herramientas para influir en los agentes: señuelo sonoro y barricada. | STK-04, STK-01 | M | STB §03 Prueba de aceptación | HU-06, HU-07 |
| StR-05 | El docente necesita que los agentes solo usen observaciones locales y eventos permitidos, sin omnisciencia. | STK-01 | M | STB §03; GUIA §4 | HU-08, RF-041 |
| StR-06 | El docente necesita que los agentes recuerden evidencias, objetivos y resultados, y replanifiquen ante cambios. | STK-01 | M | GUIA §4; STB §03 | HU-09, HU-11 |
| StR-07 | El docente necesita comunicación multiagente con propagación **recursiva** acotada de alertas y conjunto de visitados. | STK-01 | M | STB §03 | HU-09 |
| StR-08 | El docente necesita ver coordinación: distribución de aproximaciones y elección entre perseguir, explorar o interceptar. | STK-01 | M | STB Esc. 3 y 5 | HU-10, HU-12 |
| StR-09 | El docente necesita Dijkstra y A\* sobre el mismo grafo, con heurística admisible justificada. | STK-01 | M | GUIA §5; STB §03 | HU-16, RF-021, RF-022 |
| StR-10 | El docente necesita un panel con objetivo, estado, percepción relevante, ruta, algoritmo, heurística, costo, nodos expandidos y tiempo. | STK-01, STK-02 | M | GUIA §6; STB §03 | HU-14 |
| StR-11 | El docente necesita un registro de cambios de plan y de mensajes entre agentes. | STK-01 | M | GUIA §6 | HU-15 |
| StR-12 | El docente necesita experimentos reproducibles: mismos mapas, orígenes, destinos y costos; varios tamaños; ≥ 5 repeticiones; equipo, semilla, tiempos, costo, nodos y memoria registrados; pruebas con cambios del entorno. | STK-01 | M | GUIA §7; STB §03 | HU-17 |
| StR-13 | El docente necesita comparar agentes aislados y coordinados con la misma semilla. | STK-01 | M | STB §03 | HU-18 |
| StR-14 | El docente necesita un análisis de complejidad temporal y espacial, incluida la pila de la recursión. | STK-01 | M | GUIA §5; STB §03 | RNF-027, RF-047, doc. de análisis |
| StR-15 | El evaluador necesita ejecutar el juego en un equipo modesto sin instalar nada adicional ni conectarse a internet. | STK-02, STK-05 | M | NEG-07 | RNF-001 a RNF-009 |
| StR-16 | El jugador necesita guardar y cargar la partida y ajustar opciones. | STK-04 | S | Acuerdo del equipo | HU-19 |
| StR-17 | El equipo necesita optimizar parámetros de comportamiento con una técnica de optimización (algoritmo genético o recocido simulado). | STK-03 | S | Objetivo general (optimización) | HU-21 |
| StR-18 | El superviviente de la radio debe actuar de forma autónoma y observable. | STK-01, STK-04 | S | STB §01 Personajes | HU-13 |
| StR-19 | El equipo necesita compilar, probar y perfilar con un solo comando y en integración continua. | STK-03 | S | Acuerdo del equipo | RNF-024, RNF-028 |
| StR-20 | El jugador necesita un tutorial breve del desplazamiento silencioso. | STK-04 | S | STB Esc. 1 | HU-20 |

---

## 6. Características del proyecto

### 6.1 Supuestos y dependencias

| ID | Supuesto |
|---|---|
| SUP-01 | El equipo de referencia mínimo es: CPU x86-64 de 2 núcleos a 2,0 GHz, 4 GB de RAM, GPU integrada con OpenGL 3.3, disco de 1 GB libre, Windows 10 x64 o Ubuntu 22.04 x64. |
| SUP-02 | El docente acepta C++ con raylib como tecnología (GUIA §7 permite «otras tecnologías a elección del equipo»). |
| SUP-03 | Los umbrales numéricos (radios, tiempos, cantidades) pueden ajustarse sin eliminar requisitos algorítmicos (STB §03 «Libertad de desarrollo»). |
| SUP-04 | El mapa de juego más grande no supera 256 × 256 celdas. |

### 6.2 Riesgos de negocio

| ID | Riesgo | Prob. | Impacto | Mitigación |
|---|---|---|---|---|
| RN-01 | Alcance de IA demasiado amplio para el tiempo del curso. | Alta | Alto | Priorización MoSCoW; GOAP y optimización son `S`, núcleo FSM + A\*/Dijkstra + recursión es `M`. |
| RN-02 | La comparación Dijkstra/A\* no muestra diferencias en mapas pequeños. | Media | Medio | Benchmark con 32², 64², 128² y 256², mapas con obstáculos y sin ellos. |
| RN-03 | Comportamiento emergente difícil de explicar en la sustentación. | Media | Alto | Panel y registro de decisiones con el motivo de cada cambio de plan. |
| RN-04 | Rendimiento insuficiente en el equipo del evaluador. | Baja | Alto | Presupuesto por tick, object pools, quadtree, flow fields; pruebas en equipo de referencia. |

### 6.3 Extensiones futuras (fuera de 1.0)

* Integrar un modelo de lenguaje local para diálogo del superviviente, validando sus acciones contra las reglas y separando su latencia del tiempo de búsqueda (GUIA §7).
* Editor de escenarios dentro del juego.

---

## 7. Validación del StRS

| Técnica | Responsable | Criterio de salida |
|---|---|---|
| Revisión cruzada con la GUIA y el STB, cláusula por cláusula | Equipo | Toda cláusula de la GUIA §4–§8 y del STB §01–§03 tiene al menos un StR asociado (ver 06_matriz_trazabilidad.md §2). |
| Revisión con el docente | STK-01 | Aprobación o lista de cambios. |
| Prototipo jugable de la Escena 1 | Equipo | El docente observa la exploración sin información global. |

---

## 8. Estado en 0.16.0

### 8.1 Objetivos de negocio

| ID | Estado | Evidencia o pendiente |
|---|---|---|
| NEG-01 | Cumplido | Ciclo percepción → memoria → decisión → planificación → ejecución en infectados y superviviente; el panel muestra cada etapa y el registro de IA las transiciones, planes y mensajes (HU-14, HU-15). |
| NEG-02 | Cumplido | Dijkstra y A\* sobre el mismo grafo; 100 % de costos iguales en el benchmark (RF-082); 4 tamaños × 5 repeticiones en `bench/config.json`. |
| NEG-03 | Cumplido | Señuelo y barricada sin omnisciencia (CP-025, CP-050, CP-052). La escena guionizada de CP-201 sigue pendiente. |
| NEG-04 | En curso | Complejidad en arc42 §8.6 y datos del benchmark y del estrés de recursión. Falta el informe con las tablas experimentales. |
| NEG-05 | Cumplido con cambios | Partida ganable y perdible de principio a fin, en un solo mapa en lugar de cinco escenas. |
| NEG-06 | En curso | Documentación actualizada a 0.16.0; falta el informe final y la demostración. |
| NEG-07 | Parcial | Local y sin red. Búsqueda y memoria dentro de objetivo; FPS, CPU y arranque sin medir en el equipo CM-01 (ver 04 §1.1). |

### 8.2 Requisitos de las partes interesadas

| StR | Estado | Comentario |
|---|---|---|
| StR-01 | Cumplido | HU-01. |
| StR-02 | Cumplido | HU-02, HU-03. Falta la puerta metálica (RF-006). |
| StR-03 | Cumplido | HU-04, HU-05. |
| StR-04 | Cumplido | HU-06, HU-07. |
| StR-05 | Cumplido | RF-041, RF-042; los agentes solo usan su percepción, su memoria y los mensajes. |
| StR-06 | Cumplido | RF-043, RF-050, RF-052, RF-064. |
| StR-07 | Cumplido | RF-047 con caso base, visitados y profundidad 3; estrés de recursión en CI. |
| StR-08 | Cumplido | RF-049, RF-051, RF-053, RF-054. |
| StR-09 | Cumplido | RF-020 a RF-024; heurística octil admisible y consistente (CP-022). |
| StR-10 | Cumplido | RF-070 con los nueve campos de la guía. |
| StR-11 | Cumplido | RF-072, RF-073; `logs/ia_<semilla>.log`. |
| StR-12 | Cumplido | RF-080 a RF-083, RF-086. |
| StR-13 | Cumplido | RF-056, RF-084 (`--coord-exp`). |
| StR-14 | En curso | Análisis en arc42 §8.6 y RF-047; falta el informe final. |
| StR-15 | Parcial | Sin red ni instalación; no hay `.zip` portable ni mediciones en CM-01. |
| StR-16 | Cumplido | HU-19. |
| StR-17 | Cumplido | HU-21 (GA y SA). |
| StR-18 | Cumplido | HU-13. |
| StR-19 | Cumplido | CMake con presets y CI en cada push (GCC, Clang, MSVC, ASan/UBSan). |
| StR-20 | Cumplido | HU-20. |
