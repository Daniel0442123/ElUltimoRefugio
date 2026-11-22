# 06 · Matriz de trazabilidad

> Trazabilidad bidireccional según ISO/IEC/IEEE 29148:2018 §5.2.8 y §6.6:
> **fuente (GUIA/STB) → StR → HU → RF → RNF → CP**, y en sentido inverso.
> Este documento se genera desde una sola fuente de datos; el archivo [trazabilidad.csv](trazabilidad.csv)
> contiene las mismas relaciones para importarlas en Jira, Azure DevOps o GitHub Projects.

| Campo | Valor |
|---|---|
| Versión | 0.1.0 |
| Fecha | 2026-11-21 |

## 1. Cobertura

| Indicador | Valor | Objetivo |
|---|---|---|
| HU con al menos un RF | 23/23 | 100 % |
| RF con al menos una HU | 68/68 | 100 % |
| RF con al menos un CP | 68/68 | 100 % |
| RNF con al menos un CP | 32/32 | 100 % |
| RNF enlazados con algún RF | 26/32 | Los restantes son transversales (§5) |

## 2. Fuente del docente → StR → HU

| Cláusula de la fuente | Exigencia | StR | HU |
|---|---|---|---|
| GUIA §1, §2 | Juego con personajes autónomos que perciben, planifican, deciden, ejecutan y se adaptan | StR-05, StR-06 | HU-08, HU-09, HU-11, HU-12, HU-13 |
| GUIA §4 | Ciclo entorno → percepción → memoria → planificación → decisión → ejecución → evaluación; reconsiderar planes | StR-05, StR-06 | HU-08, HU-11, HU-13 |
| GUIA §5 Dijkstra, A\*, grafos | Costo mínimo, f = g + h, admisibilidad y consistencia, vértices/aristas/costos | StR-09 | HU-16, HU-17 |
| GUIA §5 Recursividad | Caso base, progreso, análisis de pila | StR-07, StR-14 | HU-09 |
| GUIA §5 Heurísticas | Estimar costos o priorizar opciones | StR-09, StR-08 | HU-11, HU-12, HU-16 |
| GUIA §5 Memoria y autonomía | Registrar eventos y decidir | StR-06 | HU-08, HU-09, HU-15 |
| GUIA §5 Comunicación multiagente | Cuando corresponda | StR-07, StR-08 | HU-09, HU-10, HU-12 |
| GUIA §5 Complejidad | O() en función de V, E, profundidad, ramificación; espacio y recursión | StR-14 | HU-17 |
| GUIA §6 Panel | Objetivo, estado, percepción, ruta, algoritmo, heurística, costo, nodos, tiempo | StR-10 | HU-14, HU-22 |
| GUIA §6 Registro | Cambios de plan y mensajes | StR-11 | HU-15 |
| GUIA §6 Comparación | Dijkstra vs A\* con datos reales del juego | StR-09 | HU-16 |
| GUIA §7 Experimentación | Mismos mapas, varios tamaños, ≥ 5 reps, equipo, semilla, tiempos, costo, nodos, memoria, cambios del entorno | StR-12 | HU-17 |
| GUIA §8 Entregables | Juego, repositorio, documentación, análisis, registros, demo | StR-15, StR-19 | HU-17, HU-22 |
| STB §01 Historia | Reunir medicamentos, combustible y batería; activar radio; extracción; derrota | StR-01, StR-02, StR-03 | HU-01 a HU-05 |
| STB §01 Personajes | Mara, exploradores, perseguidores, superviviente | StR-18 | HU-08, HU-13 |
| STB Escena 1 | Exploran sin información global | StR-05 | HU-08, HU-20 |
| STB Escena 2 | Investigan el origen y transmiten alertas | StR-07 | HU-02, HU-09 |
| STB Escena 3 | Distribuyen aproximaciones, no se concentran en una celda | StR-08 | HU-10, HU-23 |
| STB Escena 4 | Reconsideran objetivos ante señuelos | StR-04, StR-06 | HU-03, HU-06, HU-07, HU-11, HU-13 |
| STB Escena 5 | Perseguir, explorar o interceptar según evidencias | StR-08 | HU-04, HU-12 |
| STB §03 Diseño | Comparar aislados y coordinados con la misma semilla | StR-13 | HU-18 |
| STB §03 Recursividad | Propagación acotada de alertas con visitados | StR-07 | HU-09 |
| STB §03 Prueba de aceptación | Señuelo y barricada sin omnisciencia | StR-04, StR-05 | HU-06, HU-07, HU-11 |
| Objetivo general «optimización» | Resolver problemas de optimización | StR-17 | HU-21 |

## 3. Matriz HU → RF → RNF → CP

| HU | Título | RF | RNF relacionados | Casos de prueba |
|---|---|---|---|---|
| HU-01 | Moverse en sigilo | RF-001, RF-002, RF-003, RF-004, RF-016 | RNF-001, RNF-008, RNF-012, RNF-013, RNF-015, RNF-016, RNF-020, RNF-029 | CP-001, CP-002, CP-003, CP-004, CP-016, CP-101, CP-108, CP-112, CP-113, CP-115, CP-116, CP-120, CP-129, CP-202 |
| HU-02 | Recoger objetos de misión | RF-005, RF-006, RF-015, RF-016 | RNF-012, RNF-013, RNF-020, RNF-029 | CP-005, CP-006, CP-015, CP-016, CP-112, CP-113, CP-120, CP-129, CP-202 |
| HU-03 | Activar la radio del refugio | RF-007, RF-008 | RNF-014 | CP-007, CP-008, CP-114, CP-202 |
| HU-04 | Escapar por la extracción | RF-009, RF-010 | RNF-013, RNF-020 | CP-009, CP-010, CP-113, CP-120, CP-202 |
| HU-05 | Perder la partida | RF-011, RF-012, RF-055, RF-063 | RNF-020 | CP-011, CP-012, CP-055, CP-063, CP-120, CP-202 |
| HU-06 | Lanzar un señuelo sonoro | RF-013 | RNF-014 | CP-013, CP-114, CP-201, CP-202 |
| HU-07 | Colocar una barricada | RF-014, RF-025, RF-052 | RNF-002, RNF-014 | CP-014, CP-025, CP-052, CP-102, CP-114, CP-201, CP-202 |
| HU-08 | Infectados que exploran sin información global | RF-040, RF-041, RF-042, RF-043, RF-044, RF-045, RF-048 | RNF-002, RNF-005, RNF-006, RNF-023, RNF-025, RNF-026 | CP-040, CP-041, CP-042, CP-043, CP-044, CP-045, CP-048, CP-102, CP-105, CP-106, CP-123, CP-125, CP-126, CP-201, CP-204 |
| HU-09 | Investigar ruidos y propagar alertas | RF-042, RF-043, RF-046, RF-047, RF-073 | RNF-002, RNF-005, RNF-006, RNF-007, RNF-018, RNF-027 | CP-042, CP-043, CP-046, CP-047, CP-073, CP-102, CP-105, CP-106, CP-107, CP-118, CP-127, CP-203, CP-204 |
| HU-10 | Horda que reparte la aproximación | RF-029, RF-030, RF-049, RF-051 | RNF-002, RNF-027, RNF-030 | CP-029, CP-030, CP-049, CP-051, CP-102, CP-127, CP-130, CP-204 |
| HU-11 | Horda que reconsidera objetivos ante señuelos | RF-025, RF-043, RF-050, RF-052, RF-072 | RNF-002, RNF-005, RNF-006, RNF-007, RNF-018 | CP-025, CP-043, CP-050, CP-052, CP-072, CP-102, CP-105, CP-106, CP-107, CP-118, CP-201, CP-203, CP-204 |
| HU-12 | Perseguir, explorar o interceptar según evidencias | RF-049, RF-053, RF-054 | RNF-002, RNF-025 | CP-049, CP-053, CP-054, CP-102, CP-125, CP-204 |
| HU-13 | Superviviente de la radio autónomo | RF-060, RF-061, RF-062, RF-063, RF-064 | RNF-002, RNF-020, RNF-027 | CP-060, CP-061, CP-062, CP-063, CP-064, CP-102, CP-120, CP-127 |
| HU-14 | Panel de monitoreo del agente | RF-023, RF-070, RF-071 | RNF-001, RNF-013, RNF-032 | CP-023, CP-070, CP-071, CP-101, CP-113, CP-132, CP-203 |
| HU-15 | Registro de planes y mensajes | RF-072, RF-073 | RNF-007 | CP-072, CP-073, CP-107, CP-203 |
| HU-16 | Comparar Dijkstra y A\* en vivo | RF-020, RF-021, RF-022, RF-023, RF-024, RF-031, RF-074 | RNF-003, RNF-005, RNF-016, RNF-019, RNF-026, RNF-032 | CP-020, CP-021, CP-022, CP-023, CP-024, CP-031, CP-074, CP-103, CP-105, CP-116, CP-119, CP-126, CP-132, CP-203 |
| HU-17 | Benchmark reproducible de búsqueda | RF-080, RF-081, RF-082, RF-083, RF-086 | RNF-005, RNF-019, RNF-020, RNF-032 | CP-080, CP-081, CP-082, CP-083, CP-086, CP-105, CP-119, CP-120, CP-132 |
| HU-18 | Comparar agentes aislados y coordinados | RF-056, RF-084 | RNF-020 | CP-056, CP-084, CP-120, CP-203 |
| HU-19 | Guardar, cargar y configurar | RF-090, RF-091, RF-092, RF-093 | RNF-007, RNF-012, RNF-016, RNF-017, RNF-020, RNF-021 | CP-090, CP-091, CP-092, CP-093, CP-107, CP-112, CP-116, CP-117, CP-120, CP-121 |
| HU-20 | Tutorial de sigilo | RF-017 | RNF-011 | CP-017, CP-111, CP-202 |
| HU-21 | Optimizar parámetros de la IA | RF-085 | RNF-020 | CP-085, CP-120 |
| HU-22 | Controlar la simulación para explicarla | RF-075 | RNF-020 | CP-075, CP-120, CP-203 |
| HU-23 | Rutas para la horda sin congelar el juego | RF-026, RF-027, RF-028, RF-030 | RNF-001, RNF-002, RNF-003, RNF-004, RNF-006, RNF-015, RNF-027, RNF-030 | CP-026, CP-027, CP-028, CP-030, CP-101, CP-102, CP-103, CP-104, CP-106, CP-115, CP-127, CP-130 |

## 4. Matriz inversa RF → HU, RNF y CP

| RF | Título | Prio. | HU | RNF | CP | Estado |
|---|---|---|---|---|---|---|
| RF-001 | Cargar mapa de Tiled | M | HU-01 | RNF-008, RNF-016 | CP-001, CP-202 | Propuesto |
| RF-002 | Modos de movimiento de Mara | M | HU-01 | RNF-001, RNF-012 | CP-002, CP-202 | Propuesto |
| RF-003 | Ruido por movimiento | M | HU-01 | RNF-020 | CP-003, CP-202 | Propuesto |
| RF-004 | Colisiones | M | HU-01 | RNF-015 | CP-004, CP-202 | Propuesto |
| RF-005 | Recoger objeto | M | HU-02 | RNF-012 | CP-005, CP-202 | Propuesto |
| RF-006 | Puerta metálica | M | HU-02 | RNF-020 | CP-006, CP-202 | Propuesto |
| RF-007 | Instalar batería y activar radio | M | HU-03 | RNF-014 | CP-007, CP-202 | Propuesto |
| RF-008 | Interrupción de la instalación | M | HU-03 | RNF-014 | CP-008, CP-202 | Propuesto |
| RF-009 | Ventana de extracción | M | HU-04 | RNF-013 | CP-009, CP-202 | Propuesto |
| RF-010 | Victoria | M | HU-04 | RNF-020 | CP-010, CP-202 | Propuesto |
| RF-011 | Derrota por salud | M | HU-05 | RNF-020 | CP-011, CP-202 | Propuesto |
| RF-012 | Derrota por refugio | M | HU-05 | RNF-020 | CP-012, CP-202 | Propuesto |
| RF-013 | Señuelo sonoro | M | HU-06 | RNF-014 | CP-013, CP-201, CP-202 | Propuesto |
| RF-014 | Barricada | M | HU-07 | RNF-014 | CP-014, CP-201, CP-202 | Propuesto |
| RF-015 | Usar medicamento | S | HU-02 | RNF-012 | CP-015, CP-202 | Propuesto |
| RF-016 | HUD de objetivos | M | HU-01, HU-02 | RNF-013, RNF-029 | CP-016, CP-202 | Propuesto |
| RF-017 | Tutorial | S | HU-20 | RNF-011 | CP-017, CP-202 | Propuesto |
| RF-020 | Grafo de costos | M | HU-16 | RNF-005, RNF-019 | CP-020 | Propuesto |
| RF-021 | Dijkstra | M | HU-16 | RNF-003, RNF-019 | CP-021 | Propuesto |
| RF-022 | A\* con heurística octil | M | HU-16 | RNF-003, RNF-019 | CP-022 | Propuesto |
| RF-023 | Métricas de búsqueda | M | HU-14, HU-16 | RNF-032 | CP-023 | Propuesto |
| RF-024 | Algoritmo intercambiable sobre el mismo grafo | M | HU-16 | RNF-026 | CP-024 | Propuesto |
| RF-025 | Replanificación ante cambios del mapa | M | HU-07, HU-11 | RNF-002 | CP-025, CP-201 | Propuesto |
| RF-026 | Presupuesto de búsqueda por tick | M | HU-23 | RNF-001, RNF-002, RNF-003, RNF-006 | CP-026 | Propuesto |
| RF-027 | Sin ruta | M | HU-23 | RNF-015 | CP-027 | Propuesto |
| RF-028 | Flow field compartido | S | HU-23 | RNF-004, RNF-030 | CP-028 | Propuesto |
| RF-029 | Steering y boids | M | HU-10 | RNF-002 | CP-029 | Propuesto |
| RF-030 | Consulta espacial con quadtree | M | HU-10, HU-23 | RNF-027, RNF-030 | CP-030 | Propuesto |
| RF-031 | Costos de terreno | M | HU-16 | RNF-016 | CP-031 | Propuesto |
| RF-040 | Ciclo del agente | M | HU-08 | RNF-002, RNF-023 | CP-040 | Propuesto |
| RF-041 | Percepción visual local sin omnisciencia | M | HU-08 | RNF-023 | CP-041, CP-201 | Propuesto |
| RF-042 | Percepción auditiva | M | HU-08, HU-09 | RNF-002 | CP-042 | Propuesto |
| RF-043 | Memoria de evidencias | M | HU-08, HU-09, HU-11 | RNF-005, RNF-006 | CP-043 | Propuesto |
| RF-044 | Máquina de estados del infectado | M | HU-08 | RNF-025 | CP-044 | Propuesto |
| RF-045 | Exploración sin información global | M | HU-08 | RNF-005 | CP-045, CP-204 | Propuesto |
| RF-046 | Investigar ruido | M | HU-09 | RNF-002, RNF-018 | CP-046, CP-204 | Propuesto |
| RF-047 | Propagación recursiva acotada de alertas | M | HU-09 | RNF-018, RNF-027 | CP-047, CP-204 | Propuesto |
| RF-048 | Roles de infectado | M | HU-08 | RNF-026 | CP-048 | Propuesto |
| RF-049 | Coordinación con Contract Net | M | HU-10, HU-12 | RNF-002 | CP-049 | Propuesto |
| RF-050 | Reevaluación ante señuelos | M | HU-11 | RNF-002, RNF-018 | CP-050, CP-201, CP-204 | Propuesto |
| RF-051 | Distribución de la aproximación | M | HU-10 | RNF-030 | CP-051, CP-204 | Propuesto |
| RF-052 | Respuesta a barricadas | M | HU-07, HU-11 | RNF-002 | CP-052, CP-201 | Propuesto |
| RF-053 | Interceptación | M | HU-12 | RNF-002 | CP-053 | Propuesto |
| RF-054 | Selección de táctica | M | HU-12 | RNF-025 | CP-054, CP-204 | Propuesto |
| RF-055 | Ataque | M | HU-05 | RNF-020 | CP-055 | Propuesto |
| RF-056 | Modo aislado o coordinado | M | HU-18 | RNF-020 | CP-056, CP-203 | Propuesto |
| RF-060 | Selección de meta por utilidad | S | HU-13 | RNF-002 | CP-060 | Propuesto |
| RF-061 | Planificación GOAP | S | HU-13 | RNF-027 | CP-061 | Propuesto |
| RF-062 | Ejecución con árbol de comportamiento | S | HU-13 | RNF-027 | CP-062 | Propuesto |
| RF-063 | Integridad del refugio | M | HU-05, HU-13 | RNF-020 | CP-063 | Propuesto |
| RF-064 | Replanificación del superviviente | S | HU-13 | RNF-002 | CP-064 | Propuesto |
| RF-070 | Panel del agente seleccionado | M | HU-14 | RNF-013 | CP-070, CP-203 | Propuesto |
| RF-071 | Superposiciones de depuración | M | HU-14 | RNF-001 | CP-071, CP-203 | Propuesto |
| RF-072 | Registro de cambios de plan | M | HU-11, HU-15 | RNF-007 | CP-072, CP-203 | Propuesto |
| RF-073 | Registro de mensajes entre agentes | M | HU-09, HU-15 | RNF-007 | CP-073, CP-203 | Propuesto |
| RF-074 | Comparación de algoritmos en vivo | M | HU-16 | RNF-019 | CP-074, CP-203 | Propuesto |
| RF-075 | Control de la simulación | S | HU-22 | RNF-020 | CP-075, CP-203 | Propuesto |
| RF-080 | Benchmark sin ventana | M | HU-17 | RNF-005, RNF-032 | CP-080 | Propuesto |
| RF-081 | Metadatos del experimento | M | HU-17 | RNF-032 | CP-081 | Propuesto |
| RF-082 | Verificación de optimalidad | M | HU-17 | RNF-019 | CP-082 | Propuesto |
| RF-083 | Escenarios dinámicos | M | HU-17 | RNF-020 | CP-083 | Propuesto |
| RF-084 | Experimento aislados frente a coordinados | M | HU-18 | RNF-020 | CP-084 | Propuesto |
| RF-085 | Optimización de parámetros | S | HU-21 | RNF-020 | CP-085 | Propuesto |
| RF-086 | Aleatoriedad con semilla | M | HU-17 | RNF-020 | CP-086 | Propuesto |
| RF-090 | Guardar partida | S | HU-19 | RNF-007, RNF-017 | CP-090 | Propuesto |
| RF-091 | Cargar partida | S | HU-19 | RNF-020 | CP-091 | Propuesto |
| RF-092 | Configuración local | S | HU-19 | RNF-012 | CP-092 | Propuesto |
| RF-093 | Archivos corruptos o inválidos | M | HU-19 | RNF-016, RNF-021 | CP-093 | Propuesto |

## 5. Matriz RNF → RF y CP

| RNF | Título | Característica ISO 25010 | RF que lo condicionan | CP |
|---|---|---|---|---|
| RNF-001 | Tasa de cuadros | Eficiencia de desempeño | RF-002, RF-026, RF-071 | CP-101 |
| RNF-002 | Tiempo de tick de simulación | Eficiencia de desempeño | RF-025, RF-026, RF-029, RF-040, RF-042, RF-046, RF-049, RF-050, RF-052, RF-053, RF-060, RF-064 | CP-102 |
| RNF-003 | Tiempo de búsqueda | Eficiencia de desempeño | RF-021, RF-022, RF-026 | CP-103 |
| RNF-004 | Uso de CPU | Eficiencia de desempeño | RF-028 | CP-104 |
| RNF-005 | Memoria | Eficiencia de desempeño | RF-020, RF-043, RF-045, RF-080 | CP-105 |
| RNF-006 | Cero asignaciones dinámicas en el bucle | Eficiencia de desempeño | RF-026, RF-043 | CP-106 |
| RNF-007 | Almacenamiento | Eficiencia de desempeño | RF-072, RF-073, RF-090 | CP-107 |
| RNF-008 | Tiempo de arranque | Eficiencia de desempeño | RF-001 | CP-108 |
| RNF-018 | Latencia de reacción de los agentes | Eficiencia de desempeño | RF-046, RF-047, RF-050 | CP-118 |
| RNF-027 | Capacidad: profundidad de recursión acotada | Eficiencia de desempeño | RF-030, RF-047, RF-061, RF-062 | CP-127 |
| RNF-019 | Corrección: optimalidad de A\* | Adecuación funcional | RF-020, RF-021, RF-022, RF-074, RF-082 | CP-119 |
| RNF-020 | Corrección: determinismo | Adecuación funcional | RF-003, RF-006, RF-010, RF-011, RF-012, RF-055, RF-056, RF-063, RF-075, RF-083, RF-084, RF-085, RF-086, RF-091 | CP-120 |
| RNF-032 | Completitud: registro de experimentos | Adecuación funcional | RF-023, RF-080, RF-081 | CP-132 |
| RNF-009 | Interoperabilidad de plataforma | Compatibilidad | Transversal | CP-109 |
| RNF-010 | Coexistencia | Compatibilidad | Transversal | CP-110 |
| RNF-011 | Capacidad de aprendizaje | Capacidad de interacción (Usabilidad) | RF-017 | CP-111 |
| RNF-012 | Operabilidad | Capacidad de interacción (Usabilidad) | RF-002, RF-005, RF-015, RF-092 | CP-112 |
| RNF-013 | Inclusividad y legibilidad | Capacidad de interacción (Usabilidad) | RF-009, RF-016, RF-070 | CP-113 |
| RNF-014 | Protección contra errores del usuario | Capacidad de interacción (Usabilidad) | RF-007, RF-008, RF-013, RF-014 | CP-114 |
| RNF-015 | Madurez | Fiabilidad | RF-004, RF-027 | CP-115 |
| RNF-016 | Tolerancia a fallos | Fiabilidad | RF-001, RF-031, RF-093 | CP-116 |
| RNF-017 | Recuperabilidad | Fiabilidad | RF-090 | CP-117 |
| RNF-021 | Integridad de los datos locales | Seguridad (Security) | RF-093 | CP-121 |
| RNF-022 | Superficie de ataque mínima | Seguridad (Security) | Transversal | CP-122 |
| RNF-023 | Modularidad del dominio | Mantenibilidad | RF-040, RF-041 | CP-123 |
| RNF-024 | Capacidad de prueba | Mantenibilidad | Transversal | CP-124 |
| RNF-025 | Capacidad de análisis | Mantenibilidad | RF-044, RF-054 | CP-125 |
| RNF-026 | Capacidad de modificación | Mantenibilidad | RF-024, RF-048 | CP-126 |
| RNF-028 | Instalabilidad | Flexibilidad (Portabilidad) | Transversal | CP-128 |
| RNF-029 | Adaptabilidad de pantalla | Flexibilidad (Portabilidad) | RF-016 | CP-129 |
| RNF-030 | Escalabilidad | Flexibilidad (Portabilidad) | RF-028, RF-030, RF-051 | CP-130 |
| RNF-031 | Fotosensibilidad | Protección (Safety) | Transversal | CP-131 |

## 6. Uso en Jira, Azure DevOps o GitHub Projects

El archivo [trazabilidad.csv](trazabilidad.csv) tiene una fila por elemento con las columnas
`Tipo, ID, Resumen, Prioridad, Padre, Enlaces, Estado`:

* **Jira**: importar con *System → External System Import → CSV*. Mapear `Tipo` a *Issue Type*
  (Epic, Story, Requirement, Test), `ID` a una etiqueta o campo personalizado, `Padre` a *Parent* y `Enlaces`
  a *Linked issues (relates to)*. Con Xray o Zephyr, los `CP` se importan como *Test*.
* **Azure DevOps**: *Boards → Work items → Import from CSV*. Tipos: Epic, User Story, Requirement
  (plantilla CMMI) y Test Case; `Enlaces` se crea como *Related* y `Padre` como *Parent*.
* **GitHub Projects**: crear *issues* con etiquetas `HU`, `RF`, `RNF`, `CP` y el ID en el título; los enlaces
  se escriben como lista de tareas en el cuerpo.

Regla de mantenimiento: ningún PR que añada o cambie un requisito se fusiona sin actualizar esta matriz.
