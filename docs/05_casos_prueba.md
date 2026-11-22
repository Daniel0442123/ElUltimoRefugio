# 05 · Catálogo de casos de prueba

> Estructura ligera derivada de ISO/IEC/IEEE 29119-3 (especificación de casos de prueba).
> Cada requisito funcional `RF-nnn` tiene al menos el caso `CP-nnn`; cada no funcional `RNF-0nn`
> tiene el caso `CP-1nn`; los casos `CP-2nn` son pruebas de aceptación de extremo a extremo.

| Campo | Valor |
|---|---|
| Versión | 0.1.0 |
| Fecha | 2026-11-21 |
| Herramientas | Catch2 v3 (unitarias, integración y escenarios BDD), CTest, Tracy, ASan/UBSan, Valgrind, libFuzzer, scripts en `tools/` |

## 1. Niveles y estrategia

| Nivel | Alcance | Ubicación | Cuándo se ejecuta |
|---|---|---|---|
| Unitaria | Una clase o función del dominio, sin raylib | `tests/unit/` | Cada commit (CI) |
| Integración | Varios sistemas del dominio con el bus de eventos y adaptadores falsos | `tests/integration/` | Cada commit (CI) |
| Sistema | Ejecutable completo, con o sin ventana | `tests/system/` | Cada PR y nocturno |
| Rendimiento | Métricas de RNF en condiciones CM-01 a CM-05 | `bench/` | Nocturno y antes de cada entrega |
| Aceptación | Escenarios del docente y del storyboard | `tests/acceptance/` | Antes de cada entrega y en la sustentación |

* Las pruebas de dominio se ejecutan **sin ventana** gracias a la arquitectura hexagonal (ADR-0003).
* Los escenarios Gherkin de [02_historias_usuario.md](02_historias_usuario.md) se implementan con las macros
  `SCENARIO / GIVEN / WHEN / THEN` de Catch2, con el ID de la HU como etiqueta (`[HU-09]`).
* Todas las pruebas que usan aleatoriedad fijan semilla y la imprimen al fallar.
* Criterio de salida de una entrega: 100 % de los CP automatizados `M` aprobados, 0 defectos críticos abiertos.

## 2. Casos de prueba

Columna **Auto**: `Sí` automatizada en CI; `Parcial` automatizada la medición pero con revisión manual; `No` manual.

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

| Grupo | Casos |
|---|---|
| Funcionales | 68 |
| No funcionales | 32 |
| Aceptación | 4 |
| **Total** | **104** |
| Automatizados por completo | 83 (80 %) |
