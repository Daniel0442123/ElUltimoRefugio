# El último refugio

Videojuego 2D de vista cenital con **agentes inteligentes autónomos**: Mara debe reunir medicamentos,
combustible y una batería para activar la radio de un refugio y escapar, mientras una horda de infectados
la busca usando solo lo que percibe, recuerda y se comunica.

Proyecto final de **Algoritmos Computacionales** (Fundación Universitaria de Popayán), proyecto 2:
*apocalipsis zombi y coordinación*.

| | |
|---|---|
| Lenguaje | C++20 |
| Motor | raylib 5 |
| Construcción | CMake ≥ 3.24 (+ Ninja opcional) |
| Pruebas | Catch2 v3, CTest, ASan/UBSan |
| Arquitectura | Hexagonal (dominio de IA puro) + ECS + bus de eventos — ver [docs/07_arquitectura_arc42.md](docs/07_arquitectura_arc42.md) |

## Estado actual (0.9.0)

**Panel de monitoreo y registro de planes** (HU-14, HU-15). Ya se puede ver qué piensa cada agente:

- **Monitor del agente** (RF-070, RF-023): con un clic sobre un infectado, la parte baja del panel
  muestra su estado, su objetivo (con la táctica o la tarea de Contract Net), la percepción del
  último ciclo de IA (si ve a Mara y a qué distancia, cuántos ruidos oyó, la alerta que recibió y
  cuántas pistas guarda), y su última búsqueda: algoritmo, heurística, origen y destino, costo,
  largo de la ruta, nodos expandidos y generados, tamaño máximo de la abierta, tiempo en µs y
  memoria. Debajo va su historial de cambios de estado y de plan. Clic fuera de un agente lo suelta.
- **Superposición** (RF-071): la ruta del agente seleccionado se dibuja en blanco (sin necesidad de
  F1), con su objetivo marcado, y los nodos que cerró su última búsqueda se pintan en magenta, más
  intensos cuanto antes se expandieron. Hay exactamente tantos como dice «Nodos expandidos»
  (CP-071). Al seleccionarlo se repite su última consulta para que los nodos se vean al momento.
  **N** oculta o muestra los nodos.
- **Registro de IA** (RF-072, RF-073): cada cambio de estado, cambio de plan, plan nuevo del
  superviviente y mensaje entre agentes (alerta, anuncio, puja y adjudicación de Contract Net) se
  anota con su tick. **L** abre una consola sobre el mapa con tres vistas: estados y planes,
  mensajes, o todo. Las mismas líneas se escriben en `logs/ia_<semilla>.log` (junto a donde se
  ejecuta el juego), con el formato `tick⇥tipo⇥agente⇥campos`:

  ```text
  31	transicion	Z8	estado=Explorar>Investigar	motivo=oye ruido
  45	plan	Z3	anterior=(1,1)	nuevo=(14,15)	motivo=senuelo	valores=0.21>0.52
  52	mensaje	Z0>Z1	tipo=alerta	salto=1	confianza=0.70
  52	mensaje	Z2>Z0	tipo=puja	salto=1	confianza=0.70	costo=6.41
  ```

  Si la carpeta `logs/` no se puede crear, el juego sigue sin archivo.
- Seleccionar un agente no cambia la simulación (misma semilla, mismo resultado) y ni el monitor
  ni el registro asignan memoria en el bucle (CP-106).

### Versión 0.8.0

**Superviviente de la radio autónomo** (HU-13). En el refugio vive un aliado que decide solo qué
hacer, con tres técnicas encadenadas (ADR-0008):

- **Meta por utilidad** (RF-060): en cada tick de IA puntúa cuatro metas en [0, 1]. El peligro
  sigue una logística de la distancia al infectado más cercano que ve, p = 1 / (1 + e^(2(d − 3))).
  *PonerseASalvo* vale p. *RefugioSeguro* vale (1 − integridad) · (1 − p).
  *AyudarInstalacion* vale 0,8 · (1 − p) mientras Mara instala. *Vigilar* vale 0,2.
  Cambia de meta solo si la nueva supera a la actual en 0,1.
- **Plan GOAP** (RF-061): A\* regresivo sobre hechos (EnAlmacen, EnPuerta, TieneTablas, …), con
  como mucho 6 acciones. Con el refugio dañado y sin tablas, el plan es IrAlmacen, RecogerTablas,
  IrPuerta y RepararPuerta. Si el almacén está lejos o bloqueado, arranca tablas de los muebles
  (15 s).
- **Árbol de comportamiento** (RF-062): cada acción se ejecuta con un subárbol de secuencias,
  selectores, un inversor y hojas que devuelven Exito, Fallo o EnCurso.
- **Reparar** (RF-063): suma 8 puntos de integridad por segundo, hasta 40 por carga de tablas y sin
  pasar de 100. **Ayudar**: si está junto a la radio mientras Mara la instala, la instalación va
  un 50 % más rápida.
- **Replanificar** (RF-064): si una acción falla (por ejemplo, una barricada cierra el almacén) o
  deja de cumplirse una precondición, descarta el plan. En el siguiente tick de IA genera uno nuevo
  y registra el motivo.
- En el mapa aparecen el almacén («ALM») y el superviviente (círculo turquesa, con tablas si las
  lleva). Con F1 se ve también su ruta. El panel muestra su meta, el plan con la acción en curso
  entre corchetes, las cuatro utilidades y el motivo del último plan.
- Por ahora los infectados no atacan al superviviente: solo él los evita.

### Versión 0.7.0

**Extracción y derrotas** (HU-04, HU-05). La partida ya tiene su desenlace completo:

- **Extracción** (RF-009, RF-010): activar la radio ya no gana al instante. Abre durante 60 s una
  zona de extracción (círculo verde con temporizador) en el otro extremo de la ciudad, y entrar en
  ella mientras está abierta es la victoria. Si se cierra, la partida sigue y la radio se puede
  volver a activar.
- **Refugio** (RF-012, RF-063): la radio está en el refugio. Cada infectado que investiga o busca
  junto a su puerta la golpea y le quita 5 puntos de integridad por segundo. Con integridad 0 la
  partida se pierde con «Derrota: refugio inutilizado». Instalar la radio hace ruido (radio 4, una
  vez por segundo), así que la horda se acerca al refugio. La reparación llega con el superviviente
  de la radio (HU-13).
- **Derrota por salud** (RF-011, RF-055): cada infectado en Atacar quita 10 de salud por segundo.
  Con salud 0, «Derrota: Mara ha caído».
- El panel muestra el tiempo de extracción que queda y la integridad del refugio, que parpadea en
  rojo mientras la golpean.

### Versión 0.6.0

**Infectados que eligen táctica según sus pistas** (HU-12). Sobre los señuelos y barricadas de 0.5.0:

- **Selección de táctica** (RF-054): en cada ciclo de IA, cada infectado toma la mejor evidencia de su
  memoria y puntúa tres tácticas. *Perseguir* vale c / (1 + d/10), y si la evidencia es un
  avistamiento, a d se le suma lo que Mara pudo moverse desde entonces. *Interceptar* vale
  0,8 · c / (1 + d_corte/10), pero solo cuando conoce su rumbo y hay un punto de corte.
  *Explorar* vale 0,15 fijo. Gana la de mayor puntuación, con un 10 % de histéresis para no
  parpadear.
- **Punto de corte** (RF-053): con sus dos últimos avistamientos estima la velocidad y el rumbo de
  Mara. Prolonga su ruta en línea recta hasta la primera pared y elige la celda más cercana entre las
  que estén 2 a 6 s por delante de Mara y a las que él llegue antes que ella.
- Así, quien la ve cerca la persigue y quien está junto al cruce siguiente le corta el paso
  (estado Interceptar, ahora también sin Contract Net). Si la pista es vieja o débil, deja de
  investigar y vuelve a explorar.
- El cambio de táctica queda en «Cambios de plan» con las dos puntuaciones. El panel cuenta cuántos
  infectados prefieren cada táctica, y con F1 se ve el punto de corte.

### Versión 0.5.0

**Señuelos, barricadas y una horda que reconsidera** (HU-06, HU-07, HU-11). Sobre la horda de 0.4.0:

- **Señuelo** (RF-013): con **Q** Mara lanza un señuelo hacia el cursor. Cae en la celda libre más
  lejana de la línea, a 8 celdas como máximo y sin atravesar paredes, y suena 10 veces (una por
  segundo) con radio 8. Empieza con 2.
- **Barricada** (RF-014, RF-025): con **B** pone una barricada en la celda de al lado, en dirección
  al cursor, si está libre. La celda deja de ser transitable (evento CambioDeMapa) y el campo de flujo
  se recalcula. Empieza con 3; cada una aguanta 100 puntos.
- **Memoria de evidencias** (RF-043): cada infectado guarda hasta 16 pistas (avistamientos, ruidos y
  alertas) cuya confianza se reduce a la mitad cada 10 s.
- **Reevaluación** (RF-050): mientras investiga, si oye un ruido (un señuelo, por ejemplo) con más
  utilidad que la pista que sigue, cambia de objetivo. La utilidad es U = c · e^(−edad/τ) / (1 + d/10).
  Si ve a Mara, el señuelo no lo distrae.
- **Respuesta a barricadas** (RF-052): un infectado solo se entera de una barricada en su ruta cuando
  la ve. Entonces replanifica. Si no hay rodeo, o el rodeo cuesta más del doble, pasa a
  **AtacarBarricada** (marrón) y le quita 10 puntos por segundo. Al romperla sigue con lo que hacía.
- **Cambios de plan** (RF-072): el panel muestra los últimos, con el agente, el objetivo anterior y el
  nuevo, el motivo y las utilidades (o los costos de ruta, si fue por una barricada).
- Nada de esto asigna memoria en el bucle (CP-106).

### Versión 0.4.0

**Horda que reparte la aproximación** (HU-10). Sobre las alertas de 0.3.0:

- **Contract Net** (RF-049): el infectado que ve a Mara hace de gestor y anuncia tareas a los que
  alertó y aún no la persiguen: *Interceptar* el punto donde estará Mara en 2 s según sus dos últimos
  avistamientos (RF-053), *Flanquear* por hasta dos accesos a 3 celdas del lado opuesto al suyo, y
  *Perseguir*. Cada uno puja con su distancia octil y cada tarea se adjudica a la puja más baja, una
  por agente. Los de interceptar o flanquear pasan al nuevo estado **Interceptar** (turquesa).
- **Reparto de la llegada** (RF-051): con dos o más perseguidores, las 8 celdas alrededor de Mara se
  reparten minimizando la suma de distancias, con 2 infectados por celda como máximo; a 2,5 celdas
  cada uno se acerca por la suya en vez de amontonarse por el mismo lado.
- **Steering** (RF-029): el movimiento de los infectados ya no es directo. Cada paso suma seguir,
  separación, alineación, cohesión, evitación de paredes y anticipación de choques, con aceleración y
  velocidad máximas. Los vecinos salen del quadtree (RF-030).
- Con F1 se ven el destino de cada tarea (cuadro turquesa) y la celda de llegada de cada perseguidor
  (cuadro naranja). El panel cuenta las subastas y las tareas adjudicadas. Nada de esto asigna memoria
  en el bucle (CP-106).

### Versión 0.3.0

**Alertas y campo de flujo.** Sobre el prototipo jugable de 0.2.0:

- **Alertas recursivas entre infectados** (RF-046, RF-047): quien ve a Mara avisa a los que tiene a
  5 celdas o menos, y estos a los suyos, hasta 3 saltos, con un conjunto de visitados y confianza
  ×0,7 por salto. Los alertados pasan a *Investigar* la celda donde la vieron (motivo «recibe alerta»).
  En pantalla, los mensajes se ven 1 s como líneas amarillas. La tecla **C** activa o desactiva el
  modo coordinado para comparar (RF-084).
- **Flow field compartido** (RF-028): cuando 5 o más infectados persiguen a Mara (8 en
  `config/ia.json`; el prototipo usa 5 porque solo tiene 10), se calcula un único campo con Dijkstra
  inverso desde su celda y esos agentes dejan de pedir rutas A*. Solo se recalcula si Mara cambia de
  celda. Con F1 se ven las flechas del campo.
- Las alertas y el campo tampoco asignan memoria en el bucle (CP-106).

### Versión 0.2.0

**Primer prototipo jugable.** Mara recorre una ciudad generada con semilla, reúne medicamentos,
combustible y una batería y mantiene E en la radio 8 s para ganar, mientras 8 infectados autónomos
la buscan solo con lo que ven y oyen:

- Mara con modos sigilo / caminar / correr (1,5 / 3 / 6 celdas/s), ruido de radio 1 / 3 / 6 dos veces
  por segundo y colisiones que deslizan contra las paredes (RF-002 a RF-005, RF-007, RF-008, RF-010, RF-011).
- Infectados con **máquina de estados** Explorar → Investigar → Perseguir → Buscar → Atacar; cada
  transición se registra con su motivo y se ve en el panel (RF-040, RF-044). Interceptar y
  AtacarBarricada llegan con la coordinación.
- **Percepción local sin omnisciencia**: cono de visión con línea de visión por Bresenham y oído que
  pierde 2 celdas de alcance por pared (RF-041, RF-042). Exploración con mapa de visitas propio (RF-045).
- Persecución con **A\*** compartido y presupuesto de nodos por tick de IA (RF-026, RF-027); separación
  entre infectados con el quadtree (RF-030).
- Bucle de paso fijo a 60 Hz con IA a 10 Hz (ADR-0005) y **cero asignaciones de memoria por paso**
  tras la carga, comprobado por prueba (RNF-006, CP-106).

Núcleo algorítmico (desde 0.1.0): grafo 8-conexo con costos de terreno (RF-020, RF-031), Dijkstra y A\*
octil con métricas (RF-021 a RF-024), alertas recursivas acotadas (RF-047), quadtree (RF-030), PCG32
(RF-086), benchmark y estrés de recursión (RF-080 a RF-082, RNF-027) y el laboratorio Dijkstra / A\* (HU-16).

### Rendimiento de A\* (RNF-003)

`refugio_bench --bench --tamanos 128 --pares 50 --reps 10 --semilla 42`, mapa 128 × 128 con 25 % de
obstáculos, GCC 13 -O3, Xeon 2,3 GHz:

| Versión | Mediana | p95 | Memoria |
|---|---|---|---|
| 0.1.0 | 126 µs | 427 µs | 328 KB |
| 0.2.0 | 84 µs | 306 µs | 261 KB |

Cambios: bucle de vecinos sin divisiones ni comprobaciones repetidas, estado de cada nodo en un
registro de 16 bytes, entradas de la abierta de 16 bytes, extracción del montículo «de abajo arriba»
con elección del hijo sin saltos y resultados reutilizables (`buscarEn`) que no asignan memoria.
Los nodos expandidos y los costos no cambian (el benchmark sigue sin discrepancias de optimalidad).

Lo que sigue está en el backlog de [docs/02_historias_usuario.md](docs/02_historias_usuario.md).

## Compilar

Requisitos: compilador con C++20 (GCC 12+, Clang 15+ o MSVC 2022), CMake ≥ 3.24 y Git. En Linux, además:
`sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`.
raylib y Catch2 se descargan solos con `FetchContent`.

```bash
# Juego + pruebas (Release)
cmake --preset release
cmake --build --preset release
ctest --test-dir build/release --output-on-failure
./build/release/refugio

# Solo dominio, pruebas y benchmark (sin ventana, sin raylib)
cmake --preset headless
cmake --build --preset headless
ctest --preset headless

# Pruebas con AddressSanitizer + UBSan
cmake --preset asan && cmake --build --preset asan && ctest --preset asan
```

Los presets de prueba son `debug`, `headless` y `asan` (no hay `ctest --preset release`); para el
build `release` usa `ctest --test-dir build/release --output-on-failure`.

En Windows con Visual Studio sin Ninja (Developer PowerShell for VS 2022):

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\refugio.exe
```

### Controles

Partida (escena inicial):

| Tecla | Acción |
|---|---|
| WASD / flechas | Mover a Mara |
| Shift / Ctrl (mantener) | Correr (más ruido) / sigilo (casi sin ruido) |
| E (mantener) | Recoger un objeto a 1 celda; en la radio y con los tres objetos, activarla (8 s) y abrir la extracción 60 s |
| Q | Lanzar un señuelo hacia el cursor (8 celdas como máximo) |
| B | Poner una barricada en la celda de al lado, hacia el cursor |
| F1 | Depuración: conos de visión, rutas de A\*, radio del último ruido, flechas del campo de flujo, tareas y celdas de llegada |
| Clic izquierdo | Seleccionar el infectado bajo el cursor y ver su monitor; clic en otro sitio lo suelta |
| N | Mostrar u ocultar los nodos expandidos del agente seleccionado |
| L | Consola del registro de IA: estados y planes → mensajes → todo → oculta |
| C | Activar / desactivar el modo coordinado (alertas y Contract Net entre infectados) |
| R | Nueva partida (siguiente semilla) |
| Tab | Ir al laboratorio Dijkstra / A\* |

Colores de los infectados: verde = explorar, amarillo = investigar, naranja = perseguir,
magenta = buscar, rojo = atacar, turquesa = interceptar (tarea de Contract Net), marrón = atacar
una barricada.

Laboratorio Dijkstra / A\*:

| Tecla | Acción |
|---|---|
| Clic izquierdo / derecho | Fijar origen / destino |
| B | Poner o quitar una barricada bajo el cursor |
| F2 | Alternar Dijkstra / A\* |
| F3 | Comparar ambos algoritmos en el panel |
| R | Nuevo mapa (siguiente semilla) |
| Tab | Volver a la partida |

En el laboratorio los nodos expandidos por Dijkstra se pintan en cian y los de A\* en magenta; los
escombros (costo 2 a 5) son marrones.

## Experimentos

```bash
# Dijkstra vs A*: 4 tamaños x 20 pares x 5 repeticiones, semilla 42 -> CSV con metadatos del equipo
./build/headless/refugio_bench --bench --tamanos 32,64,128,256 --pares 20 --reps 5 --semilla 42 --salida results/bench.csv
python3 tools/resumen_bench.py results/bench.csv

# Estrés de las funciones recursivas (profundidad, llamadas, mensajes, pila)
./build/headless/refugio_bench --estres-recursion --agentes 50,500,2000 --profundidades 1,3,10 --salida results/estres.csv
```

El benchmark termina con código 2 si A* y Dijkstra difieren en costo (RF-082); el estrés termina con
código 3 si alguna recursión supera su límite, si un agente recibe la misma alerta dos veces o si la pila
supera 64 KB (RNF-027).

## Integración continua

| Flujo | Cuándo | Qué hace |
|---|---|---|
| `CI` | push a `main` y cada PR | Compila juego y pruebas en Linux (GCC, Clang) y Windows (MSVC); ASan + UBSan; comprueba que el dominio no dependa de raylib; humo de benchmark y estrés |
| `Rendimiento y estrés` | cada noche y manual (Actions → Run workflow) | Benchmark completo y estrés de recursión en Linux y Windows; tabla en el resumen del run y CSV como artefacto |

## Estructura

```text
src/dominio/       Lógica pura (sin raylib): núcleo, mundo, navegación, agentes (FSM, percepción),
                   juego (partida), coordinación, superviviente (Utility, GOAP, árbol), experimentos, puertos
src/adaptadores/   raylib (vistas) y archivo (registro de IA en logs/); luego Tiled y raygui
src/app/           main del juego, main del benchmark y CLI
tests/             Pruebas Catch2 (los nombres empiezan por el ID del caso de prueba, p. ej. CP-047)
config/            Parámetros de IA
docs/              Requisitos (ISO 29148), RNF (ISO 25010), trazabilidad, arc42 + C4 y ADR
tools/             Scripts de análisis y comprobaciones de CI
```

## Documentación

Empieza por [docs/README.md](docs/README.md).
