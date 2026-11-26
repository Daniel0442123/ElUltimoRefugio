# 05 · Catálogo de casos de prueba

> Estructura ligera derivada de ISO/IEC/IEEE 29119-3 (especificación de casos de prueba).
> Cada requisito funcional `RF-nnn` tiene al menos el caso `CP-nnn`; cada no funcional `RNF-0nn`
> tiene el caso `CP-1nn`; los casos `CP-2nn` son pruebas de aceptación de extremo a extremo.

| Campo | Valor |
|---|---|
| Versión | 0.2.0 (estado real del software 0.16.0) |
| Fecha | 2026-11-25 |
| Herramientas usadas | Catch2 v3.7.1, CTest, ASan + UBSan, GitHub Actions, `refugio_bench` (benchmark, estrés, experimentos), `tools/check_deps.sh`, `tools/resumen_bench.py`, `tools/trazabilidad.py` |
| Herramientas planificadas sin usar | Tracy, Valgrind, libFuzzer, llvm-cov, `tools/fps_report.py`, `tools/recursos.py`, `tools/latencia_ia.py`, `tools/validar_csv.py` |
| Última ejecución | 172/172 pruebas en verde (GCC 13.3, Release, 1,4 s) sobre el commit `e8f100e` |

## 1. Niveles y estrategia

| Nivel | Alcance | Ubicación real | Cuándo se ejecuta |
|---|---|---|---|
| Unitaria | Una clase o función del dominio, sin raylib | `tests/unit/` | Cada push y PR (CI, 3 compiladores + ASan/UBSan) |
| Integración | Una `Partida` completa con varios sistemas y adaptadores de archivo | `tests/unit/` (mismo ejecutable) | Cada push y PR |
| Sistema | `refugio` / `refugio_bench` por línea de comandos (`Cli`) | `tests/unit/` y paso «Humo» de `ci.yml` | Cada push y PR |
| Rendimiento | Benchmark y estrés de recursión con semilla | `rendimiento.yml` (nocturno, Linux y Windows) | Cada noche y a mano |
| Aceptación | Escenarios del docente y del storyboard | Manual (ver §2.3) | Antes de la entrega y en la sustentación |

* Hay **un solo ejecutable** de pruebas, `refugio_tests`, con 172 casos en 26 archivos. Los niveles de la tabla
  son una clasificación; no hay carpetas `integration/`, `system/` ni `acceptance/`.
* Las pruebas de dominio se ejecutan **sin ventana** gracias a la arquitectura hexagonal (ADR-0003).
* Los criterios Gherkin de [02_historias_usuario.md](02_historias_usuario.md) se implementan con `TEST_CASE`
  de Catch2 cuyo nombre empieza por el CP y que llevan como etiquetas el RF, el RNF o la HU (`[RF-047][HU-09]`).
  Los nombres son ASCII para que CTest los encuentre en Windows.
* Todas las pruebas que usan aleatoriedad fijan semilla y la imprimen al fallar.
* Criterio de salida de una entrega: 100 % de los CP automatizados `M` aprobados, 0 defectos críticos abiertos.

## 2. Casos de prueba

Columna **Auto** (diseño): `Sí` automatizada en CI; `Parcial` automatizada la medición pero con revisión manual; `No`
manual. El estado real de cada caso y las pruebas que lo respaldan están en §5.

### 2.1 Requisitos funcionales

| CP | Título | Verifica | Nivel | Procedimiento y datos | Resultado esperado | Auto |
|---|---|---|---|---|---|---|
| CP-001 | Carga de mapa Tiled | RF-001 | Integración | Cargar `tests/data/mapa_32.json` con 6 capas. | Se crean las 6 capas; 1024 celdas; objetos y zonas con sus coordenadas. | Sí |
| CP-002 | Velocidad por modo | RF-002 | Unitaria | Simular 60 ticks en cada modo (sigilo, caminar, correr) sin obstáculos. | Desplazamiento 1,5 / 3,0 / 6,0 celdas ±0,1. | Sí |
| CP-003 | Ruido por modo | RF-003 | Unitaria | Mover 1 s en cada modo y capturar eventos `Ruido` del bus. | 2 eventos por segundo con radios 1 / 3 / 6. | Sí |
| CP-004 | Colisiones | RF-004 | Unitaria | Mover a Mara y a un agente contra una pared y una barricada. | Ninguno ocupa la celda bloqueada. | Sí |
| CP-005 | Recoger objeto | RF-005 | Unitaria | Interactuar a distancia 1 y a distancia 3. | Inventario +1 solo a distancia 1; objeto retirado del mapa. | Sí |
| CP-006 | Ruido de puerta metálica | RF-006 | Unitaria | Abrir la puerta metálica de la farmacia. | 1 evento `Ruido` de radio 10 con origen en la puerta. | Sí |
| CP-007 | Activar radio | RF-007 | Integración | Inventario completo; mantener interactuar 8 s (480 ticks). | Radio activada; evento `RadioActivada`. Con objetos incompletos no inicia. | Sí |
| CP-008 | Interrumpir instalación | RF-008 | Unitaria | Aplicar daño a los 5 s; soltar la tecla a los 3 s. | Progreso vuelve a 0 en ambos casos. | Sí |
| CP-009 | Ventana de extracción | RF-009 | Unitaria | Publicar `RadioActivada` y avanzar 59 s y 61 s. | Zona abierta a 59 s, cerrada a 61 s; temporizador visible. | Sí |
| CP-010 | Victoria | RF-010 | Integración | Llevar a Mara a la zona abierta. | Resultado `Victoria`. | Sí |
| CP-011 | Derrota por salud | RF-011 | Unitaria | Fijar salud 5 y aplicar 10 de daño. | Resultado `Derrota: Mara ha caído`. | Sí |
| CP-012 | Derrota por refugio | RF-012 | Unitaria | Fijar integridad 3 y aplicar golpe de 5. | Resultado `Derrota: refugio inutilizado`. | Sí |
| CP-013 | Señuelo | RF-013 | Unitaria | Lanzar a 6 y a 12 celdas. | Señuelo a 6 y a 8 celdas; 10 eventos de ruido de radio 8 en 10 s. | Sí |
| CP-014 | Barricada | RF-014 | Unitaria | Colocar en celda libre y en celda ocupada. | Celda libre pasa a no transitable y se publica `CambioDeMapa`; la ocupada se rechaza. | Sí |
| CP-015 | Medicamento | RF-015 | Unitaria | Salud 40 y 80; usar medicamento. | Salud 75 y 100 (tope). | Sí |
| CP-016 | HUD | RF-016 | Sistema | Capturar el HUD en cada escena. | Salud, inventario, objetivo y temporizador presentes. | No |
| CP-017 | Tutorial | RF-017 | Sistema | Partida nueva; realizar cada acción; repetir con «Saltar». | Indicaciones en orden y retiradas al cumplirse; sin indicaciones al saltar. | Parcial |
| CP-020 | Construcción del grafo | RF-020 | Unitaria | Mapa 5×5 con una pared en esquina; contar vértices y aristas. | |V| y |E| esperados; no hay aristas diagonales que corten esquinas; w ≥ 1. | Sí |
| CP-021 | Dijkstra óptimo | RF-021 | Unitaria | 10 mapas pequeños con costo óptimo calculado a mano / por fuerza bruta (BFS ponderado). | Costo igual al óptimo en 10/10. | Sí |
| CP-022 | A* óptimo y heurística | RF-022 | Unitaria | Mismos 10 mapas; verificar h(n) ≤ h*(n) y h(n) ≤ w + h(n') en todas las aristas. | Costo óptimo; admisibilidad y consistencia en el 100 % de nodos y aristas. | Sí |
| CP-023 | Métricas de búsqueda | RF-023 | Unitaria | Búsqueda en mapa conocido. | Se devuelven ruta, costo, expandidos, generados, max abierta, tiempo y memoria (> 0). | Sí |
| CP-024 | Mismo grafo e interfaz | RF-024 | Unitaria | Inspección de código y prueba con un `IBuscadorRuta` falso. | Ambos algoritmos reciben la misma referencia de `Grafo`. | Sí |
| CP-025 | Replanificación por cambio | RF-025 | Integración | Agente con ruta por (30,12); colocar barricada allí. | La ruta se invalida y la nueva no pasa por (30,12). | Sí |
| CP-026 | Presupuesto por tick | RF-026 | Integración | 100 solicitudes simultáneas en mapa 128². | ≤ 4000 nodos por tick; todas resueltas en ≤ 10 ticks de IA. | Sí |
| CP-027 | Sin ruta | RF-027 | Unitaria | Destino encerrado. | `SinRuta` y el agente cambia de comportamiento en el tick siguiente. | Sí |
| CP-028 | Flow field | RF-028 | Integración | 20 agentes con el mismo destino. | 1 flow field calculado; 0 A* individuales; todos llegan. | Sí |
| CP-029 | Steering | RF-029 | Unitaria | Dos agentes en trayectorias que se cruzan. | La separación evita el solape; velocidad ≤ máxima. | Sí |
| CP-030 | Quadtree | RF-030 | Unitaria | 1000 puntos aleatorios; comparar consultas por radio con fuerza bruta. | Mismos resultados; profundidad ≤ 8. | Sí |
| CP-031 | Costos de terreno | RF-031 | Unitaria | Mapas con m = 1, 5, 10 y uno con m = 0. | Costos correctos; el mapa con 0 se rechaza. | Sí |
| CP-040 | Orden del ciclo de agente | RF-040 | Unitaria | Agente con etapas instrumentadas. | Orden percepción → memoria → planificación → decisión → ejecución → evaluación en cada tick. | Sí |
| CP-041 | No omnisciencia | RF-041 | Unitaria | Mara detrás de pared y fuera del cono; además inspección de dependencias del módulo de decisión. | Ninguna evidencia de Mara; el módulo no incluye el registro ECS. | Sí |
| CP-042 | Audición con paredes | RF-042 | Unitaria | Ruido de radio 6 a 5 celdas con 0 y 1 pared intermedia. | Se oye sin pared; no se oye con 1 pared (alcance 4). | Sí |
| CP-043 | Memoria y decaimiento | RF-043 | Unitaria | Insertar 20 evidencias; avanzar 10 s. | Se conservan las 16 de mayor confianza; confianza a la mitad tras 10 s. | Sí |
| CP-044 | Transiciones FSM | RF-044 | Unitaria | Tabla de transiciones con un caso por fila. | Cada transición ocurre y se registra con motivo. | Sí |
| CP-045 | Exploración propia | RF-045 | Sistema | 1 explorador solo en mapa 64², 120 s, semilla 42. | Cobertura ≥ 30 %; ninguna celda objetivo repetida > 3 veces. | Sí |
| CP-046 | Investigar ruido | RF-046 | Unitaria | Agente en Explorar; ruido a 5 celdas. | Estado Investigar y destino = origen del ruido en el siguiente tick de IA. | Sí |
| CP-047 | Propagación recursiva | RF-047 | Unitaria | Cadena A-B-C-D-E a 4 celdas; triángulo X-Y-Z; grafo completo de 50 agentes. | B, C, D alertados y E no; cada agente alertado una vez; profundidad ≤ 3. | Sí |
| CP-048 | Roles | RF-048 | Unitaria | Crear un explorador y un perseguidor desde el prototipo. | Visión y velocidad según `config/ia.json`. | Sí |
| CP-049 | Contract Net | RF-049 | Integración | 1 gestor, 6 agentes alertados con costos conocidos. | Cada tarea adjudicada a la menor puja; una tarea por agente; mensajes registrados. | Sí |
| CP-050 | Reevaluación ante señuelo | RF-050 | Integración | Agente con evidencia vieja (c = 0,3); señuelo a 6 celdas; otro caso con avistamiento a 3 celdas. | Cambia al señuelo en ≤ 5 ticks de IA; con avistamiento mantiene Perseguir. | Sí |
| CP-051 | Distribución de aproximación | RF-051 | Integración | 12 perseguidores con el mismo objetivo; 10 s. | ≤ 2 por celda destino y en todo tick; ≥ 3 accesos distintos. | Sí |
| CP-052 | Respuesta a barricadas | RF-052 | Integración | Barricada con alternativa barata y sin alternativa. | Replanifica en el primer caso; AtacarBarricada en el segundo y la durabilidad baja 10/s. | Sí |
| CP-053 | Interceptación | RF-053 | Unitaria | Mara a 3 celdas/s hacia el este; perseguidor cerca del cruce. | Destino en la ruta prevista ≥ 2 s por delante de Mara. | Sí |
| CP-054 | Selección de táctica | RF-054 | Unitaria | Esquema de 4 ejemplos de HU-12. | Táctica esperada en 4/4 y puntuación registrada. | Sí |
| CP-055 | Ataque | RF-055 | Unitaria | Infectado adyacente en Atacar durante 1 s. | Salud −10. | Sí |
| CP-056 | Modo aislado | RF-056 | Integración | Mismo escenario y semilla en aislado y coordinado. | En aislado 0 alertas y 0 mensajes Contract Net; resto idéntico hasta la primera detección. | Sí |
| CP-060 | Utilidad del superviviente | RF-060 | Unitaria | Integridad 30 % sin amenaza; amenaza a 2 celdas; oscilación cerca del empate. | RefugioSeguro; PonerseASalvo; sin cambios de meta por debajo de la histéresis. | Sí |
| CP-061 | GOAP | RF-061 | Unitaria | Estado inicial sin tablas, meta RefugioSeguro. | Plan [IrAlmacen, RecogerTablas, IrPuerta, RepararPuerta] de costo mínimo; profundidad ≤ 6. | Sí |
| CP-062 | Árbol de comportamiento | RF-062 | Unitaria | Secuencia y selector con hojas simuladas. | Estados Exito / Fallo / EnCurso propagados correctamente. | Sí |
| CP-063 | Integridad del refugio | RF-063 | Unitaria | 1 infectado golpeando 2 s; superviviente reparando 2 s. | −10 y +16 con tope en 100. | Sí |
| CP-064 | Replanificación del superviviente | RF-064 | Integración | Bloquear el almacén durante el plan. | Plan descartado y uno nuevo en el siguiente tick; motivo registrado. | Sí |
| CP-070 | Panel del agente | RF-070 | Sistema | Seleccionar un infectado con el panel activo. | Los 9 campos de GUIA §6 visibles y actualizados. | Parcial |
| CP-071 | Superposiciones | RF-071 | Sistema | Activar cada superposición. | Nodos pintados = nodos expandidos del panel. | Parcial |
| CP-072 | Registro de cambios de plan | RF-072 | Integración | Forzar 3 cambios de plan. | 3 líneas con tick, id, plan anterior, nuevo y motivo en panel y archivo. | Sí |
| CP-073 | Registro de mensajes | RF-073 | Integración | Propagar una alerta en la cadena de CP-047. | 3 mensajes con emisor, receptor, tipo, salto y confianza. | Sí |
| CP-074 | Comparación en vivo | RF-074 | Sistema | F3 sobre un agente en mapa 64²; F2 dos veces. | Tabla con ambos algoritmos y mismo costo; el algoritmo activo alterna. | Parcial |
| CP-075 | Control de simulación | RF-075 | Integración | Pausa + N ×10; velocidades 0,25× y 4× sobre 600 ticks. | 10 ticks exactos; mismo hash de estado final a cualquier velocidad. | Sí |
| CP-080 | Benchmark completo | RF-080 | Sistema | `refugio --bench tests/data/bench_mini.json` (4 tamaños × 20 pares × 5 reps). | CSV con 800 filas y columnas de RF-080. | Sí |
| CP-081 | Metadatos | RF-081 | Unitaria | Generar un CSV y analizar su cabecera. | CPU, núcleos, RAM, SO, compilador, versión, commit, fecha, semilla y config presentes. | Sí |
| CP-082 | Verificación de optimalidad | RF-082 | Integración | Inyectar un buscador defectuoso que suma 1 al costo. | El benchmark reporta las discrepancias y termina con código ≠ 0. | Sí |
| CP-083 | Escenario dinámico | RF-083 | Integración | Barricada en el tick 30 sobre la ruta. | Replanificaciones ≥ 1 registradas para ambos algoritmos. | Sí |
| CP-084 | Experimento aislado/coordinado | RF-084 | Sistema | `--coord-exp 7 --reps 10`. | 20 simulaciones y métricas de RF-084 en el CSV. | Sí |
| CP-085 | Optimización GA y SA | RF-085 | Sistema | GA 30×40 y SA 2000 iteraciones con semilla 11. | JSON con el mejor individuo; aptitud del mejor no decreciente (GA). | Sí |
| CP-086 | Semillas | RF-086 | Unitaria | Dos PCG32 con la misma semilla y otra distinta. | Secuencias idénticas y distintas respectivamente; flujos independientes. | Sí |
| CP-090 | Guardar | RF-090 | Integración | Guardar en la ranura 1. | Existe `saves/slot1.sav`; no queda archivo temporal. | Sí |
| CP-091 | Cargar | RF-091 | Integración | Guardar y cargar la ranura 1. | Hash de estado idéntico. | Sí |
| CP-092 | Configuración | RF-092 | Integración | Cambiar volumen y una tecla; reiniciar. | Cambios persistidos en los JSON. | Sí |
| CP-093 | Archivos inválidos | RF-093 | Integración | Cargar partida con 1 byte alterado, JSON mal formado y mapa con m = 0. | Mensajes descriptivos; valores por defecto; sin cierre. | Sí |

### 2.2 Requisitos no funcionales

| CP | Título | Verifica | Nivel | Procedimiento y datos | Resultado esperado | Auto |
|---|---|---|---|---|---|---|
| CP-101 | FPS nominales | RNF-001 | Rendimiento | CM-01, CM-02, CM-04. | p5 ≥ 60 FPS. | Parcial |
| CP-102 | Tiempo de tick | RNF-002 | Rendimiento | CM-02 con Tracy. | p99 de `Sim::tick` ≤ 4 ms. | Parcial |
| CP-103 | Tiempo de búsqueda e IA | RNF-003 | Rendimiento | Benchmark 128² con 25 % de obstáculos y CM-02. | A* p95 ≤ 1 ms; IA p95 ≤ 2 ms. | Sí |
| CP-104 | Uso de CPU | RNF-004 | Rendimiento | CM-02 con VSync, `tools/recursos.py`. | Promedio ≤ 35 % de un núcleo. | Parcial |
| CP-105 | Memoria | RNF-005 | Rendimiento | CM-02 y benchmark 256². | RSS pico ≤ 128 MB y ≤ 64 MB. | Sí |
| CP-106 | Asignaciones por frame | RNF-006 | Rendimiento | Compilación Profile con contador de asignaciones. | 0 asignaciones en p99 de los frames. | Sí |
| CP-107 | Almacenamiento | RNF-007 | Rendimiento | Medir el .zip, una partida y los registros de 30 min. | ≤ 50 MB, ≤ 64 KB, ≤ 20 MB. | Sí |
| CP-108 | Arranque | RNF-008 | Rendimiento | 5 arranques en frío en CM-01. | Menú ≤ 3 s; escena ≤ 2 s. | Parcial |
| CP-109 | Matriz de plataformas | RNF-009 | Sistema | CI en Windows (MSVC, MinGW) y Ubuntu (GCC, Clang). | 4/4 trabajos en verde. | Sí |
| CP-110 | Coexistencia | RNF-010 | Sistema | Traza de archivos durante 5 min. | 0 escrituras fuera de la carpeta permitida. | No |
| CP-111 | Aprendizaje del tutorial | RNF-011 | Aceptación | 5 personas nuevas juegan la Escena 1. | Tutorial ≤ 3 min; ≥ 80 % superan la escena sin ayuda. | No |
| CP-112 | Operabilidad | RNF-012 | Sistema | Reasignar todas las acciones; medir latencia de entrada. | 100 % reasignables; ≤ 33 ms. | Parcial |
| CP-113 | Legibilidad y color | RNF-013 | Sistema | Capturas a escala 100 % y 200 %; simulador de daltonismo. | ≥ 12 px; contraste ≥ 4,5:1; estados con icono o letra. | No |
| CP-114 | Errores del usuario | RNF-014 | Sistema | Salir sin guardar; acciones inválidas. | Confirmación y mensajes en el 100 % de los casos. | Parcial |
| CP-115 | Resistencia y sanitizadores | RNF-015 | Sistema | 30 min a 4×; suite con ASan + UBSan. | 0 cierres; 0 errores. | Sí |
| CP-116 | Corpus inválido y fuzzing | RNF-016 | Sistema | ≥ 30 archivos inválidos; libFuzzer 10 min. | 100 % manejados sin cierre. | Sí |
| CP-117 | Guardado atómico | RNF-017 | Sistema | 100 interrupciones durante el guardado. | 100 % de partidas previas cargables. | Sí |
| CP-118 | Latencia de reacción | RNF-018 | Rendimiento | CM-02; analizar el log de IA con `tools/latencia_ia.py`. | p95 ≤ 200 ms y máximo ≤ 500 ms. | Sí |
| CP-119 | Optimalidad en benchmark | RNF-019 | Rendimiento | Benchmark completo ≥ 800 consultas. | 100 % de coincidencia de costos. | Sí |
| CP-120 | Determinismo | RNF-020 | Sistema | 10 pares de ejecuciones de 10 000 ticks. | Hash idéntico en el 100 % de los ticks. | Sí |
| CP-121 | Integridad por mutación | RNF-021 | Unitaria | 1000 mutaciones de 1 byte sobre una partida. | ≥ 99,9 % detectadas. | Sí |
| CP-122 | Sin red ni código externo | RNF-022 | Sistema | `strace -e trace=network`; revisión de dependencias. | 0 sockets; 0 CVE altas. | Parcial |
| CP-123 | Dominio sin raylib | RNF-023 | Sistema | `tools/check_deps.sh` en CI. | 0 includes prohibidos. | Sí |
| CP-124 | Cobertura | RNF-024 | Sistema | Trabajo `coverage`. | ≥ 80 % líneas y ≥ 70 % ramas en dominio; suite ≤ 60 s. | Sí |
| CP-125 | Análisis estático | RNF-025 | Sistema | Compilación estricta, clang-tidy, lizard, Doxygen. | 0 advertencias; CCN ≤ 15; 100 % API documentada. | Sí |
| CP-126 | Ejercicio de extensibilidad | RNF-026 | Sistema | Añadir BFS y agente «Corredor». | ≤ 1 archivo existente modificado. | No |
| CP-127 | Pila de recursión | RNF-027 | Unitaria | Grafo completo de 200 agentes para alertas; GOAP con 20 acciones; quadtree con 10⁴ puntos coincidentes. | Profundidades ≤ 3, 6 y 8; `-fstack-usage` × profundidad ≤ 64 KB. | Sí |
| CP-128 | Instalabilidad | RNF-028 | Sistema | Clon limpio en CI y .zip en equipo limpio. | 2 comandos, ≤ 5 min; el .zip arranca. | Parcial |
| CP-129 | Resoluciones | RNF-029 | Sistema | 5 resoluciones de la lista. | Sin deformación ni recortes. | No |
| CP-130 | Escalabilidad | RNF-030 | Rendimiento | n ∈ {25, 50, 100, 150, 200} en 256², 5 reps. | Pendiente log-log ≤ 1,3; ≤ 5 ms con 200. | Sí |
| CP-131 | Fotosensibilidad | RNF-031 | Sistema | Grabar Escenas 3 y 4; analizar destellos. | ≤ 3 destellos/s. | No |
| CP-132 | Validación de CSV | RNF-032 | Unitaria | `tools/validar_csv.py` sobre la salida de CP-080. | 100 % de campos presentes. | Sí |

### 2.3 Aceptación de extremo a extremo

| CP | Título | Verifica | Nivel | Procedimiento y datos | Resultado esperado | Auto |
|---|---|---|---|---|---|---|
| CP-201 | Prueba de aceptación del docente: señuelo y barricada | RF-013, RF-014, RF-025, RF-041, RF-050, RF-052 | Aceptación | Escena 4 con semilla registrada: (1) lanzar un señuelo lejos de Mara; (2) colocar una barricada en la ruta de la horda. Repetir con F4 en aislado. | Los agentes que perciben el señuelo cambian de objetivo y lo registran; los que no lo perciben no cambian (sin omnisciencia); los afectados por la barricada replanifican o la atacan. | Sí |
| CP-202 | Partida completa de principio a fin | RF-001 a RF-017 | Aceptación | Jugar las 5 escenas con el guion `guion_victoria.json` y con `guion_derrota.json`. | Victoria y derrota alcanzables; cada escena muestra su comportamiento inteligente del storyboard. | Sí |
| CP-203 | Demostración en vivo para la sustentación | RF-070 a RF-075, RF-056 | Aceptación | Guion de demo de 15 min: panel, F2/F3, F4, pausa y paso a paso. | El evaluador puede seguir una decisión desde la percepción hasta la acción. | No |
| CP-204 | Comportamientos del storyboard por escena | RF-045, RF-046, RF-047, RF-051, RF-050, RF-054 | Aceptación | Ejecutar cada escena en modo observación con la semilla 42. | Esc. 1 exploración sin información global; Esc. 2 investigan y alertan; Esc. 3 se distribuyen; Esc. 4 reconsideran ante señuelos; Esc. 5 eligen perseguir, explorar o interceptar. | Parcial |

## 3. Plantilla para registrar una ejecución

| Campo | Valor |
|---|---|
| CP | CP-nnn |
| Fecha y hora | |
| Versión / commit | |
| Equipo (CPU, RAM, SO) | |
| Semilla | |
| Resultado | Aprobado / Fallido / Bloqueado |
| Evidencia | Ruta al log, CSV o captura en `results/` |
| Defecto asociado | Issue de GitHub |

## 4. Resumen

<!-- generado:resumen inicio -->
| Grupo | Casos | Pasan | Parciales | Pendientes |
|---|---|---|---|---|
| Funcionales (CP-0nn) | 68 | 59 | 4 | 5 |
| No funcionales (CP-1nn) | 32 | 6 | 13 | 13 |
| Aceptación (CP-2nn) | 4 | 0 | 2 | 2 |
| **Total** | **104** | **65** | **19** | **20** |
<!-- generado:resumen fin -->

Los pendientes funcionales son los de RF-001, RF-006, RF-015 y RF-048, que no están implementados, y CP-040.
La mayoría de los pendientes no funcionales necesitan una herramienta de medición que no se ha escrito (FPS,
CPU, arranque, latencia) o una prueba con personas (CP-111, CP-113). La tabla también
la genera `tools/trazabilidad.py`.

## 5. Evidencia por caso de prueba

Tabla generada por `python tools/trazabilidad.py generar` desde [trazabilidad.csv](trazabilidad.csv) y los
`TEST_CASE` de `tests/`. *Relacionada* indica una prueba etiquetada con el requisito del mismo número que no
cita el CP en su nombre.

<!-- generado:evidencia inicio -->
| CP | Verifica | Estado | Pruebas automatizadas (archivo: nombre) | Notas |
|---|---|---|---|---|
| CP-001 | RF-001 | Pendiente | — | RF-001 pendiente. |
| CP-002 | RF-002 | Pasa | `test_partida.cpp`: CP-002 Velocidad de Mara por modo |  |
| CP-003 | RF-003 | Pasa | `test_partida.cpp`: CP-003 Ruido de Mara: 2 eventos por segundo con radio segun el modo |  |
| CP-004 | RF-004 | Pasa | `test_partida.cpp`: CP-004 Mara no atraviesa paredes y se desliza a lo largo de ellas<br>`test_partida.cpp`: CP-045 Al explorar, el infectado recorre el mapa sin salirse de las celdas libres *(relacionada)* |  |
| CP-005 | RF-005 | Pasa | `test_partida.cpp`: CP-005 Recoger un objeto solo a distancia 1 |  |
| CP-006 | RF-006 | Pendiente | — | RF-006 pendiente. |
| CP-007 | RF-007 | Pasa | `test_partida.cpp`: CP-007 Activar la radio con los tres objetos manteniendo interactuar<br>`test_superviviente.cpp`: HU-13 Si Mara instala la radio el superviviente ayuda y tarda menos *(relacionada)* |  |
| CP-008 | RF-008 | Parcial | `test_partida.cpp`: CP-007 Activar la radio con los tres objetos manteniendo interactuar | Falta el caso de daño. |
| CP-009 | RF-009 | Pasa | `test_extraccion_refugio.cpp`: CP-009 La zona de extraccion sigue abierta a 59 s y esta cerrada a 61 s |  |
| CP-010 | RF-010 | Pasa | `test_extraccion_refugio.cpp`: CP-010 Llegar a la zona abierta es la victoria; cerrada, la partida continua<br>`test_partida.cpp`: CP-007 Activar la radio con los tres objetos manteniendo interactuar |  |
| CP-011 | RF-011 | Pasa | `test_extraccion_refugio.cpp`: CP-011 Con 5 de salud y un infectado atacando, en 1 s Mara cae<br>`test_partida.cpp`: CP-011 Derrota cuando la salud de Mara llega a 0 |  |
| CP-012 | RF-012 | Pasa | `test_extraccion_refugio.cpp`: CP-012 Con integridad 3 un golpe de la horda inutiliza el refugio |  |
| CP-013 | RF-013 | Pasa | `test_senuelos_barricadas.cpp`: CP-013 Senuelo a 6 y a 12 celdas: cae a 6 y a 8, y suena 10 veces con radio 8<br>`test_senuelos_barricadas.cpp`: CP-013 El senuelo no atraviesa paredes: cae en la ultima celda libre<br>`test_senuelos_barricadas.cpp`: CP-106 Senuelos, barricadas y memoria no asignan memoria en el bucle *(relacionada)* |  |
| CP-014 | RF-014 | Pasa | `test_senuelos_barricadas.cpp`: CP-014 Barricada en celda libre adyacente; la ocupada se rechaza<br>`test_senuelos_barricadas.cpp`: CP-106 Senuelos, barricadas y memoria no asignan memoria en el bucle *(relacionada)* |  |
| CP-015 | RF-015 | Pendiente | — | RF-015 pendiente. |
| CP-016 | RF-016 | Parcial | — | Manual: capturas en la carpeta capturas/ del proyecto. |
| CP-017 | RF-017 | Pasa | `test_tutorial.cpp`: CP-017 las indicaciones salen en orden y se retiran al hacer la accion<br>`test_tutorial.cpp`: CP-017 saltar el tutorial quita las indicaciones<br>`test_tutorial.cpp`: RF-017 con el tutorial desactivado no hay indicaciones *(relacionada)* |  |
| CP-020 | RF-020 | Pasa | `test_grafo.cpp`: CP-020 Grafo 8-conexo sin cortar esquinas |  |
| CP-021 | RF-021 | Pasa | `test_busqueda.cpp`: CP-021 / CP-022 Dijkstra y A* son optimos frente a Bellman-Ford<br>`test_busqueda.cpp`: Reutilizar el buscador no contamina busquedas siguientes *(relacionada)*<br>`test_coordinacion_horda.cpp`: CP-028 El campo de flujo da el costo optimo desde cada celda *(relacionada)*<br>`test_planificador_rutas.cpp`: RF-026 La busqueda reanudable da el mismo resultado que la busqueda de una vez *(relacionada)* |  |
| CP-022 | RF-022 | Pasa | `test_busqueda.cpp`: CP-021 / CP-022 Dijkstra y A* son optimos frente a Bellman-Ford<br>`test_busqueda.cpp`: CP-022 La heuristica octil es admisible y consistente<br>`test_busqueda.cpp`: CP-106 A* reutilizando el resultado no asigna memoria *(relacionada)*<br>`test_grafo.cpp`: Distancia octil *(relacionada)* |  |
| CP-023 | RF-023 | Pasa | `test_busqueda.cpp`: CP-023 Metricas de busqueda completas<br>`test_comparacion.cpp`: CP-074 F3 compara la ultima consulta del agente seleccionado *(relacionada)*<br>`test_monitoreo.cpp`: CP-070 El agente seleccionado expone objetivo, estado, percepcion, ruta y metricas de A* *(relacionada)* |  |
| CP-024 | RF-024 | Pasa | `test_comparacion.cpp`: CP-074 F2 alterna el algoritmo de todos los agentes *(relacionada)*<br>`test_comparacion.cpp`: CP-106 La partida con Dijkstra es determinista y no asigna memoria *(relacionada)* | Cubierto por las pruebas de F2 y de Dijkstra en la partida. |
| CP-025 | RF-025 | Pasa | `test_busqueda.cpp`: CP-025 Una barricada cambia la ruta en la siguiente busqueda<br>`test_senuelos_barricadas.cpp`: CP-025 Barricada en la ruta con rodeo barato: replanifica sin pasar por ella<br>`test_benchmark.cpp`: CP-083 Barricada en el tick 30 sobre la ruta: ambos algoritmos replanifican *(relacionada)*<br>`test_planificador_rutas.cpp`: RF-025 Una busqueda a medias vuelve a empezar si cambia el mapa o el algoritmo *(relacionada)* |  |
| CP-026 | RF-026 | Pasa | `test_planificador_rutas.cpp`: CP-026 100 solicitudes en el mismo tick: 4000 nodos por tick y 10 ticks como maximo<br>`test_planificador_rutas.cpp`: CP-026 Horda de 100 infectados en 128x128 sin superar el presupuesto<br>`test_planificador_rutas.cpp`: RF-026 La busqueda reanudable da el mismo resultado que la busqueda de una vez *(relacionada)*<br>`test_planificador_rutas.cpp`: RF-026 La cola atiende por prioridad y antiguedad sin dejar a nadie esperando siempre *(relacionada)*<br>`test_planificador_rutas.cpp`: RF-025 Una busqueda a medias vuelve a empezar si cambia el mapa o el algoritmo *(relacionada)* |  |
| CP-027 | RF-027 | Pasa | `test_busqueda.cpp`: CP-027 Destino encerrado devuelve SinRuta<br>`test_planificador_rutas.cpp`: CP-027 La busqueda reanudable termina en SinRuta sin bloquear<br>`test_planificador_rutas.cpp`: CP-027 Un infectado con el destino encerrado cambia de comportamiento sin bloquear el juego |  |
| CP-028 | RF-028 | Pasa | `test_coordinacion_horda.cpp`: CP-028 El campo de flujo da el costo optimo desde cada celda<br>`test_coordinacion_horda.cpp`: CP-028 Horda con el mismo destino: un campo de flujo y ningun A* individual<br>`test_coordinacion_horda.cpp`: CP-106 Alertas y campo de flujo tampoco asignan memoria en el bucle *(relacionada)* |  |
| CP-029 | RF-029 | Pasa | `test_horda_coordinada.cpp`: CP-029 Steering: dos agentes que se cruzan no se solapan y no pasan de la velocidad maxima<br>`test_horda_coordinada.cpp`: CP-029 La evitacion aleja de las paredes cercanas<br>`test_horda_coordinada.cpp`: CP-106 Contract Net, reparto de accesos y steering no asignan memoria en el bucle *(relacionada)* |  |
| CP-030 | RF-030 | Pasa | `test_quadtree.cpp`: CP-030 Quadtree coincide con fuerza bruta<br>`test_planificador_rutas.cpp`: CP-026 Horda de 100 infectados en 128x128 sin superar el presupuesto *(relacionada)*<br>`test_quadtree.cpp`: CP-127 Quadtree con 10000 puntos coincidentes no supera la profundidad maxima *(relacionada)*<br>`test_quadtree.cpp`: Quadtree rechaza puntos fuera de los limites y se puede limpiar *(relacionada)* |  |
| CP-031 | RF-031 | Pasa | `test_grafo.cpp`: CP-031 Multiplicadores de terreno en [1,10] |  |
| CP-040 | RF-040 | Pendiente | `test_partida.cpp`: CP-044 Un infectado ve a Mara, la persigue con A* y la ataca *(relacionada)* | Etapas sin instrumentar. |
| CP-041 | RF-041 | Pasa | `test_percepcion.cpp`: CP-041 Vision: cono, radio y linea de vision con Bresenham | La inspección del ECS no aplica (ADR-0018). |
| CP-042 | RF-042 | Pasa | `test_percepcion.cpp`: CP-042 Oido: cada pared cruzada resta 2 celdas de alcance |  |
| CP-043 | RF-043 | Pasa | `test_senuelos_barricadas.cpp`: CP-043 Memoria: 20 evidencias dejan las 16 de mayor confianza y la confianza cae a la mitad en 10 s<br>`test_senuelos_barricadas.cpp`: CP-106 Senuelos, barricadas y memoria no asignan memoria en el bucle *(relacionada)* |  |
| CP-044 | RF-044 | Pasa | `test_maquina_estados.cpp`: CP-044 Transiciones de la FSM del infectado: una fila por transicion<br>`test_maquina_estados.cpp`: CP-044 Cada estado y motivo tiene nombre para el registro<br>`test_partida.cpp`: CP-044 Un infectado ve a Mara, la persigue con A* y la ataca<br>`test_tacticas.cpp`: CP-054 La FSM sigue a la tactica elegida *(relacionada)* |  |
| CP-045 | RF-045 | Parcial | `test_partida.cpp`: CP-045 Al explorar, el infectado recorre el mapa sin salirse de las celdas libres | No mide la cobertura. |
| CP-046 | RF-046 | Pasa | `test_coordinacion_horda.cpp`: CP-047 Quien ve a Mara alerta en cadena a los que estan tras el muro *(relacionada)* | Fila «Explorar + ruido» de CP-044. |
| CP-047 | RF-047 | Pasa | `test_coordinacion_horda.cpp`: CP-047 Quien ve a Mara alerta en cadena a los que estan tras el muro<br>`test_monitoreo.cpp`: CP-073 Una alerta en la cadena de CP-047 registra emisor, receptor, tipo, salto y confianza<br>`test_propagador_alertas.cpp`: CP-047 Cadena A-B-C-D-E: la alerta llega hasta el salto 3<br>`test_propagador_alertas.cpp`: CP-047 Ciclo X-Y-Z: cada agente recibe la alerta una sola vez<br>`test_coordinacion_horda.cpp`: CP-084 Sin modo coordinado nadie avisa y los de detras del muro siguen explorando *(relacionada)*<br>`test_coordinacion_horda.cpp`: CP-106 Alertas y campo de flujo tampoco asignan memoria en el bucle *(relacionada)*<br>`test_propagador_alertas.cpp`: CP-127 Grafo completo de 200 agentes: profundidad acotada y sin duplicados *(relacionada)*<br>`test_propagador_alertas.cpp`: Profundidad 0: el origen no avisa a nadie *(relacionada)* |  |
| CP-048 | RF-048 | Pendiente | — | RF-048 pendiente. |
| CP-049 | RF-049 | Pasa | `test_coordinacion_horda.cpp`: CP-047 Quien ve a Mara alerta en cadena a los que estan tras el muro<br>`test_horda_coordinada.cpp`: CP-049 Contract Net adjudica cada tarea a la puja mas baja y una por agente<br>`test_horda_coordinada.cpp`: CP-049 En la partida el que ve a Mara reparte interceptar, flanquear y perseguir<br>`test_horda_coordinada.cpp`: CP-106 Contract Net, reparto de accesos y steering no asignan memoria en el bucle *(relacionada)*<br>`test_monitoreo.cpp`: CP-073 Contract Net registra cada anuncio, puja y adjudicacion *(relacionada)* |  |
| CP-050 | RF-050 | Pasa | `test_senuelos_barricadas.cpp`: CP-050 Un senuelo cercano supera a una evidencia debil y cambia el objetivo en 5 ticks de IA<br>`test_senuelos_barricadas.cpp`: CP-050 Con Mara a la vista un senuelo no le hace cambiar |  |
| CP-051 | RF-051 | Pasa | `test_horda_coordinada.cpp`: CP-051 El reparto de accesos respeta el maximo por celda y minimiza distancias<br>`test_horda_coordinada.cpp`: CP-051 Doce perseguidores durante 10 s: max 2 por celda de llegada y 3 o mas accesos<br>`test_horda_coordinada.cpp`: CP-106 Contract Net, reparto de accesos y steering no asignan memoria en el bucle *(relacionada)* |  |
| CP-052 | RF-052 | Pasa | `test_senuelos_barricadas.cpp`: CP-052 Barricada sin alternativa: AtacarBarricada baja 10 por segundo y al romperla sigue<br>`test_senuelos_barricadas.cpp`: CP-052 Rodeo de mas del doble: ataca la barricada en vez de rodear<br>`test_senuelos_barricadas.cpp`: CP-025 Barricada en la ruta con rodeo barato: replanifica sin pasar por ella *(relacionada)* |  |
| CP-053 | RF-053 | Pasa | `test_tacticas.cpp`: CP-053 El punto de corte esta en la ruta prevista de Mara, 2 s o mas por delante<br>`test_tacticas.cpp`: CP-053 En la partida el infectado cerca del cruce elige interceptar y registra la tactica<br>`test_horda_coordinada.cpp`: CP-049 En la partida el que ve a Mara reparte interceptar, flanquear y perseguir *(relacionada)* |  |
| CP-054 | RF-054 | Pasa | `test_tacticas.cpp`: CP-054 La tactica depende de la evidencia: los 4 ejemplos de HU-12<br>`test_tacticas.cpp`: CP-054 La FSM sigue a la tactica elegida<br>`test_tacticas.cpp`: CP-053 En la partida el infectado cerca del cruce elige interceptar y registra la tactica *(relacionada)* |  |
| CP-055 | RF-055 | Pasa | `test_extraccion_refugio.cpp`: CP-055 Un infectado en Atacar quita 10 de salud por segundo<br>`test_extraccion_refugio.cpp`: CP-011 Con 5 de salud y un infectado atacando, en 1 s Mara cae *(relacionada)* |  |
| CP-056 | RF-056 | Pasa | `test_aislados_coordinados.cpp`: CP-056 Modo aislado sin alertas ni Contract Net e identico hasta la primera deteccion<br>`test_aislados_coordinados.cpp`: CP-056 F4 en vivo corta la comunicacion a partir de ese momento |  |
| CP-060 | RF-060 | Pasa | `test_superviviente.cpp`: CP-060 El superviviente elige la meta por utilidad con histeresis<br>`test_superviviente.cpp`: HU-13 Con el refugio al 30 % el superviviente va a por tablas y repara *(relacionada)*<br>`test_superviviente.cpp`: HU-13 Ante un infectado a 2 celdas el superviviente se pone a salvo *(relacionada)*<br>`test_superviviente.cpp`: HU-13 Si Mara instala la radio el superviviente ayuda y tarda menos *(relacionada)* |  |
| CP-061 | RF-061 | Pasa | `test_superviviente.cpp`: CP-061 GOAP: sin tablas y con meta RefugioSeguro el plan es ir, recoger, volver y reparar<br>`test_superviviente.cpp`: CP-127 GOAP con 20 acciones no pasa de 6 acciones de profundidad *(relacionada)*<br>`test_superviviente.cpp`: HU-13 Con el refugio al 30 % el superviviente va a por tablas y repara *(relacionada)* |  |
| CP-062 | RF-062 | Pasa | `test_superviviente.cpp`: CP-062 Arbol de comportamiento: secuencia, selector y decoradores propagan los estados<br>`test_superviviente.cpp`: CP-062 Los subarboles del superviviente caben en la profundidad maxima<br>`test_superviviente.cpp`: HU-13 Ante un infectado a 2 celdas el superviviente se pone a salvo *(relacionada)* |  |
| CP-063 | RF-063 | Pasa | `test_extraccion_refugio.cpp`: CP-063 Un infectado golpeando la puerta 2 s quita 10 de integridad<br>`test_superviviente.cpp`: CP-063 Reparando 2 s el superviviente suma 16 puntos con tope en el maximo<br>`test_superviviente.cpp`: HU-13 Con el refugio al 30 % el superviviente va a por tablas y repara *(relacionada)* |  |
| CP-064 | RF-064 | Pasa | `test_superviviente.cpp`: CP-064 Si el almacen queda bloqueado el superviviente replanifica en el siguiente tick<br>`test_monitoreo.cpp`: CP-072 El superviviente y las transiciones de la FSM tambien van al registro de IA *(relacionada)* |  |
| CP-070 | RF-070 | Pasa | `test_monitoreo.cpp`: CP-070 El agente seleccionado expone objetivo, estado, percepcion, ruta y metricas de A*<br>`test_monitoreo.cpp`: CP-106 Monitorear un agente y registrar en un sumidero no asigna memoria *(relacionada)* | Datos del dominio automatizados; vista revisada a mano. |
| CP-071 | RF-071 | Parcial | `test_monitoreo.cpp`: CP-071 Los nodos pintados son exactamente los nodos expandidos del panel<br>`test_monitoreo.cpp`: CP-071 Seleccionar un agente no cambia la simulacion | Falta dibujar la frontera. |
| CP-072 | RF-072 | Pasa | `test_monitoreo.cpp`: CP-072 Tres cambios de plan aparecen en la consola del panel y en logs/ia_<semilla>.log<br>`test_monitoreo.cpp`: CP-072 Si la carpeta de registros no se puede crear el juego sigue sin archivo<br>`test_monitoreo.cpp`: CP-072 El superviviente y las transiciones de la FSM tambien van al registro de IA<br>`test_senuelos_barricadas.cpp`: CP-072 Tres cambios de plan quedan registrados con tick, agente, plan anterior, nuevo y motivo<br>`test_maquina_estados.cpp`: CP-044 Cada estado y motivo tiene nombre para el registro *(relacionada)*<br>`test_monitoreo.cpp`: RegistroIa guarda las ultimas lineas en anillo y recorta las largas *(relacionada)*<br>`test_monitoreo.cpp`: CP-106 Monitorear un agente y registrar en un sumidero no asigna memoria *(relacionada)*<br>`test_senuelos_barricadas.cpp`: CP-050 Un senuelo cercano supera a una evidencia debil y cambia el objetivo en 5 ticks de IA *(relacionada)*<br>`test_tacticas.cpp`: CP-053 En la partida el infectado cerca del cruce elige interceptar y registra la tactica *(relacionada)* |  |
| CP-073 | RF-073 | Pasa | `test_monitoreo.cpp`: CP-073 Una alerta en la cadena de CP-047 registra emisor, receptor, tipo, salto y confianza<br>`test_monitoreo.cpp`: CP-073 Contract Net registra cada anuncio, puja y adjudicacion<br>`test_monitoreo.cpp`: CP-106 Monitorear un agente y registrar en un sumidero no asigna memoria *(relacionada)* |  |
| CP-074 | RF-074 | Pasa | `test_comparacion.cpp`: CP-074 Misma consulta (3,3)>(60,50) en 64x64: mismo costo y A* expande menos<br>`test_comparacion.cpp`: CP-074 La comparacion detecta costos distintos y consultas sin ruta<br>`test_comparacion.cpp`: CP-074 F2 alterna el algoritmo de todos los agentes<br>`test_comparacion.cpp`: CP-074 F3 compara la ultima consulta del agente seleccionado<br>`test_comparacion.cpp`: CP-074 Comparar con F3 no cambia la simulacion |  |
| CP-075 | RF-075 | Pasa | `test_control_simulacion.cpp`: CP-075 En pausa cada paso pedido ejecuta exactamente un tick<br>`test_control_simulacion.cpp`: CP-075 Velocidades 0.25x a 4x dan 15 a 240 ticks por segundo real<br>`test_control_simulacion.cpp`: CP-075 Un tiron largo se recorta a 0.25 s y la pausa no acumula tiempo<br>`test_control_simulacion.cpp`: CP-075 Mismo estado final tras 600 ticks a cualquier velocidad<br>`test_control_simulacion.cpp`: RF-075 Las teclas de control de la simulacion se pueden reasignar *(relacionada)* |  |
| CP-080 | RF-080 | Pasa | `test_benchmark.cpp`: CP-080 refugio --bench bench_mini.json escribe 800 filas con las columnas de RF-080<br>`test_benchmark.cpp`: CP-080 Misma configuracion y semilla 42 dan costos y nodos identicos<br>`test_benchmark.cpp`: CP-080 --dinamico escribe un segundo CSV junto al principal<br>`test_benchmark.cpp`: bench/config.json se lee con sus valores y las claves ausentes toman el defecto *(relacionada)*<br>`test_benchmark.cpp`: Una configuracion invalida da un mensaje que dice que corregir *(relacionada)* |  |
| CP-081 | RF-081 | Pasa | `test_benchmark.cpp`: CP-081 La cabecera registra equipo, compilacion, version, commit, fecha, semilla y config |  |
| CP-082 | RF-082 | Pasa | `test_benchmark.cpp`: CP-082 Un A* defectuoso que suma 1 al costo se reporta como discrepancia<br>`test_benchmark.cpp`: CP-080 refugio --bench bench_mini.json escribe 800 filas con las columnas de RF-080 *(relacionada)*<br>`test_comparacion.cpp`: CP-074 Misma consulta (3,3)>(60,50) en 64x64: mismo costo y A* expande menos *(relacionada)*<br>`test_comparacion.cpp`: CP-074 La comparacion detecta costos distintos y consultas sin ruta *(relacionada)* |  |
| CP-083 | RF-083 | Pasa | `test_benchmark.cpp`: CP-083 Barricada en el tick 30 sobre la ruta: ambos algoritmos replanifican<br>`test_benchmark.cpp`: CP-083 Un evento en una celda fija es el mismo para los dos algoritmos<br>`test_benchmark.cpp`: CP-080 --dinamico escribe un segundo CSV junto al principal *(relacionada)* |  |
| CP-084 | RF-084 | Pasa | `test_aislados_coordinados.cpp`: CP-084 Experimento con semilla 7 y 10 repeticiones da 20 simulaciones pareadas<br>`test_aislados_coordinados.cpp`: CP-084 El experimento es reproducible con la misma semilla<br>`test_aislados_coordinados.cpp`: CP-084 El tope de segundos corta cada simulacion<br>`test_aislados_coordinados.cpp`: CP-084 El guion de la jugadora recoge los objetos y activa la radio sin infectados<br>`test_aislados_coordinados.cpp`: CP-084 La huella de estado cambia con la simulacion y coincide con la misma semilla<br>`test_aislados_coordinados.cpp`: CP-084 refugio --coord-exp escribe el CSV y termina con codigo 0<br>`test_coordinacion_horda.cpp`: CP-084 Sin modo coordinado nadie avisa y los de detras del muro siguen explorando |  |
| CP-085 | RF-085 | Pasa | `test_optimizacion.cpp`: Codificar y decodificar los pesos ida y vuelta, recortando a [0, 1]<br>`test_optimizacion.cpp`: CP-085 GA: el mejor nunca empeora y se acerca al optimo<br>`test_optimizacion.cpp`: CP-085 GA: misma semilla mismo resultado; otra semilla otro<br>`test_optimizacion.cpp`: GA: el individuo inicial entra en la poblacion y valida sus datos<br>`test_optimizacion.cpp`: CP-085 SA: registra temperatura, solucion actual y mejor por iteracion<br>`test_optimizacion.cpp`: SA: acepta empeoramientos con calor y es reproducible<br>`test_optimizacion.cpp`: CP-085 lee opt/ga.json y opt/sa.json del repositorio<br>`test_optimizacion.cpp`: CP-085 GA con simulaciones escribe el JSON con el mejor y la evolucion<br>`test_optimizacion.cpp`: CP-085 SA con simulaciones escribe temperatura y soluciones por iteracion<br>`test_optimizacion.cpp`: CP-085 refugio --optimize escribe results/opt_<alg>_<semilla>.json<br>`test_optimizacion.cpp`: Genes: uno por peso de ia.json y los valores por defecto dentro del rango *(relacionada)*<br>`test_optimizacion.cpp`: Aptitud penaliza una horda demasiado dificil o demasiado facil *(relacionada)*<br>`test_optimizacion.cpp`: Configuracion de optimizacion invalida da un error claro *(relacionada)* |  |
| CP-086 | RF-086 | Pasa | `test_partida.cpp`: CP-086 La partida es determinista con la misma semilla y la misma entrada<br>`test_pcg32.cpp`: CP-086 PCG32 es reproducible con la misma semilla<br>`test_benchmark.cpp`: CP-080 Misma configuracion y semilla 42 dan costos y nodos identicos *(relacionada)*<br>`test_optimizacion.cpp`: CP-085 GA: misma semilla mismo resultado; otra semilla otro *(relacionada)*<br>`test_optimizacion.cpp`: SA: acepta empeoramientos con calor y es reproducible *(relacionada)*<br>`test_pcg32.cpp`: PCG32: enteroMenorQue respeta el limite *(relacionada)* |  |
| CP-090 | RF-090 | Pasa | `test_menu_pausa.cpp`: Menu: Esc abre y cierra, guardar pide la ranura<br>`test_persistencia.cpp`: CP-090 guardar escribe saves/slot1.sav sin dejar temporal |  |
| CP-091 | RF-091 | Pasa | `test_persistencia.cpp`: CP-091 guardar y cargar reproduce el estado exacto<br>`test_optimizacion.cpp`: Partida guardada conserva los pesos y lee el formato 1 *(relacionada)* |  |
| CP-092 | RF-092 | Pasa | `test_configuracion.cpp`: CP-092 cambiar volumen y una tecla se guarda en los JSON y se relee<br>`test_menu_pausa.cpp`: CP-092 opciones: volumen y tutorial desde el menu<br>`test_configuracion.cpp`: RF-092 config/ia.json del repositorio se lee sin errores *(relacionada)*<br>`test_configuracion.cpp`: RF-092 ia.json cambia los parametros y valida las reglas entre campos *(relacionada)*<br>`test_optimizacion.cpp`: ia.json: la seccion pesos se lee, se valida y se aplica *(relacionada)* |  |
| CP-093 | RF-093 | Pasa | `test_configuracion.cpp`: CP-093 JSON mal formado usa los valores por defecto con un mensaje<br>`test_configuracion.cpp`: CP-093 campos fuera de rango vuelven a su valor por defecto<br>`test_persistencia.cpp`: CP-093 rangos invalidos con CRC correcto se rechazan<br>`test_persistencia.cpp`: CP-093 ranura con un byte alterado avisa que esta danada<br>`test_benchmark.cpp`: Una configuracion invalida da un mensaje que dice que corregir *(relacionada)*<br>`test_monitoreo.cpp`: CP-072 Si la carpeta de registros no se puede crear el juego sigue sin archivo *(relacionada)*<br>`test_optimizacion.cpp`: ia.json: la seccion pesos se lee, se valida y se aplica *(relacionada)* |  |
| CP-101 | RNF-001 | Pendiente | — | Sin Tracy. |
| CP-102 | RNF-002 | Parcial | `test_planificador_rutas.cpp`: CP-026 100 solicitudes en el mismo tick: 4000 nodos por tick y 10 ticks como maximo *(relacionada)*<br>`test_planificador_rutas.cpp`: CP-026 Horda de 100 infectados en 128x128 sin superar el presupuesto *(relacionada)* | Solo búsqueda por tick (CP-026). |
| CP-103 | RNF-003 | Parcial | — | A* p95 548 µs en 128²; IA por tick sin medir. |
| CP-104 | RNF-004 | Pendiente | — |  |
| CP-105 | RNF-005 | Parcial | — | Benchmark 256²: 13,7 MB. |
| CP-106 | RNF-006 | Pasa | `test_busqueda.cpp`: CP-106 A* reutilizando el resultado no asigna memoria<br>`test_comparacion.cpp`: CP-106 La partida con Dijkstra es determinista y no asigna memoria<br>`test_coordinacion_horda.cpp`: CP-106 Alertas y campo de flujo tampoco asignan memoria en el bucle<br>`test_horda_coordinada.cpp`: CP-106 Contract Net, reparto de accesos y steering no asignan memoria en el bucle<br>`test_monitoreo.cpp`: CP-106 Monitorear un agente y registrar en un sumidero no asigna memoria<br>`test_partida.cpp`: CP-106 El paso de simulacion no asigna memoria tras la carga<br>`test_senuelos_barricadas.cpp`: CP-106 Senuelos, barricadas y memoria no asignan memoria en el bucle<br>`test_monitoreo.cpp`: RegistroIa guarda las ultimas lineas en anillo y recorta las largas *(relacionada)* |  |
| CP-107 | RNF-007 | Parcial | `test_persistencia.cpp`: CP-107 cabecera REFG y partida de 10 minutos de 64 KB o menos | Solo la partida guardada. |
| CP-108 | RNF-008 | Pendiente | — |  |
| CP-109 | RNF-009 | Parcial | — | CI: 3 de 4 trabajos (sin MinGW). |
| CP-110 | RNF-010 | Pendiente | — |  |
| CP-111 | RNF-011 | Pendiente | `test_tutorial.cpp`: RNF-011 el tutorial mide su duracion y no cambia la simulacion *(relacionada)* | Requiere 5 personas. |
| CP-112 | RNF-012 | Parcial | `test_menu_pausa.cpp`: CP-112 todas las acciones se reasignan desde el menu<br>`test_configuracion.cpp`: Reasignar una tecla usada intercambia las dos acciones *(relacionada)*<br>`test_control_simulacion.cpp`: RF-075 Las teclas de control de la simulacion se pueden reasignar *(relacionada)* | Reasignación sí; latencia no. |
| CP-113 | RNF-013 | Pendiente | — |  |
| CP-114 | RNF-014 | Parcial | `test_menu_pausa.cpp`: CP-114 cargar y salir con progreso sin guardar piden confirmacion | Confirmaciones sí; mensajes de acción inválida sin prueba. |
| CP-115 | RNF-015 | Parcial | — | CI: ASan + UBSan; falta la sesión de 30 min. |
| CP-116 | RNF-016 | Parcial | `test_configuracion.cpp`: CP-116 corpus de configuraciones invalidas sin cierres<br>`test_persistencia.cpp`: CP-116 archivos de partida truncados o vacios no cierran el juego | Corpus sí; libFuzzer no. |
| CP-117 | RNF-017 | Pasa | `test_persistencia.cpp`: CP-117 un guardado interrumpido no estropea la partida anterior |  |
| CP-118 | RNF-018 | Pendiente | — | Sin tools/latencia_ia.py. |
| CP-119 | RNF-019 | Pasa | `test_busqueda.cpp`: CP-021 / CP-022 Dijkstra y A* son optimos frente a Bellman-Ford *(relacionada)* | CI: el humo de benchmark falla con código 2 si un costo difiere. |
| CP-120 | RNF-020 | Parcial | `test_aislados_coordinados.cpp`: CP-084 El experimento es reproducible con la misma semilla *(relacionada)*<br>`test_aislados_coordinados.cpp`: CP-084 La huella de estado cambia con la simulacion y coincide con la misma semilla *(relacionada)*<br>`test_comparacion.cpp`: CP-074 Comparar con F3 no cambia la simulacion *(relacionada)*<br>`test_comparacion.cpp`: CP-106 La partida con Dijkstra es determinista y no asigna memoria *(relacionada)*<br>`test_control_simulacion.cpp`: CP-075 Mismo estado final tras 600 ticks a cualquier velocidad *(relacionada)*<br>`test_monitoreo.cpp`: CP-071 Seleccionar un agente no cambia la simulacion *(relacionada)*<br>`test_optimizacion.cpp`: Aptitud con simulaciones: determinista e independiente de los hilos *(relacionada)*<br>`test_partida.cpp`: CP-086 La partida es determinista con la misma semilla y la misma entrada *(relacionada)*<br>`test_pcg32.cpp`: CP-086 PCG32 es reproducible con la misma semilla *(relacionada)* | Varias pruebas de determinismo, no 10 × 10 000 ticks. |
| CP-121 | RNF-021 | Pasa | `test_persistencia.cpp`: CP-121 todas las mutaciones de 1 byte se detectan<br>`test_persistencia.cpp`: RNF-021 archivos de mas de 10 MB se rechazan sin leerlos *(relacionada)* |  |
| CP-122 | RNF-022 | Pendiente | — |  |
| CP-123 | RNF-023 | Pasa | — | CI: trabajo «Dominio sin raylib» (tools/check_deps.sh). |
| CP-124 | RNF-024 | Pendiente | — | Sin trabajo de cobertura. |
| CP-125 | RNF-025 | Parcial | — | Solo compilación con -Werror. |
| CP-126 | RNF-026 | Pendiente | — |  |
| CP-127 | RNF-027 | Pasa | `test_propagador_alertas.cpp`: CP-127 Grafo completo de 200 agentes: profundidad acotada y sin duplicados<br>`test_quadtree.cpp`: CP-127 Quadtree con 10000 puntos coincidentes no supera la profundidad maxima<br>`test_superviviente.cpp`: CP-127 GOAP con 20 acciones no pasa de 6 acciones de profundidad<br>`test_superviviente.cpp`: CP-062 Los subarboles del superviviente caben en la profundidad maxima *(relacionada)* |  |
| CP-128 | RNF-028 | Parcial | — | Sin .zip. |
| CP-129 | RNF-029 | Pendiente | — | RNF-029 pendiente. |
| CP-130 | RNF-030 | Pendiente | `test_horda_coordinada.cpp`: CP-051 Doce perseguidores durante 10 s: max 2 por celda de llegada y 3 o mas accesos *(relacionada)* |  |
| CP-131 | RNF-031 | Pendiente | — |  |
| CP-132 | RNF-032 | Parcial | `test_benchmark.cpp`: CP-080 refugio --bench bench_mini.json escribe 800 filas con las columnas de RF-080 | Sin tools/validar_csv.py. |
| CP-201 | RF-013 | Parcial | — | CP-025, CP-050 y CP-052 cubren cada parte; falta la escena guionizada. |
| CP-202 | RF-001 | Parcial | — | El guion de HU-18 recoge, activa la radio y llega a la extracción; sin guiones de victoria y derrota. |
| CP-203 | RF-070 | Pendiente | — | Manual, en la sustentación. |
| CP-204 | RF-045 | Pendiente | — | No hay escenas separadas. |
<!-- generado:evidencia fin -->
