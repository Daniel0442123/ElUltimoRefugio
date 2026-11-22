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

## Estado actual (0.3.0)

**Horda coordinada.** Sobre el prototipo jugable de 0.2.0:

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
| E (mantener) | Recoger un objeto a 1 celda; en la radio y con los tres objetos, activarla (8 s) |
| F1 | Depuración: conos de visión, rutas de A\*, radio del último ruido y flechas del campo de flujo |
| C | Activar / desactivar el modo coordinado (alertas entre infectados) |
| R | Nueva partida (siguiente semilla) |
| Tab | Ir al laboratorio Dijkstra / A\* |

Colores de los infectados: verde = explorar, amarillo = investigar, naranja = perseguir,
magenta = buscar, rojo = atacar.

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
                   juego (partida), coordinación, experimentos, puertos
src/adaptadores/   Implementaciones con raylib (y luego archivos, Tiled, raygui)
src/app/           main del juego, main del benchmark y CLI
tests/             Pruebas Catch2 (los nombres empiezan por el ID del caso de prueba, p. ej. CP-047)
config/            Parámetros de IA
docs/              Requisitos (ISO 29148), RNF (ISO 25010), trazabilidad, arc42 + C4 y ADR
tools/             Scripts de análisis y comprobaciones de CI
```

## Documentación

Empieza por [docs/README.md](docs/README.md).
