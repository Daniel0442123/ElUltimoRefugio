# 02 · Historias de usuario y criterios de aceptación

> Formato **Connextra**: *Como* ‹rol› *quiero* ‹capacidad› *para* ‹beneficio›.
> Calidad verificada con **INVEST** (Independiente, Negociable, Valiosa, Estimable, pequeña / *Small*, *Testable*).
> Criterios de aceptación en **Gherkin** en español (`# language: es`), ejecutables como pruebas de aceptación.

| Campo | Valor |
|---|---|
| Versión | 0.1.0 |
| Fecha | 2026-11-21 |
| Estimación | Puntos de historia (Fibonacci: 1, 2, 3, 5, 8, 13). Una historia de 13 o más se divide. |
| Prioridad | MoSCoW (`M`, `S`, `C`, `W`) |

## 1. Roles

| Rol | Parte interesada | Descripción |
|---|---|---|
| Jugadora | STK-04 | Persona que controla a Mara. |
| Docente | STK-01 | Evalúa la solución algorítmica y la arquitectura de agentes. |
| Evaluador | STK-02 | Observa la demostración y la sustentación. |
| Desarrollador | STK-03 | Construye, prueba y experimenta. |

## 2. Épicas

| ID | Épica | Objetivo de negocio | Historias |
|---|---|---|---|
| EP-01 | Juego base: Mara, objetos, refugio y desenlace | NEG-05 | HU-01 a HU-07, HU-20 |
| EP-02 | Navegación y búsqueda de caminos | NEG-02, NEG-04 | HU-16, HU-23 |
| EP-03 | Agentes infectados autónomos y coordinados | NEG-01, NEG-03 | HU-08 a HU-12 |
| EP-04 | Superviviente de la radio | NEG-01 | HU-13 |
| EP-05 | Panel de monitoreo y registro | NEG-01, NEG-06 | HU-14, HU-15, HU-22 |
| EP-06 | Experimentación y optimización | NEG-02, NEG-04 | HU-17, HU-18, HU-21 |
| EP-07 | Persistencia y configuración local | NEG-07 | HU-19 |

## 3. Resumen del backlog

| ID | Título | Épica | Prioridad | Puntos | Escena | RF principales |
|---|---|---|---|---|---|---|
| HU-01 | Moverse en sigilo | EP-01 | M | 3 | 1 | RF-002, RF-003, RF-004 |
| HU-02 | Recoger objetos de misión | EP-01 | M | 3 | 2 | RF-005, RF-006, RF-015, RF-016 |
| HU-03 | Activar la radio del refugio | EP-01 | M | 5 | 4 | RF-007, RF-008 |
| HU-04 | Escapar por la extracción | EP-01 | M | 3 | 5 | RF-009, RF-010 |
| HU-05 | Perder la partida | EP-01 | M | 2 | todas | RF-011, RF-012, RF-055, RF-063 |
| HU-06 | Lanzar un señuelo sonoro | EP-01 | M | 3 | 4 | RF-013 |
| HU-07 | Colocar una barricada | EP-01 | M | 3 | 4 | RF-014, RF-025 |
| HU-08 | Infectados que exploran sin información global | EP-03 | M | 8 | 1 | RF-040 a RF-045, RF-048 |
| HU-09 | Investigar ruidos y propagar alertas | EP-03 | M | 8 | 2 | RF-046, RF-047 |
| HU-10 | Horda que reparte la aproximación | EP-03 | M | 8 | 3 | RF-049, RF-051, RF-029 |
| HU-11 | Horda que reconsidera objetivos ante señuelos | EP-03 | M | 5 | 4 | RF-050, RF-052 |
| HU-12 | Perseguir, explorar o interceptar según evidencias | EP-03 | M | 8 | 5 | RF-053, RF-054 |
| HU-13 | Superviviente de la radio autónomo | EP-04 | S | 8 | 4 | RF-060 a RF-064 |
| HU-14 | Panel de monitoreo del agente | EP-05 | M | 5 | todas | RF-070, RF-071, RF-023 |
| HU-15 | Registro de planes y mensajes | EP-05 | M | 3 | todas | RF-072, RF-073 |
| HU-16 | Comparar Dijkstra y A\* en vivo | EP-02 | M | 5 | todas | RF-020 a RF-024, RF-074 |
| HU-17 | Benchmark reproducible de búsqueda | EP-06 | M | 8 | — | RF-080 a RF-083, RF-086 |
| HU-18 | Comparar agentes aislados y coordinados | EP-06 | M | 5 | — | RF-056, RF-084 |
| HU-19 | Guardar, cargar y configurar | EP-07 | S | 3 | — | RF-090 a RF-093 |
| HU-20 | Tutorial de sigilo | EP-01 | S | 2 | 1 | RF-017 |
| HU-21 | Optimizar parámetros de la IA | EP-06 | S | 8 | — | RF-085 |
| HU-22 | Controlar la simulación para explicarla | EP-05 | S | 2 | todas | RF-075 |
| HU-23 | Rutas para la horda sin congelar el juego | EP-02 | M | 5 | 3 | RF-026, RF-027, RF-028, RF-030 |

Total estimado: 113 puntos. Las historias `M` suman 90 puntos y las `S` suman 23.

---

## 4. Historias detalladas

### HU-01 · Moverse en sigilo

**Como** jugadora, **quiero** mover a Mara caminando, corriendo o en sigilo, **para** decidir cuánto ruido hago y evitar atraer a los infectados.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-01 · M · 3 |
| Fuente | STB §01, Escena 1 |
| RF | RF-002, RF-003, RF-004, RF-016 |
| RNF | RNF-001, RNF-012 |
| Pruebas | CP-002, CP-003, CP-004 |

```gherkin
# language: es
Característica: Desplazamiento de Mara con distintos niveles de ruido

  Antecedentes:
    Dado un mapa de prueba de 32x32 celdas sin infectados
    Y Mara en la celda (5,5)

  Esquema del escenario: Cada modo de movimiento tiene velocidad y ruido propios
    Cuando la jugadora mueve a Mara en modo "<modo>" durante 1 segundo hacia el este
    Entonces Mara avanza entre <min> y <max> celdas
    Y el sistema emite eventos de ruido de radio <radio> celdas

    Ejemplos:
      | modo     | min | max | radio |
      | sigilo   | 1.4 | 1.6 | 1     |
      | caminar  | 2.9 | 3.1 | 3     |
      | correr   | 5.9 | 6.1 | 6     |

  Escenario: Mara no atraviesa paredes
    Dado una pared en la celda (6,5)
    Cuando la jugadora mueve a Mara hacia el este durante 1 segundo
    Entonces Mara permanece en la columna 5
```

**INVEST**: I ✔ no depende de otras HU · N ✔ velocidades y radios ajustables · V ✔ base de la mecánica de sigilo · E ✔ · S ✔ 3 puntos · T ✔ escenarios medibles.

---

### HU-02 · Recoger objetos de misión

**Como** jugadora, **quiero** recoger medicamentos, combustible y la batería, **para** reunir lo necesario para activar la radio.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-01 · M · 3 |
| Fuente | STB §01, Escena 2 |
| RF | RF-005, RF-006, RF-015, RF-016 |
| Pruebas | CP-005, CP-006, CP-015 |

```gherkin
# language: es
Característica: Recolección de objetos

  Escenario: Recoger un objeto cercano
    Dado Mara a 1 celda de un "medicamento"
    Cuando la jugadora pulsa la tecla de interactuar
    Entonces el inventario contiene 1 "medicamento"
    Y el objeto desaparece del mapa
    Y el HUD muestra el objetivo pendiente siguiente

  Escenario: No se recoge un objeto lejano
    Dado Mara a 3 celdas de un "combustible"
    Cuando la jugadora pulsa la tecla de interactuar
    Entonces el inventario no cambia

  Escenario: La puerta metálica de la farmacia hace ruido
    Dado Mara junto a la puerta metálica cerrada de la farmacia
    Cuando la jugadora abre la puerta
    Entonces el sistema emite un evento de ruido de radio 10 celdas con origen en la puerta

  Escenario: Usar un medicamento recupera salud
    Dado Mara con 40 de salud y 1 "medicamento" de sobra
    Cuando la jugadora usa el medicamento
    Entonces la salud de Mara es 75
```

**INVEST**: I ✔ · N ✔ · V ✔ · E ✔ · S ✔ · T ✔.

---

### HU-03 · Activar la radio del refugio

**Como** jugadora, **quiero** depositar los objetos e instalar la batería en la radio, **para** pedir ayuda y abrir la extracción.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-01 · M · 5 |
| Fuente | STB §01, Escena 4 |
| RF | RF-007, RF-008 |
| Pruebas | CP-007, CP-008 |

```gherkin
# language: es
Característica: Instalación de la batería en la radio

  Escenario: Instalación completa
    Dado Mara junto a la radio con "batería", "combustible" y "medicamento" en el inventario
    Cuando la jugadora mantiene la tecla de interactuar durante 8 segundos sin recibir daño
    Entonces la radio queda "activada"
    Y se publica el evento "RadioActivada"

  Escenario: Faltan objetos
    Dado Mara junto a la radio solo con "batería"
    Cuando la jugadora intenta instalarla
    Entonces la instalación no comienza
    Y el HUD indica "Faltan: combustible, medicamento"

  Escenario: El daño interrumpe la instalación
    Dado una instalación en curso con 5 segundos transcurridos
    Cuando un infectado inflige daño a Mara
    Entonces la instalación se interrumpe
    Y el progreso vuelve a 0
```

**INVEST**: I ✔ (usa inventario de HU-02 mediante datos de prueba) · N ✔ duración ajustable · V ✔ · E ✔ · S ✔ · T ✔.

---

### HU-04 · Escapar por la extracción

**Como** jugadora, **quiero** que la radio abra una ruta de escape temporal, **para** ganar la partida si llego a tiempo.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-01 · M · 3 |
| Fuente | STB §01, Escena 5 |
| RF | RF-009, RF-010 |
| Pruebas | CP-009, CP-010 |

```gherkin
# language: es
Característica: Extracción

  Escenario: Ganar dentro de la ventana
    Dado la radio activada hace 10 segundos
    Y la ventana de extracción dura 60 segundos
    Cuando Mara entra en la zona de extracción
    Entonces la partida termina con resultado "Victoria"

  Escenario: Llegar tarde
    Dado la radio activada hace 61 segundos
    Cuando Mara entra en la zona de extracción
    Entonces la zona está cerrada
    Y la partida continúa
```

---

### HU-05 · Perder la partida

**Como** jugadora, **quiero** perder cuando mi salud o la integridad del refugio llegan a cero, **para** que mis decisiones tengan consecuencias.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-01 · M · 2 |
| Fuente | STB §01 Objetivo y desenlace |
| RF | RF-011, RF-012, RF-055, RF-063 |
| Pruebas | CP-011, CP-012, CP-055, CP-063 |

```gherkin
# language: es
Característica: Condiciones de derrota

  Escenario: Salud a cero
    Dado Mara con 5 de salud
    Y un infectado adyacente en estado "Atacar"
    Cuando pasa 1 segundo de simulación
    Entonces la salud de Mara es 0
    Y la partida termina con resultado "Derrota: Mara ha caído"

  Escenario: Refugio inutilizado
    Dado la integridad del refugio en 3
    Cuando un infectado golpea la puerta del refugio
    Entonces la integridad es 0
    Y la partida termina con resultado "Derrota: refugio inutilizado"
```

---

### HU-06 · Lanzar un señuelo sonoro

**Como** jugadora, **quiero** lanzar un señuelo que haga ruido en otro lugar, **para** desviar a la horda mientras trabajo en el refugio.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-01 · M · 3 |
| Fuente | STB §03 Prueba de aceptación; Escena 4 |
| RF | RF-013 |
| Pruebas | CP-013, CP-201 |

```gherkin
# language: es
Característica: Señuelo sonoro

  Escenario: Lanzar dentro del alcance
    Dado Mara con 1 señuelo
    Cuando la jugadora apunta a una celda transitable a 6 celdas y lanza
    Entonces aparece un señuelo en esa celda
    Y el señuelo emite un ruido de radio 8 celdas cada 1 segundo durante 10 segundos
    Y el inventario queda con 0 señuelos

  Escenario: Fuera de alcance
    Cuando la jugadora apunta a una celda a 12 celdas
    Entonces el señuelo cae en la celda más lejana alcanzable de la línea, a 8 celdas como máximo
```

---

### HU-07 · Colocar una barricada

**Como** jugadora, **quiero** colocar barricadas en celdas adyacentes, **para** cortar el paso de los infectados y ganar tiempo.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-01 · M · 3 |
| Fuente | STB §03 Prueba de aceptación |
| RF | RF-014, RF-025 |
| Pruebas | CP-014, CP-025, CP-201 |

```gherkin
# language: es
Característica: Barricadas

  Escenario: Colocar barricada
    Dado Mara con 1 barricada y una celda adyacente transitable y libre
    Cuando la jugadora coloca la barricada en esa celda
    Entonces la celda pasa a ser no transitable en el grafo
    Y se publica el evento "CambioDeMapa" con la celda afectada

  Escenario: Celda ocupada
    Dado un infectado en la celda adyacente
    Cuando la jugadora intenta colocar la barricada allí
    Entonces la barricada no se coloca
```

---

### HU-08 · Infectados que exploran sin información global

**Como** docente, **quiero** que los infectados exploren la ciudad usando solo lo que perciben y recuerdan, **para** comprobar que su autonomía no depende de información omnisciente.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-03 · M · 8 |
| Fuente | STB Escena 1; STB §03; GUIA §4 |
| RF | RF-040, RF-041, RF-042, RF-043, RF-044, RF-045, RF-048 |
| RNF | RNF-002, RNF-023 |
| Pruebas | CP-040 a CP-045, CP-048 |

```gherkin
# language: es
Característica: Exploración autónoma sin omnisciencia

  Escenario: Un infectado no conoce la posición de Mara si no la percibe
    Dado un infectado explorador en (10,10) mirando al norte
    Y Mara en (10,20) detrás de una pared y en sigilo
    Cuando pasan 5 segundos de simulación
    Entonces la memoria del infectado no contiene evidencias de Mara
    Y su estado es "Explorar"

  Escenario: Visión con línea de visión
    Dado un infectado explorador en (10,10) mirando al este
    Y Mara en (15,10) sin obstáculos entre ambos
    Cuando se ejecuta un tick de IA
    Entonces la memoria del infectado contiene una evidencia "Avistamiento" en (15,10) con confianza 1.0

  Escenario: La exploración cubre zonas nuevas
    Dado un infectado explorador solo en un mapa de 64x64
    Cuando pasan 120 segundos de simulación
    Entonces el infectado ha visitado al menos el 30 % de las celdas transitables
    Y no ha repetido una misma celda objetivo de exploración más de 3 veces
```

**INVEST**: I ✔ · N ✔ (umbrales de cobertura ajustables) · V ✔ (criterio de 25 % de la nota) · E ✔ · S ⚠ 8 puntos, se divide en tareas: percepción, memoria y exploración · T ✔.

---

### HU-09 · Investigar ruidos y propagar alertas

**Como** docente, **quiero** que los infectados que oyen un ruido lo investiguen y avisen a sus vecinos mediante una propagación recursiva acotada, **para** evaluar la recursividad y la comunicación multiagente.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-03 · M · 8 |
| Fuente | STB Escena 2; STB §03 Recursividad |
| RF | RF-043, RF-046, RF-047, RF-073 |
| RNF | RNF-027 |
| Pruebas | CP-046, CP-047, CP-073 |

```gherkin
# language: es
Característica: Investigación de ruidos y alertas recursivas

  Escenario: Investigar el origen de un ruido
    Dado un infectado en (20,20) en estado "Explorar"
    Cuando se emite un ruido de radio 10 en (25,20)
    Entonces en el siguiente tick de IA su estado es "Investigar"
    Y su objetivo de ruta es la celda (25,20)

  Escenario: La alerta se propaga con profundidad máxima
    Dado una cadena de infectados A-B-C-D-E, cada uno a 4 celdas del siguiente
    Y el radio de comunicación es 5 celdas
    Y la profundidad máxima de propagación es 3
    Cuando A detecta a Mara
    Entonces B, C y D reciben la alerta con saltos 1, 2 y 3
    Y E no recibe la alerta
    Y la confianza de la alerta en D es menor que en B

  Escenario: Los ciclos no generan bucles infinitos
    Dado tres infectados X, Y, Z todos dentro del radio de comunicación entre sí
    Cuando X emite una alerta
    Entonces cada infectado recibe la alerta exactamente una vez
    Y el registro muestra 2 mensajes enviados
```

---

### HU-10 · Horda que reparte la aproximación

**Como** docente, **quiero** que los zombis se repartan los accesos al objetivo y no se amontonen en una celda, **para** observar coordinación espacial.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-03 · M · 8 |
| Fuente | STB Escena 3 |
| RF | RF-049, RF-051, RF-029, RF-030 |
| Pruebas | CP-049, CP-051, CP-029 |

```gherkin
# language: es
Característica: Distribución de la aproximación

  Escenario: Ningún punto de llegada se sobrecarga
    Dado 12 infectados perseguidores con la misma evidencia de Mara en (40,40)
    Cuando se asignan los puntos de aproximación
    Entonces ninguna celda destino tiene más de 2 infectados asignados
    Y los destinos asignados cubren al menos 3 accesos distintos al objetivo

  Escenario: Separación durante el movimiento
    Dado 12 infectados moviéndose hacia el mismo objetivo
    Cuando pasan 10 segundos de simulación
    Entonces en ningún tick hay más de 2 infectados ocupando la misma celda
```

---

### HU-11 · Horda que reconsidera objetivos ante señuelos

**Como** docente, **quiero** que la horda cambie de objetivo cuando oye un señuelo más prometedor que su evidencia actual, **para** validar la replanificación ante cambios.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-03 · M · 5 |
| Fuente | STB Escena 4; STB §03 Prueba de aceptación |
| RF | RF-043, RF-050, RF-052, RF-072 |
| Pruebas | CP-050, CP-052, CP-201 |

```gherkin
# language: es
Característica: Reevaluación de objetivos

  Escenario: Señuelo que supera a una evidencia antigua
    Dado un infectado en "Investigar" hacia una evidencia de hace 20 segundos con confianza 0.3
    Cuando oye un señuelo a 6 celdas
    Entonces en 500 ms o menos su objetivo pasa a ser la celda del señuelo
    Y el registro contiene un "CambioDePlan" con motivo "señuelo: utilidad 0.71 > 0.30"

  Escenario: Avistamiento directo pesa más que un señuelo
    Dado un infectado que ve a Mara a 3 celdas
    Cuando oye un señuelo a 6 celdas
    Entonces mantiene el estado "Perseguir" hacia Mara

  Escenario: Barricada en la ruta
    Dado un infectado con ruta que pasa por (30,12)
    Cuando la jugadora coloca una barricada en (30,12)
    Entonces el infectado replanifica una ruta que no pasa por (30,12)
    Pero si no existe ruta alternativa con costo menor a 2 veces el original, ataca la barricada
```

---

### HU-12 · Perseguir, explorar o interceptar según evidencias

**Como** docente, **quiero** que cada infectado elija entre perseguir, explorar o interceptar según sus evidencias, **para** ver decisiones distintas y justificadas durante la extracción.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-03 · M · 8 |
| Fuente | STB Escena 5 |
| RF | RF-053, RF-054, RF-049 |
| Pruebas | CP-053, CP-054 |

```gherkin
# language: es
Característica: Elección de táctica

  Esquema del escenario: La táctica depende de la evidencia
    Dado un infectado con evidencia "<evidencia>" de antigüedad <edad> s
    Cuando el infectado evalúa sus opciones
    Entonces elige la táctica "<tactica>"

    Ejemplos:
      | evidencia               | edad | tactica     |
      | avistamiento cercano    | 0    | Perseguir   |
      | avistamiento con rumbo  | 2    | Interceptar |
      | alerta de 3 saltos      | 15   | Explorar    |
      | ninguna                 | -    | Explorar    |

  Escenario: Interceptar en un punto de corte
    Dado Mara avanzando hacia el este a 3 celdas/s por una calle
    Y un infectado perseguidor más cerca del cruce siguiente que de Mara
    Cuando el infectado elige "Interceptar"
    Entonces su destino es una celda de la ruta prevista de Mara a 2 s o más por delante
```

---

### HU-13 · Superviviente de la radio autónomo

**Como** jugadora, **quiero** que el superviviente de la radio repare el refugio, refuerce accesos y me ayude con la instalación, **para** sentir que tengo un aliado que decide por sí mismo.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-04 · S · 8 |
| Fuente | STB §01 Personajes |
| RF | RF-060, RF-061, RF-062, RF-063, RF-064 |
| Pruebas | CP-060 a CP-064 |

```gherkin
# language: es
Característica: Superviviente con utilidad, GOAP y árbol de comportamiento

  Escenario: Prioriza reparar cuando el refugio está dañado
    Dado la integridad del refugio en 30 %
    Y ningún infectado a menos de 8 celdas
    Cuando el superviviente evalúa sus metas
    Entonces elige la meta "RefugioSeguro"
    Y su plan GOAP contiene las acciones "RecogerTablas" y "RepararPuerta" en ese orden

  Escenario: Se pone a salvo ante peligro inmediato
    Dado un infectado a 2 celdas del superviviente
    Cuando el superviviente evalúa sus metas
    Entonces elige la meta "PonerseASalvo"

  Escenario: Replanifica si una acción falla
    Dado un plan que incluye "RecogerTablas" en el almacén
    Cuando el almacén queda bloqueado por una barricada
    Entonces el superviviente descarta el plan y genera uno nuevo en el siguiente tick de IA
```

---

### HU-14 · Panel de monitoreo del agente

**Como** docente, **quiero** seleccionar un agente y ver su objetivo, estado, percepción, ruta, algoritmo, heurística, costo, nodos expandidos y tiempo de búsqueda, **para** evaluar sus decisiones sin leer el código.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-05 · M · 5 |
| Fuente | GUIA §6; STB §03 Requisitos comunes |
| RF | RF-023, RF-070, RF-071 |
| RNF | RNF-013 |
| Pruebas | CP-070, CP-071 |

```gherkin
# language: es
Característica: Panel de monitoreo

  Escenario: Ver los datos de un agente
    Dado el panel activado con F1
    Cuando el docente hace clic sobre un infectado
    Entonces el panel muestra los campos: objetivo, estado, percepción relevante, ruta, algoritmo, heurística, costo, nodos expandidos y tiempo de búsqueda en microsegundos
    Y la ruta del agente se dibuja sobre el mapa

  Escenario: Ver los nodos expandidos
    Dado un agente seleccionado que acaba de planificar con A*
    Cuando el docente activa la superposición de nodos expandidos
    Entonces cada nodo cerrado se pinta en el mapa
    Y el número de nodos pintados es igual al valor "nodos expandidos" del panel
```

---

### HU-15 · Registro de planes y mensajes

**Como** docente, **quiero** un registro de cambios de plan y de mensajes entre agentes, **para** reconstruir por qué la horda actuó como actuó.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-05 · M · 3 |
| Fuente | GUIA §6 |
| RF | RF-072, RF-073 |
| Pruebas | CP-072, CP-073 |

```gherkin
# language: es
Característica: Registro de eventos de IA

  Escenario: Se registra un cambio de plan
    Cuando un agente cambia de objetivo
    Entonces se añade una línea con tick, id del agente, plan anterior, plan nuevo y motivo
    Y la línea aparece en la consola del panel y en el archivo "logs/ia_<semilla>.log"

  Escenario: Se registra un mensaje entre agentes
    Cuando un agente envía una alerta a otro
    Entonces se registra tick, emisor, receptor, tipo de mensaje, salto y confianza
```

---

### HU-16 · Comparar Dijkstra y A\* en vivo

**Como** docente, **quiero** alternar entre Dijkstra y A\* durante la partida y ver ambos resultados para la misma consulta, **para** comparar optimalidad y esfuerzo con datos reales del juego.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-02 · M · 5 |
| Fuente | GUIA §5, §6; STB §03 |
| RF | RF-020, RF-021, RF-022, RF-023, RF-024, RF-074 |
| RNF | RNF-003, RNF-019 |
| Pruebas | CP-020 a CP-024, CP-074 |

```gherkin
# language: es
Característica: Comparación Dijkstra / A* en el juego

  Escenario: Misma consulta con ambos algoritmos
    Dado un agente seleccionado con origen (3,3) y destino (60,50) en el mapa de 64x64
    Cuando el docente pulsa F3 "Comparar"
    Entonces el panel muestra una tabla con Dijkstra y A* para esa consulta
    Y ambos tienen el mismo costo de ruta con diferencia menor a 1e-6
    Y los nodos expandidos de A* son menores o iguales a los de Dijkstra

  Escenario: Cambiar el algoritmo activo
    Cuando el docente pulsa F2
    Entonces el algoritmo de todos los agentes alterna entre "A*" y "Dijkstra"
    Y el panel muestra el algoritmo activo
```

---

### HU-17 · Benchmark reproducible de búsqueda

**Como** docente, **quiero** ejecutar un experimento sin ventana que compare Dijkstra y A\* en varios tamaños de mapa con semillas registradas, **para** obtener tablas reales y reproducibles de tiempo, costo, nodos y memoria.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-06 · M · 8 |
| Fuente | GUIA §7; STB §03 |
| RF | RF-080, RF-081, RF-082, RF-083, RF-086 |
| RNF | RNF-020, RNF-032 |
| Pruebas | CP-080 a CP-083, CP-086 |

```gherkin
# language: es
Característica: Benchmark de búsqueda

  Escenario: Ejecución completa
    Dado el archivo "bench/config.json" con tamaños 32, 64, 128 y 256, 20 pares origen-destino y 5 repeticiones
    Cuando el desarrollador ejecuta "refugio --bench bench/config.json"
    Entonces se crea "results/bench_<fecha>_<semilla>.csv"
    Y el CSV contiene 2 x 4 x 20 x 5 = 800 filas de medición
    Y cada fila incluye algoritmo, tamaño, semilla, origen, destino, costo, nodos expandidos, tiempo en microsegundos y memoria pico en bytes
    Y la cabecera incluye CPU, RAM, sistema operativo, compilador y versión del juego

  Escenario: Reproducibilidad
    Dado dos ejecuciones con la misma configuración y semilla 42
    Entonces los costos y nodos expandidos son idénticos en ambas

  Escenario: Entorno dinámico
    Dado un escenario con una barricada que aparece en el tick 30
    Cuando se ejecuta el benchmark dinámico
    Entonces se registran las replanificaciones y el costo final de cada algoritmo
```

---

### HU-18 · Comparar agentes aislados y coordinados

**Como** docente, **quiero** comparar la horda con y sin comunicación usando la misma semilla, **para** medir el beneficio real de la coordinación.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-06 · M · 5 |
| Fuente | STB §03 |
| RF | RF-056, RF-084 |
| Pruebas | CP-056, CP-084 |

```gherkin
# language: es
Característica: Aislados frente a coordinados

  Escenario: Experimento pareado
    Dado la semilla 7 y el escenario "farmacia"
    Cuando se ejecuta "refugio --coord-exp 7 --reps 10"
    Entonces se ejecutan 10 simulaciones en modo "aislado" y 10 en modo "coordinado" con un guion de jugadora fijo
    Y el CSV registra por simulación: tiempo hasta primera detección, tiempo hasta contacto, cobertura explorada, mensajes enviados y celdas sobrecargadas

  Escenario: Modo aislado en vivo
    Cuando el docente pulsa F4 durante la partida
    Entonces la comunicación entre infectados se desactiva
    Y el panel muestra "Modo: aislado"
```

---

### HU-19 · Guardar, cargar y configurar

**Como** jugadora, **quiero** guardar y cargar la partida y cambiar controles y volumen, **para** continuar otro día y jugar cómoda.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-07 · S · 3 |
| Fuente | Acuerdo del equipo |
| RF | RF-090, RF-091, RF-092, RF-093 |
| RNF | RNF-017, RNF-021 |
| Pruebas | CP-090 a CP-093 |

```gherkin
# language: es
Característica: Persistencia local

  Escenario: Guardar y cargar
    Dado una partida en la escena 3
    Cuando la jugadora guarda en la ranura 1 y luego carga la ranura 1
    Entonces el estado cargado tiene el mismo hash de estado que el guardado

  Escenario: Archivo de guardado corrupto
    Dado un archivo "saves/slot1.sav" con un byte alterado
    Cuando la jugadora carga la ranura 1
    Entonces aparece el mensaje "La partida guardada está dañada"
    Y el juego sigue en el menú sin cerrarse
```

---

### HU-20 · Tutorial de sigilo

**Como** jugadora nueva, **quiero** un tutorial breve en la Escena 1, **para** aprender a moverme en silencio antes de que aparezca el peligro.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-01 · S · 2 |
| Fuente | STB Escena 1 |
| RF | RF-017 |
| RNF | RNF-011 |
| Pruebas | CP-017, CP-111 |

```gherkin
# language: es
Característica: Tutorial

  Escenario: Pasos del tutorial
    Dado una partida nueva
    Cuando comienza la Escena 1
    Entonces se muestran en orden las indicaciones: moverse, sigilo, correr hace ruido, recoger objeto
    Y cada indicación desaparece cuando la jugadora realiza la acción

  Escenario: Saltar el tutorial
    Cuando la jugadora pulsa "Saltar tutorial"
    Entonces la Escena 1 continúa sin indicaciones
```

---

### HU-21 · Optimizar parámetros de la IA

**Como** desarrollador, **quiero** ajustar los pesos de utilidad y de steering con un algoritmo genético o con recocido simulado, **para** obtener una horda exigente pero justa sin ajustar a mano.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-06 · S · 8 |
| Fuente | Objetivo general (optimización) |
| RF | RF-085, RF-086 |
| Pruebas | CP-085 |

```gherkin
# language: es
Característica: Optimización offline

  Escenario: Algoritmo genético
    Dado una población de 30 individuos, 40 generaciones y semilla 11
    Cuando se ejecuta "refugio --optimize ga --config opt/ga.json"
    Entonces se escribe "results/opt_ga_11.json" con el mejor individuo, su aptitud y la aptitud media por generación
    Y la aptitud del mejor individuo nunca disminuye entre generaciones (elitismo)

  Escenario: Recocido simulado
    Cuando se ejecuta "refugio --optimize sa --config opt/sa.json"
    Entonces se registran temperatura, solución actual y mejor solución por iteración
```

---

### HU-22 · Controlar la simulación para explicarla

**Como** evaluador, **quiero** pausar, avanzar tick a tick y cambiar la velocidad de simulación, **para** seguir con calma una decisión durante la sustentación.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-05 · S · 2 |
| Fuente | GUIA §8 Demostración en vivo |
| RF | RF-075 |
| Pruebas | CP-075 |

```gherkin
# language: es
Característica: Control de la simulación

  Escenario: Paso a paso
    Dado la simulación en pausa
    Cuando el evaluador pulsa "N"
    Entonces se ejecuta exactamente 1 tick de simulación
    Y el panel se actualiza

  Escenario: Velocidad
    Cuando el evaluador elige velocidad 0.25x
    Entonces la simulación avanza 15 ticks por segundo real en lugar de 60
```

---

### HU-23 · Rutas para la horda sin congelar el juego

**Como** jugadora, **quiero** que decenas de infectados busquen rutas sin que el juego se trabe, **para** que la horda sea creíble incluso en mi portátil.

| Atributo | Valor |
|---|---|
| Épica / prioridad / puntos | EP-02 · M · 5 |
| Fuente | Escena 3; NEG-07 |
| RF | RF-026, RF-027, RF-028, RF-030 |
| RNF | RNF-001, RNF-002, RNF-030 |
| Pruebas | CP-026, CP-027, CP-028, CP-030, CP-101 |

```gherkin
# language: es
Característica: Planificación con presupuesto

  Escenario: Muchas solicitudes en el mismo tick
    Dado 100 infectados que solicitan ruta en el mismo tick
    Cuando se procesa la cola de planificación
    Entonces el tiempo de IA por tick no supera 2 ms
    Y todas las solicitudes se resuelven en 10 ticks de IA o menos

  Escenario: Destino común con flow field
    Dado 20 infectados con el mismo destino
    Entonces el sistema calcula un único flow field para ese destino
    Y ninguno de esos agentes ejecuta A* individual

  Escenario: Sin ruta posible
    Dado un destino encerrado por barricadas
    Cuando un infectado solicita ruta hacia él
    Entonces la búsqueda devuelve "SinRuta"
    Y el infectado cambia a "Explorar" o "AtacarBarricada" sin bloquear el juego
```

---

## 5. Definición de terminado (DoD) para una historia

1. Todos sus escenarios Gherkin pasan como pruebas automatizadas (Catch2 con escenarios `SCENARIO/GIVEN/WHEN/THEN`).
2. Los RF y RNF enlazados están en estado `Implementado` y sus CP en `Aprobado`.
3. Sin errores de ASan/UBSan ni advertencias del compilador (`-Wall -Wextra -Werror`).
4. Cobertura del dominio ≥ 80 % de líneas (RNF-024).
5. Documentación Doxygen de la API pública tocada.
6. Matriz de trazabilidad actualizada.
7. Revisión de código por otro miembro del equipo.
