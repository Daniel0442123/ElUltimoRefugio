# 04 · Requisitos no funcionales (RNF)

> Clasificados con el modelo de calidad de producto **ISO/IEC 25010:2023** (se indica entre paréntesis el
> nombre equivalente de la edición 2011 cuando cambió). Todo RNF es **medible**: tiene métrica, valor
> objetivo, umbral mínimo aceptable, condiciones y método de medición, y se redacta con EARS.
> Atributos de requisito según ISO/IEC/IEEE 29148:2018 §5.2.8.

| Campo | Valor |
|---|---|
| Versión | 0.2.0 (estado real del software 0.16.0) |
| Fecha | 2026-11-25 |
| Estado | Línea base revisada contra la implementación |

## 1. Condiciones de medición comunes

| ID | Condición |
|---|---|
| CM-01 | **Equipo de referencia mínimo** (SUP-01): CPU x86-64 de 2 núcleos a 2,0 GHz (p. ej. Intel Core i3-5005U o equivalente), 4 GB RAM, GPU integrada con OpenGL 3.3, Windows 10 x64 o Ubuntu 22.04 x64. |
| CM-02 | **Escenario de carga nominal**: mapa de 128 × 128 celdas, 100 infectados activos (70 % perseguidores), 1 superviviente, 30 % de los agentes con búsqueda pendiente. |
| CM-03 | **Compilación de medición**: `Release` (`-O2`), sin sanitizadores, Tracy activado solo en modo perfil. |
| CM-04 | **Duración**: 5 minutos de simulación tras 30 s de calentamiento, semilla 42, guion de jugadora fijo `bench/guion_nominal.json`. |
| CM-05 | **Percentiles** calculados sobre todas las muestras por frame o por tick del periodo medido. |

### 1.1 Estado de las mediciones en 0.16.0

De los 32 RNF, 6 están verificados con pruebas automatizadas (RNF-006, RNF-017, RNF-019, RNF-021, RNF-023 y
RNF-027), 25 están implementados con la medición incompleta y 1 está pendiente (RNF-029, escalado de pantalla).
Las causas principales son tres: no se integró Tracy (CM-03), no se escribieron los scripts de medición previstos
en `tools/` (`fps_report.py`, `recursos.py`, `latencia_ia.py`, `validar_csv.py`), y faltan las pruebas con
personas. El escenario nominal CM-02 (100 infectados en 128²) existe como prueba de CP-026, pero no el guion
`bench/guion_nominal.json` de CM-04. Cada RNF tiene una fila «Estado (0.16.0)» con lo medido y lo que falta, y la
[matriz](06_matriz_trazabilidad.md) §5.1 los resume.

Mediciones disponibles (contenedor de desarrollo, Xeon 2,3 GHz, 4 núcleos, GCC 13.3, Release, 2026-11-25):

| Medida | Valor | RNF |
|---|---|---|
| A\* en 128² con 25 % de obstáculos (100 consultas) | mediana 108 µs, p95 548 µs, máx. 1,15 ms | RNF-003 (objetivo p95 ≤ 1 ms) |
| Dijkstra en el mismo escenario | mediana 497 µs, p95 1,65 ms | — (referencia) |
| A\* en 256² con 25 % de obstáculos | mediana 512 µs, p95 1,39 ms | — |
| RSS pico del benchmark 128² y 256² | 13,7 MB | RNF-005 (objetivo ≤ 64 MB) |
| Búsquedas de 100 solicitudes en 128², por tick de IA | ≤ 4000 nodos y ≤ 0,7 ms; todas en 5 ticks | RNF-002, RNF-030 (CP-026) |
| Partida guardada de 10 min | ≈ 16 KB | RNF-007 (objetivo ≤ 64 KB) |
| Asignaciones por paso tras la carga | 0 | RNF-006 (CP-106) |
| Suite de pruebas | 172 casos en 1,4 s | RNF-024 (objetivo ≤ 60 s) |

Comando del benchmark: `refugio_bench --bench --tamanos 128,256 --densidad 0.25 --pares 20 --reps 5 --semilla 42`.

FPS, tiempo de tick, CPU, memoria del juego y arranque (RNF-001, RNF-002, RNF-004, RNF-005, RNF-008) se miden
con ventana con `python tools/rendimiento.py medir`, que ejecuta `refugio --medir-rendimiento`: 5 arranques y dos
partidas jugadas solas con un guion fijo (10 y 100 infectados en el mapa del juego, 30 s de calentamiento y 5 min
medidos, VSync). La CPU se lee con `GetProcessTimes` en Windows y `getrusage` en Linux, y la memoria con
`GetProcessMemoryInfo` o `/proc/self/statm`. No usa Tracy ni `tools/fps_report.py` ni `tools/recursos.py`: el
juego mide cada frame y cada segundo y la herramienta escribe el bloque siguiente.

<!-- generado:rendimiento inicio -->
Mediciones con ventana (`tools/rendimiento.py`, 2026-11-26, 12th Gen Intel(R) Core(TM) i7-12650H, 16 hilos, 15,7 GB, Intel(R) UHD Graphics, Windows, msvc 194435230 Release, ventana 1920x991):

| RNF | Resultado | Estado |
|---|---|---|
| RNF-001 | FPS p5 58,0 (10 infectados) y 58,8 (100 infectados); frames de más de 25 ms: 0,02 % (10 infectados) y 0,01 % (100 infectados). | Cumple |
| RNF-002 | Paso de simulación p99 0,14 ms (10 infectados) y 2,25 ms (100 infectados). | Cumple |
| RNF-004 | CPU media 27,5 % (10 infectados) y 28,1 % (100 infectados) de un núcleo lógico, con VSync. | Cumple |
| RNF-005 | Memoria residente pico del juego 174,9 MB (10 infectados) y 143,9 MB (100 infectados); benchmark 256²: 13,7 MB (≤ 64 MB). | Cumple el umbral |
| RNF-008 | Primer frame de la partida en 0,49 s de media en 5 arranques (máx. 1,16 s); escena en 3,0 ms. No hay menú principal: cuenta el primer frame. | Cumple |
<!-- generado:rendimiento fin -->

## 2. Resumen por característica

| Característica ISO 25010:2023 | RNF |
|---|---|
| Adecuación funcional | RNF-019, RNF-020, RNF-032 |
| Eficiencia de desempeño | RNF-001 a RNF-008, RNF-018, RNF-027 |
| Compatibilidad | RNF-009, RNF-010 |
| Capacidad de interacción (Usabilidad) | RNF-011 a RNF-014 |
| Fiabilidad | RNF-015 a RNF-017 |
| Seguridad (Security) | RNF-021, RNF-022 |
| Mantenibilidad | RNF-023 a RNF-026 |
| Flexibilidad (Portabilidad) | RNF-028 a RNF-030 |
| Protección (Safety) | RNF-031 |

---

## 3. Eficiencia de desempeño

### RNF-001 · Tasa de cuadros
| Atributo | Valor |
|---|---|
| Subcaracterística | Comportamiento temporal |
| Enunciado EARS | **Mientras** se ejecuta el escenario CM-02 en el equipo CM-01, el juego deberá mantener al menos 60 FPS en el percentil 5 de los frames. |
| Métrica | FPS = 1 / duración del frame; percentil 5 (p5) |
| Objetivo / umbral mínimo | p5 ≥ 60 FPS / p5 ≥ 30 FPS |
| Método | Tracy + exportación de `FrameMark`; script `tools/fps_report.py` |
| Prioridad / fuente / HU | M · NEG-07, StR-15 · HU-01, HU-23 |
| Verificación | P · CP-101 |
| Estado (medido) | **Verificado**. FPS p5 58,0 (10 infectados) y 58,8 (100 infectados); frames de más de 25 ms: 0,02 % (10 infectados) y 0,01 % (100 infectados). Cumple (docs/informe/datos/rendimiento/resumen.json). |

### RNF-002 · Tiempo de tick de simulación
| Atributo | Valor |
|---|---|
| Subcaracterística | Comportamiento temporal |
| Enunciado EARS | **Mientras** se ejecuta CM-02, cada tick de simulación (física + IA + eventos, sin render) deberá completarse en 4 ms o menos en el percentil 99. |
| Métrica | Duración de la zona Tracy `Sim::tick` (ms), p99 |
| Objetivo / umbral | p99 ≤ 4 ms / p99 ≤ 8 ms |
| Método | Tracy, zonas instrumentadas |
| Prioridad / fuente / HU | M · NEG-07 · HU-08, HU-23 |
| Verificación | P · CP-102 |
| Estado (medido) | **Verificado**. Paso de simulación p99 0,14 ms (10 infectados) y 2,25 ms (100 infectados). Cumple (docs/informe/datos/rendimiento/resumen.json). |

### RNF-003 · Tiempo de búsqueda
| Atributo | Valor |
|---|---|
| Subcaracterística | Comportamiento temporal |
| Enunciado EARS | **Cuando** se ejecute una búsqueda A\* completa en un mapa de 128 × 128 con 25 % de obstáculos, el sistema deberá terminarla en 1 ms o menos en el p95; el tiempo total de IA por tick de IA deberá ser ≤ 2 ms en el p95 bajo CM-02. |
| Métrica | `tiempo_us` de RF-023; zona Tracy `IA::tick` |
| Objetivo / umbral | A\* p95 ≤ 1 ms, IA p95 ≤ 2 ms / A\* p95 ≤ 3 ms, IA p95 ≤ 5 ms |
| Método | Benchmark RF-080 y Tracy |
| Prioridad / fuente / HU | M · GUIA §5, §7 · HU-16, HU-17 |
| Verificación | P · CP-103 |
| Estado (0.16.0) | **Implementado en 0.1.0**. A* en 128² con 25 % de obstáculos: mediana 108 µs, p95 548 µs (cumple). IA por tick sin medir. |

### RNF-004 · Uso de CPU
| Atributo | Valor |
|---|---|
| Subcaracterística | Utilización de recursos |
| Enunciado EARS | **Mientras** se ejecuta CM-02 con VSync a 60 Hz, el proceso deberá usar en promedio 35 % o menos de un núcleo lógico. |
| Métrica | Tiempo de CPU del proceso / tiempo real (Windows: `GetProcessTimes`; Linux: `/proc/self/stat`) |
| Objetivo / umbral | ≤ 35 % / ≤ 60 % |
| Método | Muestreo cada segundo por `tools/recursos.py`; promedio del periodo CM-04 |
| Prioridad / fuente | M · NEG-07, StR-15 |
| Verificación | P · CP-104 |
| Estado (medido) | **Verificado**. CPU media 27,5 % (10 infectados) y 28,1 % (100 infectados) de un núcleo lógico, con VSync. Cumple (docs/informe/datos/rendimiento/resumen.json). |

### RNF-005 · Memoria
| Atributo | Valor |
|---|---|
| Subcaracterística | Utilización de recursos |
| Enunciado EARS | El proceso deberá mantener su memoria residente (RSS / *Working Set*) en 128 MB o menos durante CM-02, y en 64 MB o menos en modo benchmark con mapas de 256 × 256. |
| Métrica | RSS pico (MB) |
| Objetivo / umbral | ≤ 128 MB / ≤ 256 MB |
| Método | `tools/recursos.py`; Valgrind Massif en Linux para el desglose |
| Prioridad / fuente | M · NEG-07 |
| Verificación | P · CP-105 |
| Estado (medido) | **Implementado**. Memoria residente pico del juego 174,9 MB (10 infectados) y 143,9 MB (100 infectados); benchmark 256²: 13,7 MB (≤ 64 MB). Cumple el umbral (docs/informe/datos/rendimiento/resumen.json). |

### RNF-006 · Cero asignaciones dinámicas en el bucle
| Atributo | Valor |
|---|---|
| Subcaracterística | Utilización de recursos |
| Enunciado EARS | **Mientras** la partida está en curso, tras la carga de la escena, el bucle de juego no deberá realizar asignaciones de memoria del montículo en el percentil 99 de los frames. |
| Métrica | Número de llamadas a `operator new`/`malloc` por frame (contador instrumentado + `TracyAlloc`) |
| Objetivo / umbral | 0 en p99 / ≤ 4 en p99 |
| Método | Compilación `Profile` con contador global de asignaciones |
| Prioridad / fuente | S · NEG-07; patrón Object Pool (ADR-0004) |
| Verificación | P · CP-106 |
| Estado (0.16.0) | **Verificado en 0.2.0**. 0 asignaciones por paso con contador global (CP-106). |

### RNF-007 · Almacenamiento
| Atributo | Valor |
|---|---|
| Subcaracterística | Utilización de recursos |
| Enunciado EARS | La distribución del juego (ejecutable + recursos) deberá ocupar 50 MB o menos, cada partida guardada 64 KB o menos, y los registros de una sesión de 30 min 20 MB o menos (con rotación). |
| Métrica | Tamaño en disco (bytes) |
| Objetivo / umbral | 50 MB / 100 MB; 64 KB / 256 KB; 20 MB / 50 MB |
| Método | Paso de CI `size-check` sobre el `.zip`; prueba de guardado |
| Prioridad / fuente | M · NEG-07 |
| Verificación | P · CP-107 |
| Estado (0.16.0) | **Implementado en 0.12.0**. Partida de 10 min ≈ 16 KB (cumple ≤ 64 KB). Sin medir el .zip ni los registros; no hay rotación. |

### RNF-008 · Tiempo de arranque
| Atributo | Valor |
|---|---|
| Subcaracterística | Comportamiento temporal |
| Enunciado EARS | **Cuando** se inicie el ejecutable en CM-01 con caché fría, el sistema deberá mostrar el menú principal en 3 s o menos y cargar una escena en 2 s o menos. |
| Métrica | Tiempo desde `main()` hasta el primer frame del menú; tiempo de carga de escena |
| Objetivo / umbral | 3 s y 2 s / 6 s y 4 s |
| Método | Marcas de tiempo en el registro; promedio de 5 arranques |
| Prioridad / fuente | S · StR-15 |
| Verificación | P · CP-108 |
| Estado (medido) | **Verificado**. Primer frame de la partida en 0,49 s de media en 5 arranques (máx. 1,16 s); escena en 3,0 ms. No hay menú principal: cuenta el primer frame. Cumple (docs/informe/datos/rendimiento/resumen.json). |

### RNF-018 · Latencia de reacción de los agentes
| Atributo | Valor |
|---|---|
| Subcaracterística | Comportamiento temporal |
| Enunciado EARS | **Cuando** un agente perciba un estímulo relevante (ruido, avistamiento, señuelo o alerta), el sistema deberá reflejar la decisión resultante (cambio de estado u objetivo) en 200 ms o menos de tiempo de simulación en el p95, y en 500 ms o menos en el peor caso. |
| Métrica | Ticks entre el evento de percepción y el evento `CambioDePlan` asociado, convertidos a ms |
| Objetivo / umbral | p95 ≤ 200 ms y máx ≤ 500 ms / p95 ≤ 300 ms y máx ≤ 800 ms |
| Método | Análisis de `logs/ia_<semilla>.log` del escenario CM-02 con `tools/latencia_ia.py` |
| Prioridad / fuente / HU | M · STB Esc. 2 y 4; GUIA §4 «reconsiderar sus planes ante cambios» · HU-09, HU-11 |
| Verificación | P · CP-118 |
| Estado (0.16.0) | **Implementado en 0.5.0**. CP-050: cambio ante señuelo en ≤ 5 ticks de IA (500 ms). Sin tools/latencia_ia.py para el p95. |

### RNF-027 · Capacidad: profundidad de recursión acotada
| Atributo | Valor |
|---|---|
| Subcaracterística | Capacidad |
| Enunciado EARS | Toda función recursiva del sistema deberá tener una profundidad máxima declarada (alertas ≤ `ALERTA_PROF_MAX` = 3, GOAP ≤ 6, quadtree ≤ 8, árbol de comportamiento ≤ 10) y el uso de pila de la recursión más profunda deberá ser 64 KB o menos. |
| Métrica | Profundidad máxima observada (contador instrumentado) y bytes de pila por marco × profundidad |
| Objetivo / umbral | Profundidad ≤ límite declarado en el 100 % de las llamadas; pila ≤ 64 KB / ≤ 256 KB |
| Método | Aserciones en compilación `Debug`; análisis de marco con `-fstack-usage` (GCC) |
| Prioridad / fuente | M · GUIA §5 «análisis de pila»; STB §03 |
| Verificación | A + P · CP-127 |
| Estado (0.16.0) | **Verificado en 0.1.0**. CP-127 y estrés de recursión en CI (hasta 2000 agentes con ASan). |

---

## 4. Adecuación funcional

### RNF-019 · Corrección: optimalidad de A\*
| Atributo | Valor |
|---|---|
| Subcaracterística | Corrección funcional |
| Enunciado EARS | **Cuando** se ejecute el benchmark, A\* deberá devolver un costo igual al de Dijkstra (diferencia ≤ 1e-6) en el 100 % de las consultas con ruta, y ambos deberán coincidir en las consultas sin ruta. |
| Métrica | Porcentaje de consultas con |costo_A\* − costo_Dijkstra| ≤ 1e-6 |
| Objetivo / umbral | 100 % / 100 % |
| Método | RF-082 sobre ≥ 800 consultas |
| Prioridad / fuente / HU | M · GUIA §5; STB §03 · HU-16, HU-17 |
| Verificación | P · CP-119 |
| Estado (0.16.0) | **Verificado en 0.10.0**. 100 % de coincidencia; el benchmark termina con código 2 si no. |

### RNF-020 · Corrección: determinismo
| Atributo | Valor |
|---|---|
| Subcaracterística | Corrección funcional |
| Enunciado EARS | **Cuando** se ejecute la simulación dos veces con la misma semilla, la misma configuración y el mismo guion de entradas en el mismo binario, el sistema deberá producir el mismo hash de estado (FNV-1a de 64 bits) en cada tick. |
| Métrica | Porcentaje de ticks con hash idéntico en 10 pares de ejecuciones de 10 000 ticks |
| Objetivo / umbral | 100 % / 100 % |
| Método | Prueba `determinismo` en CI |
| Prioridad / fuente | M · GUIA §7; STB §03 «semillas registradas»; RES-09 |
| Verificación | P · CP-120 |
| Estado (0.16.0) | **Implementado en 0.2.0**. Determinismo comprobado en CP-075, CP-084, CP-086 y CP-091, no con 10 × 10 000 ticks. |

### RNF-032 · Completitud: registro de experimentos
| Atributo | Valor |
|---|---|
| Subcaracterística | Completitud funcional |
| Enunciado EARS | Cada archivo de resultados deberá contener el 100 % de los campos de metadatos de RF-081 y de las columnas de RF-080, sin valores vacíos salvo `memoria_pico_bytes` cuando la plataforma no permita medirla, en cuyo caso deberá valer `NA`. |
| Métrica | Campos presentes / campos exigidos |
| Objetivo / umbral | 100 % / 100 % |
| Método | Validador `tools/validar_csv.py` en CI |
| Prioridad / fuente | M · GUIA §7 |
| Verificación | P · CP-132 |
| Estado (0.16.0) | **Implementado en 0.10.0**. La prueba de CP-080 exige filas completas; no hay tools/validar_csv.py. |

---

## 5. Compatibilidad

### RNF-009 · Interoperabilidad de plataforma
| Atributo | Valor |
|---|---|
| Subcaracterística | Interoperabilidad |
| Enunciado EARS | El sistema deberá compilar y ejecutarse sin cambios de código en Windows 10/11 x64 (MSVC 2022 y MinGW-w64 GCC 13) y Ubuntu 22.04 x64 (GCC 12+ y Clang 15+), con OpenGL 3.3. |
| Métrica | Matriz de CI: plataformas × compiladores con compilación y pruebas en verde |
| Objetivo / umbral | 4/4 / 2/4 (al menos un compilador por SO) |
| Método | GitHub Actions `build.yml` |
| Prioridad / fuente | M · StR-15 |
| Verificación | P · CP-109 |
| Estado (0.16.0) | **Implementado en 0.1.0**. CI con Linux GCC, Linux Clang y Windows MSVC (3 de 4; falta MinGW). Cumple el umbral 2/4. |

### RNF-010 · Coexistencia
| Atributo | Valor |
|---|---|
| Subcaracterística | Coexistencia |
| Enunciado EARS | El sistema deberá ejecutarse sin instalar servicios, controladores ni runtimes adicionales, y sin escribir fuera de su carpeta salvo en la carpeta de datos de usuario del SO cuando la suya sea de solo lectura. |
| Métrica | Archivos y claves creados fuera de la carpeta (Process Monitor / `strace -e trace=file`) |
| Objetivo / umbral | 0 / 0 |
| Método | Inspección con herramientas de traza en una ejecución de 5 min |
| Prioridad / fuente | M · NEG-07; RES-05 |
| Verificación | I + P · CP-110 |
| Estado (0.16.0) | **Implementado en 0.12.0**. Solo escribe en config/, saves/, logs/ y results/. Sin traza de archivos. |

---

## 6. Capacidad de interacción (Usabilidad)

### RNF-011 · Capacidad de aprendizaje
| Atributo | Valor |
|---|---|
| Subcaracterística | Capacidad de aprendizaje |
| Enunciado EARS | **Cuando** una persona que nunca jugó el juego complete el tutorial, deberá hacerlo en 3 min o menos y el 80 % de los participantes deberá superar la Escena 1 sin ayuda externa. |
| Métrica | Tiempo de tutorial (registro); tasa de éxito |
| Objetivo / umbral | 3 min y 80 % / 5 min y 60 % |
| Método | Prueba de usabilidad con ≥ 5 personas |
| Prioridad / fuente / HU | S · StR-20 · HU-20 |
| Verificación | D · CP-111 |
| Estado (0.16.0) | **Implementado en 0.12.0**. El tutorial registra su duración; falta la prueba con 5 personas. |

### RNF-012 · Operabilidad
| Atributo | Valor |
|---|---|
| Subcaracterística | Operabilidad |
| Enunciado EARS | El sistema deberá permitir reasignar el 100 % de las acciones de juego y mostrar la respuesta visual a cada entrada en el mismo frame o el siguiente (latencia de entrada ≤ 33 ms). |
| Métrica | Acciones reasignables / total; latencia entrada → frame |
| Objetivo / umbral | 100 % y ≤ 33 ms / 90 % y ≤ 50 ms |
| Método | Inspección del menú; marcas de tiempo de entrada y presentación |
| Prioridad / fuente / HU | S · StR-16 · HU-01, HU-19 |
| Verificación | I + P · CP-112 |
| Estado (0.16.0) | **Implementado en 0.12.0**. 100 % de las acciones reasignables (CP-112). Latencia de entrada sin medir. |

### RNF-013 · Inclusividad y legibilidad
| Atributo | Valor |
|---|---|
| Subcaracterística | Inclusividad / Accesibilidad |
| Enunciado EARS | El sistema deberá mostrar el texto del HUD y del panel con altura ≥ 12 px lógicos y escala ajustable de 100 % a 200 %, usar contraste ≥ 4,5:1, y no codificar estados de agente solo por color (cada estado tiene además icono o letra). |
| Métrica | Tamaño de fuente; ratio de contraste WCAG; estados con codificación redundante |
| Objetivo / umbral | 12 px, 4,5:1, 100 % / 10 px, 3:1, 100 % |
| Método | Inspección con capturas y verificador de contraste; simulación de daltonismo |
| Prioridad / fuente / HU | S · StR-10 · HU-14 |
| Verificación | I · CP-113 |
| Estado (0.16.0) | **Implementado en 0.16.0**. Estados con color y «?»/«!»; sin revisión de contraste ni escala. |

### RNF-014 · Protección contra errores del usuario
| Atributo | Valor |
|---|---|
| Subcaracterística | Protección frente a errores del usuario |
| Enunciado EARS | **Cuando** la jugadora intente salir o cargar con progreso no guardado, el sistema deberá pedir confirmación; **si** intenta una acción inválida (colocar barricada en celda ocupada, lanzar sin señuelos), **entonces** deberá indicar el motivo en el HUD. |
| Métrica | Acciones destructivas con confirmación / total; acciones inválidas con mensaje / total |
| Objetivo / umbral | 100 % / 100 % |
| Método | Pruebas de interfaz |
| Prioridad / fuente | S · StR-16 |
| Verificación | P · CP-114 |
| Estado (0.16.0) | **Implementado en 0.12.0**. Confirmación al cargar o salir sin guardar (CP-114). |

---

## 7. Fiabilidad

### RNF-015 · Madurez
| Atributo | Valor |
|---|---|
| Subcaracterística | Ausencia de fallos (Madurez) |
| Enunciado EARS | El sistema deberá ejecutar 30 min continuos del escenario CM-02 a velocidad 4× sin cierres inesperados, y la suite de pruebas deberá ejecutarse sin errores de AddressSanitizer ni UndefinedBehaviorSanitizer. |
| Métrica | Cierres por sesión; errores de sanitizadores |
| Objetivo / umbral | 0 y 0 / 0 y 0 |
| Método | Prueba de resistencia nocturna en CI; trabajo `sanitizers` (Linux, Clang) |
| Prioridad / fuente | M · NEG-05 |
| Verificación | P · CP-115 |
| Estado (0.16.0) | **Implementado en 0.1.0**. ASan + UBSan en CI sin errores. Falta la prueba de 30 min a 4×. |

### RNF-016 · Tolerancia a fallos
| Atributo | Valor |
|---|---|
| Subcaracterística | Tolerancia a fallos |
| Enunciado EARS | **Si** se proporciona un mapa, configuración o partida inválidos, **entonces** el sistema deberá rechazarlos con un mensaje y continuar en el 100 % de los casos del corpus de entradas inválidas. |
| Métrica | Casos manejados sin cierre / casos del corpus (≥ 30 archivos: truncados, JSON mal formado, rangos fuera de límites, CRC erróneo, tamaño > 10 MB) |
| Objetivo / umbral | 100 % / 100 % |
| Método | Pruebas con corpus `tests/data/invalid/`; *fuzzing* con libFuzzer 10 min en CI nocturno |
| Prioridad / fuente | M · RF-093 |
| Verificación | P · CP-116 |
| Estado (0.16.0) | **Implementado en 0.12.0**. Corpus de ≥ 30 entradas inválidas sin cierres (CP-116). Sin libFuzzer. |

### RNF-017 · Recuperabilidad
| Atributo | Valor |
|---|---|
| Subcaracterística | Recuperabilidad |
| Enunciado EARS | **Si** el proceso termina abruptamente durante un guardado, **entonces** la última partida guardada completa deberá seguir siendo cargable. |
| Métrica | Partidas cargables tras 100 interrupciones simuladas en puntos aleatorios del guardado |
| Objetivo / umbral | 100 % / 100 % |
| Método | Prueba que mata el proceso hijo durante `guardar()` (escritura temporal + renombrado atómico) |
| Prioridad / fuente / HU | S · HU-19 |
| Verificación | P · CP-117 |
| Estado (0.16.0) | **Verificado en 0.12.0**. 100 interrupciones simuladas, todas cargables (CP-117). |

---

## 8. Seguridad (Security)

### RNF-021 · Integridad de los datos locales
| Atributo | Valor |
|---|---|
| Subcaracterística | Integridad |
| Enunciado EARS | El sistema deberá validar la cabecera, la versión y el CRC32 de cada partida, el esquema y los rangos de cada JSON, y rechazar archivos de más de 10 MB antes de analizarlos. |
| Métrica | Archivos alterados detectados / archivos alterados (1 000 mutaciones de 1 byte) |
| Objetivo / umbral | ≥ 99,9 % / ≥ 99 % |
| Método | Prueba de mutación de bytes |
| Prioridad / fuente | M · RF-093 |
| Verificación | P · CP-121 |
| Estado (0.16.0) | **Verificado en 0.12.0**. Todas las mutaciones de 1 byte detectadas (CP-121). |

### RNF-022 · Superficie de ataque mínima
| Atributo | Valor |
|---|---|
| Subcaracterística | Confidencialidad / Resistencia |
| Enunciado EARS | El sistema no deberá abrir sockets de red, cargar bibliotecas dinámicas externas en tiempo de ejecución, ni ejecutar código o scripts leídos de archivos de datos; no deberá recopilar datos personales. |
| Métrica | Sockets abiertos; bibliotecas cargadas fuera de la lista blanca; dependencias con CVE altas |
| Objetivo / umbral | 0; 0; 0 / igual |
| Método | `strace -f -e trace=network`, Process Monitor; revisión de dependencias en CI |
| Prioridad / fuente | M · RES-05 |
| Verificación | I + P · CP-122 |
| Estado (0.16.0) | **Implementado en 0.1.0**. Sin código de red; sin traza strace ni revisión de CVE. |

---

## 9. Mantenibilidad

### RNF-023 · Modularidad del dominio
| Atributo | Valor |
|---|---|
| Subcaracterística | Modularidad |
| Enunciado EARS | El código de `src/dominio/` no deberá incluir cabeceras de raylib, raygui, del sistema operativo ni de E/S de archivos, y deberá compilar como biblioteca estática independiente (`refugio_dominio`). |
| Métrica | Número de `#include` prohibidos en `src/dominio/`; dependencias de enlace de `refugio_dominio` |
| Objetivo / umbral | 0 / 0 |
| Método | Script de CI `tools/check_deps.sh` (grep + `cmake --graphviz`) |
| Prioridad / fuente | M · RES-06; ADR-0003 |
| Verificación | I · CP-123 |
| Estado (0.16.0) | **Verificado en 0.1.0**. tools/check_deps.sh en CI. |

### RNF-024 · Capacidad de prueba
| Atributo | Valor |
|---|---|
| Subcaracterística | Capacidad de ser probado |
| Enunciado EARS | Las pruebas automatizadas deberán cubrir ≥ 80 % de las líneas y ≥ 70 % de las ramas de `src/dominio/`, y la suite completa deberá ejecutarse en 60 s o menos. |
| Métrica | Cobertura gcov/llvm-cov; duración de `ctest` |
| Objetivo / umbral | 80 % / 70 % líneas; 70 % / 60 % ramas; 60 s / 120 s |
| Método | Trabajo `coverage` en CI con informe publicado |
| Prioridad / fuente | M · StR-19 |
| Verificación | P · CP-124 |
| Estado (0.16.0) | **Implementado en 0.1.0**. Suite de 172 pruebas en ≈ 1,4 s. Sin trabajo de cobertura. |

### RNF-025 · Capacidad de análisis
| Atributo | Valor |
|---|---|
| Subcaracterística | Capacidad de ser analizado |
| Enunciado EARS | El código deberá compilar sin advertencias con `-Wall -Wextra -Wpedantic -Werror` (o `/W4 /WX`), pasar `clang-tidy` con el perfil del proyecto, tener complejidad ciclomática ≤ 15 por función y documentación Doxygen en el 100 % de la API pública del dominio. |
| Métrica | Advertencias; hallazgos de clang-tidy; CCN máximo (lizard); % de símbolos públicos documentados |
| Objetivo / umbral | 0; 0; ≤ 15; 100 % / 0; 0; ≤ 20; 90 % |
| Método | CI: compilación estricta, `clang-tidy`, `lizard`, Doxygen con `WARN_IF_UNDOCUMENTED` |
| Prioridad / fuente | S · NEG-06 |
| Verificación | I · CP-125 |
| Estado (0.16.0) | **Implementado en 0.1.0**. -Werror en CI; clang-tidy a mano. Sin lizard ni Doxygen. |

### RNF-026 · Capacidad de modificación
| Atributo | Valor |
|---|---|
| Subcaracterística | Capacidad de ser modificado |
| Enunciado EARS | **Cuando** se agregue un nuevo algoritmo de búsqueda o un nuevo tipo de agente, el cambio deberá requerir solo archivos nuevos más el registro en la fábrica correspondiente (≤ 1 archivo existente modificado). |
| Métrica | Archivos existentes modificados para añadir el algoritmo de prueba «BFS» y el agente de prueba «Corredor» |
| Objetivo / umbral | ≤ 1 / ≤ 3 |
| Método | Ejercicio de cambio documentado (rama `spike/extensibilidad`) |
| Prioridad / fuente | S · ADR-0003, patrones Strategy y Factory |
| Verificación | D · CP-126 |
| Estado (0.16.0) | **Implementado en 0.10.0**. IBuscadorRuta permite añadir buscadores; el ejercicio no se ha hecho. |

---

## 10. Flexibilidad (Portabilidad)

### RNF-028 · Instalabilidad
| Atributo | Valor |
|---|---|
| Subcaracterística | Instalabilidad |
| Enunciado EARS | El proyecto deberá compilarse desde un clon limpio con `cmake --preset release && cmake --build --preset release` en 5 min o menos en CI, y distribuirse como `.zip` portable que se ejecuta al descomprimir. |
| Métrica | Pasos de instalación; tiempo de compilación limpia |
| Objetivo / umbral | 2 comandos y ≤ 5 min / ≤ 4 comandos y ≤ 10 min |
| Método | Trabajo de CI cronometrado; prueba manual del `.zip` en un equipo limpio |
| Prioridad / fuente | M · StR-15, StR-19 |
| Verificación | P · CP-128 |
| Estado (0.16.0) | **Implementado en 0.1.0**. Dos comandos de CMake con presets; no hay .zip portable. |

### RNF-029 · Adaptabilidad de pantalla
| Atributo | Valor |
|---|---|
| Subcaracterística | Adaptabilidad |
| Enunciado EARS | El sistema deberá funcionar en ventana redimensionable y pantalla completa de 1280 × 720 a 3840 × 2160, escalando la vista sin deformación. |
| Métrica | Resoluciones de la lista de prueba sin defectos visuales |
| Objetivo / umbral | 5/5 (720p, 900p, 1080p, 1440p, 2160p) / 3/5 |
| Método | Prueba manual con capturas |
| Prioridad / fuente | S · StR-15 |
| Verificación | D · CP-129 |
| Estado (0.16.0) | **Aprobado**. Pendiente: ventana fija de 1280 × 720 sin escalado. |

### RNF-030 · Escalabilidad
| Atributo | Valor |
|---|---|
| Subcaracterística | Escalabilidad |
| Enunciado EARS | **Mientras** el número de infectados crece de 25 a 200 en un mapa de 256 × 256, el tiempo de IA por tick deberá crecer de forma aproximadamente lineal (pendiente de regresión log-log ≤ 1,3) y mantenerse ≤ 5 ms con 200 agentes. |
| Métrica | Tiempo de IA por tick vs. n agentes |
| Objetivo / umbral | Pendiente ≤ 1,3 y ≤ 5 ms / pendiente ≤ 1,6 y ≤ 10 ms |
| Método | Benchmark de escalado con n ∈ {25, 50, 100, 150, 200}, 5 repeticiones |
| Prioridad / fuente | S · NEG-04 |
| Verificación | P · CP-130 |
| Estado (0.16.0) | **Implementado en 0.15.0**. 100 infectados en 128² sin pasar el presupuesto (CP-026). Falta la serie hasta 200. |

---

## 11. Protección (Safety)

### RNF-031 · Fotosensibilidad
| Atributo | Valor |
|---|---|
| Subcaracterística | Advertencia de riesgo / Fallo seguro |
| Enunciado EARS | El sistema no deberá producir más de 3 destellos por segundo de alto contraste en ninguna zona mayor al 25 % de la pantalla, y deberá permitir desactivar el temblor de cámara y los destellos. |
| Métrica | Destellos por segundo (análisis de vídeo de la Escena 3 y 4) |
| Objetivo / umbral | ≤ 3 / ≤ 3 |
| Método | Análisis de grabación con herramienta PEAT o equivalente |
| Prioridad / fuente | C · Buenas prácticas WCAG 2.3.1 |
| Verificación | A · CP-131 |
| Estado (0.16.0) | **Implementado en 0.7.0**. Solo parpadea la integridad del refugio; sin análisis de destellos. |

---

## 12. Escenarios de calidad priorizados (para arc42 §10)

| ID | Atributo | Estímulo | Respuesta esperada | Medida | RNF |
|---|---|---|---|---|---|
| QS-01 | Desempeño | 100 infectados piden ruta a la vez | Las rutas se reparten en varios ticks y el juego no se congela | p5 ≥ 60 FPS, IA ≤ 2 ms/tick | RNF-001, RNF-003 |
| QS-02 | Corrección | El docente compara A\* y Dijkstra | Mismos costos, A\* expande menos o igual | 100 % de coincidencia | RNF-019 |
| QS-03 | Reproducibilidad | Se repite un experimento con la semilla 42 | Mismos resultados | Hash idéntico en 100 % de ticks | RNF-020 |
| QS-04 | Mantenibilidad | Se agrega un algoritmo BFS | Solo archivos nuevos y registro en fábrica | ≤ 1 archivo modificado | RNF-026 |
| QS-05 | Recursos | Ejecución en portátil de 4 GB | Juego fluido y ligero | RSS ≤ 128 MB, CPU ≤ 35 % | RNF-004, RNF-005 |
| QS-06 | Fiabilidad | Partida corrupta | Mensaje y vuelta al menú | 0 cierres | RNF-016 |
