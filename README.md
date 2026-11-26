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

## Estado actual (0.17.0)

**Ventana adaptable y tipografía legible** (ADR-0021).

- **Cualquier pantalla**: la ventana se abre maximizada, se puede redimensionar, maximizar y
  restaurar como cualquier programa, y **F11** alterna la pantalla completa (sin bordes, no cambia
  la resolución del monitor). Tamaño mínimo 800 × 450.
- **El HUD escala con la ventana**: la interfaz se diseña sobre 1280 × 720 unidades y cada frame se
  calcula la escala (1,5 en 1920 × 1080, 3 en 3840 × 2160). En pantallas más anchas o más altas que
  16:9 el espacio sobrante va al mapa y el panel lateral sigue pegado al borde derecho; el registro
  de IA y los avisos se colocan sobre el borde inferior del mapa.
- **El mapa llena su zona**: se amplía para caber entero (Z: mapa completo) o sigue a Mara con zoom
  2× sobre ese tamaño. El arte pixel no cambia; si sobra espacio, el mapa queda centrado.
- **Tipografía nueva**: los textos usan **Inter** (sans-serif, licencia SIL OFL 1.1, en
  `assets/fuentes/`) en lugar de la fuente pixelada de raylib. Se incrusta en el ejecutable y se
  rasteriza al tamaño final en píxeles, así se ve nítida a cualquier escala, e incluye tildes, ñ,
  ¿, ¡, « y ». Los títulos van en negrita.
- `refugio --captura archivo.png --ventana 1920x1080` guarda capturas a otras resoluciones.
- La simulación, las partidas guardadas y los experimentos no cambian.

### Versión 0.16.0

**Arte pixel con linterna y sombras** (ADR-0017). El mundo deja de ser una rejilla de cuadros y
círculos.

- **Ciudad de noche**: calles de asfalto, aceras junto a los edificios, azoteas con pretil, aires
  acondicionados y claraboyas, fachadas de ladrillo con ventanas (algunas encendidas), escombros de
  piedras y tablas donde el terreno es lento, alcantarillas, grietas y manchas.
- **Sprites originales** de 16 × 16 px dibujados al doble: infectados en 4 variantes con 2 fotogramas
  de paso y ojos rojos que se ven en la oscuridad, Mara con su linterna, el superviviente, botiquín,
  bidón, batería, radio, almacén, barricada y bengala. Todo se genera por código: no hay archivos de
  imagen ni arte de terceros.
- **Luz con sombras**: la linterna de Mara es un cono que los muros cortan (más corto en sigilo);
  la radio, la extracción abierta, las bengalas, el superviviente y las ventanas también iluminan.
  Cuando hieren a Mara queda sangre en el suelo.
- **Cámara** (Z): sigue a Mara con zoom 2× o enseña el mapa completo. El clic del monitor y el
  señuelo apuntan bien en los dos modos.
- **La IA sigue a la vista**: F1 quita la oscuridad y marca el estado de cada infectado con un
  anillo de color; los nodos expandidos, rutas, alertas y el monitor se dibujan encima del arte.
  Sin F1, «?» indica que un infectado oyó algo o la busca y «!» que la ve o ataca.
- **Capturas**: `refugio --captura archivo.png [--frames N] [--mapa] [--depuracion]` juega N frames
  y guarda la pantalla.
- La simulación no cambia: la decoración usa un hash propio y no el generador de la partida, así
  que las partidas guardadas y los resultados de los experimentos son los mismos.

### Versión 0.15.0

**Rutas para la horda sin congelar el juego** (HU-23, RF-026, RF-027, ADR-0007).

- **A\* iterativo**: una búsqueda ya no tiene que terminar en el tick en que empieza. El buscador
  guarda la abierta, g, padre y cerrado y sigue en el tick siguiente donde se quedó, con exactamente
  la misma ruta, el mismo costo y el mismo orden de expansión que si la hubiera hecho de una vez.
- **Presupuesto por tick de IA**: entre todos los infectados no se expanden más de
  `presupuesto_nodos_tick` nodos (4000 en `config/ia.json`). Antes se comprobaba solo al empezar cada
  búsqueda y un tick con muchas búsquedas largas podía pasarse; ahora el límite es exacto.
- **Cola por prioridad y antigüedad**: perseguir, interceptar y atacar una barricada van primero,
  luego investigar y buscar, y al final explorar. Cada 0,5 s de espera una solicitud sube un nivel,
  así nadie se queda esperando para siempre. Quien pasa al campo de flujo o deja de necesitar ruta
  sale de la cola; si el mapa cambia (barricada) o se pulsa F2, la búsqueda a medias empieza de nuevo.
- **Sin ruta** (RF-027): una búsqueda hacia un destino encerrado también se reparte en varios ticks
  y, al agotarse, el infectado cambia de comportamiento en el tick de IA siguiente.
- **Panel**: la línea del algoritmo muestra los nodos del último tick sobre el presupuesto
  (`12/4000`) y la siguiente, las solicitudes en cola (`+1` si hay una a medias), el tiempo de
  búsqueda del tick y la mayor espera de la partida en ticks de IA.
- **Medido** (pruebas CP-026, Release, Xeon 2,3 GHz): 100 solicitudes en el mismo tick en una
  ciudad de 128 × 128 se resuelven en 5 ticks, sin pasar de 4000 nodos ni de 0,7 ms por tick. Una
  partida de 100 infectados en 128 × 128 durante 60 s hizo 1477 búsquedas con una espera máxima de
  4 ticks de IA. El benchmark de A\* (RNF-003) no cambia: unos 85 µs de mediana en 128 × 128.
- Las partidas guardadas con la 0.14.0 o antes pueden dar «otra versión del juego» al cargarlas si
  en algún tick se llegó al presupuesto, porque ahora ese tick se reparte de otra forma.

### Versión 0.14.0

**Controlar la simulación para explicarla** (HU-22, RF-075, ADR-0005).

- **P** pausa y reanuda la simulación; **F6**, en pausa, avanza exactamente un tick (1/60 s);
  **F7** y **F8** bajan y suben la velocidad entre 0,25×, 0,5×, 1×, 2× y 4× (15 a 240 ticks por
  segundo real). Las cuatro teclas se pueden cambiar en Esc > Opciones > Controles.
- En pausa se puede seguir seleccionando infectados, abrir su monitor, la comparación F3, los nodos
  expandidos y la consola, así que una decisión se puede explicar tick a tick. Un rótulo arriba del
  mapa recuerda la pausa o la velocidad, y la franja bajo el mapa muestra semilla, tick y velocidad.
- El control solo decide cuántos pasos fijos corren en cada frame (`juego::ControlSimulacion`);
  la partida recibe los mismos pasos con las mismas entradas a cualquier velocidad, así que el
  resultado no cambia: 600 ticks a 0,25×, 1×, 4× o pulsando F6 dan la misma huella de estado (CP-075).
- Un `config/controles.json` de una versión anterior se sigue leyendo: si ya usabas P o F6 a F8
  para otra acción, las acciones nuevas reciben teclas libres en vez de perder tu configuración.
- El panel muestra los estados de los infectados en dos columnas para que quepa la nueva línea
  de ayuda.

### Versión 0.13.0

**Optimizar los parámetros de la IA con algoritmo genético y recocido simulado** (HU-21, RF-085,
ADR-0015).

- **Qué se optimiza**: once pesos de la horda, que ahora están en la sección `"pesos"` de
  `config/ia.json` y el juego lee al iniciar: los de steering (`seguir`, `separacion`,
  `alineacion`, `cohesion`, `anticipacion`), los de utilidad de las tácticas (`utilidad_explorar`,
  `peso_interceptar`, `histeresis`) y la dificultad del perseguidor (`vision_radio`,
  `vel_perseguir`, `danio_por_segundo`). Los valores del repositorio son los de siempre, así que el
  juego se comporta igual que en 0.12.0 mientras no se cambien.
- **Cómo se ejecuta**: `refugio --optimize ga --config opt/ga.json` o
  `refugio --optimize sa --config opt/sa.json` (también con `refugio_bench`, sin ventana). Escribe
  `results/opt_ga_11.json` o `results/opt_sa_11.json`. Opciones: `--semilla`, `--salida` y
  `--hilos` (por defecto, todos los núcleos; el número de hilos no cambia el resultado).
- **Algoritmo genético** (`opt/ga.json`): 30 individuos, 40 generaciones, torneo de 3, cruce
  uniforme (0,9), mutación gaussiana σ = 0,1 con probabilidad 0,2 por gen y elitismo de 2. Los
  pesos actuales entran como primer individuo. El JSON guarda por generación la aptitud mejor,
  media y peor y el mejor individuo; la del mejor nunca baja (si bajara, el programa termina con
  código 5).
- **Recocido simulado** (`opt/sa.json`): 2000 iteraciones desde los pesos actuales, temperatura
  inicial 0,05 y enfriamiento geométrico α = 0,995. El vecino suma ruido gaussiano σ = 0,1 a todos
  los genes. Por iteración se guardan la temperatura, la aptitud del vecino, la actual y la mejor,
  si se aceptó y la solución actual.
- **Aptitud**: cada candidato juega 8 partidas sin ventana (semillas 11 a 18, horda coordinada y el
  guion fijo de Mara del experimento de HU-18). Todos los candidatos usan las mismas semillas, así
  que se comparan en igualdad de condiciones y la misma configuración da siempre el mismo JSON.
  `aptitud = presión + cobertura − 0,5 × celdas sobrecargadas por tick de IA − 4 × desvío de la
  tasa de victorias del guion fuera de [20 %, 40 %]`, con presión = 1 − t_contacto / 90 s. Los pesos
  de la fórmula se cambian en la sección `"aptitud"` del archivo.
- **Resultado**: el JSON trae las métricas de los pesos iniciales y de los mejores y la sección
  `"pesos"` lista para copiar a `config/ia.json`. Con la semilla 11, el GA tarda unos 10 s en 4
  núcleos (1122 evaluaciones) y el SA unos 20 s (2001 evaluaciones). Los dos suben la aptitud de
  0,42 a cerca de 1,39: la cobertura pasa de 34 % a 46-47 % y las celdas sobrecargadas casi
  desaparecen, pero para que el guion gane un 20-40 % de las partidas bajan la visión y la velocidad
  de los perseguidores por debajo de la de Mara caminando. El guion no esquiva a nadie, así que
  esos pesos hacen la horda demasiado fácil para una persona: por eso no se copian solos a
  `config/ia.json` (ver ADR-0015, riesgo de sobreajuste al guion).
- **Partidas guardadas**: el formato pasa a la versión 2 para guardar también los pesos. Las de
  0.12.0 se siguen leyendo, con los pesos por defecto.

### Versión 0.12.0

**Guardar, cargar y configurar** (HU-19, RF-090 a RF-093) y **tutorial de sigilo** (HU-20, RF-017).

- **Menú de pausa (Esc)**: Continuar, Guardar partida, Cargar partida, Opciones y Salir del juego.
  Se maneja con las flechas, Enter y Esc, y la simulación se detiene mientras está abierto. Esc ya
  no cierra el juego: se sale desde el menú o cerrando la ventana.
- **Guardar y cargar** (RF-090, RF-091): tres ranuras en `saves/slot1.sav` a `slot3.sav`. F5 guarda
  y F9 carga la ranura 1 sin abrir el menú. El archivo se escribe primero como `slotN.sav.tmp` y
  luego se renombra, así que un corte a mitad no estropea la partida anterior (RNF-017). Cargar o
  salir con progreso sin guardar pide confirmación (RNF-014).
- **Qué se guarda** (ADR-0016): como la simulación es determinista, la partida se guarda como su
  inicio (semilla, mapa, parámetros de IA) más las entradas de cada tick, comprimidas en tramos, y
  la huella del estado. Al cargar se repiten las entradas y se comprueba la huella, así que el
  generador aleatorio, las memorias, las rutas y los planes quedan exactamente igual. 10 minutos de
  juego ocupan unos 16 KB. Una partida solo se puede continuar con la misma versión del juego.
- **Archivos dañados** (RF-093, RNF-016, RNF-021): la cabecera `REFG`, la versión, la longitud, el
  CRC-32 y los rangos se validan antes de cargar, y los archivos de más de 10 MB se rechazan sin
  leerlos. Si algo falla, el menú dice «La partida guardada está dañada» y la partida actual sigue.
- **Opciones** (RF-092): volumen (con efectos de sonido generados en memoria al recoger, lanzar un
  señuelo, poner una barricada, guardar y avanzar en el tutorial), tutorial activado o no, y
  **Controles**: cada acción se reasigna pulsando Enter y luego la tecla nueva; si la tecla ya
  estaba en uso, las dos acciones se intercambian. Esc, Enter y las flechas quedan reservadas para
  el menú (las flechas siempre mueven a Mara). Los cambios se guardan al momento en
  `config/juego.json` y `config/controles.json`; el panel muestra siempre las teclas actuales.
- **Al iniciar** se leen `config/juego.json`, `config/controles.json` y `config/ia.json` (velocidades,
  ruidos, tiempos de la misión, señuelo, memoria, alertas, coordinación y navegación). Si falta un
  archivo se usan los valores por defecto; si uno es inválido, el juego avisa en pantalla y en la
  consola qué campo falla y usa los valores por defecto de ese campo.
- **Tutorial** (RF-017): en la primera partida nueva salen cuatro indicaciones en orden (moverse,
  sigilo, correr hace ruido y recoger un objeto), con una barra de progreso. Cada una desaparece al
  hacer la acción. «Saltar tutorial» (botón o T) lo quita. Al terminar, `logs/ia_<semilla>.log`
  anota cuánto tardó (RNF-011). No sale al reiniciar con R ni al cargar una partida.

### Versión 0.11.0

**Comparar agentes aislados y coordinados** (HU-18, RF-056, RF-084).

- **F4 en la partida** (RF-056): apaga o enciende la comunicación entre infectados. En modo aislado
  no se propagan alertas (RF-047) ni hay Contract Net (RF-049); todo lo demás (percepción, memoria,
  tácticas, reparto de llegadas, generador aleatorio) sigue igual. El panel muestra «Modo: aislado
  (F4)» en naranja o «Modo: coordinado (F4)» en amarillo, y debajo las alertas y los mensajes. La
  tecla C sigue funcionando como antes y el modo se conserva al reiniciar con R.
- **Experimento pareado** (RF-084): `refugio --coord-exp 7 --reps 10` (o `refugio_bench`, sin
  ventana) simula 10 partidas en modo aislado y 10 en coordinado. La repetición r usa la semilla
  7 + r en los dos modos, así que cada par comparte mapa, infectados y objetos. Mara sigue un guion
  fijo que no mira a los infectados: camina por A\* a cada objeto, instala la radio y va a la
  extracción. Sin `--salida` escribe `results/coord_<fecha>_<semilla>.csv`. Opciones: `--infectados`
  (10), `--segundos` (tope por partida, 120).
- **Columnas** (RF-084): `modo, semilla, repeticion, ticks, duracion_s, resultado, motivo_fin,
  t_primera_deteccion_s, t_contacto_s, cobertura, mensajes, mensajes_alerta, mensajes_contract_net,
  alertas, subastas, celdas_sobrecargadas, max_celdas_sobrecargadas, max_infectados_cerca,
  salud_final, objetos_recogidos, hash_previo_deteccion, hash_final`. Un tiempo de -1 significa que
  no ocurrió. La cobertura es la fracción de celdas transitables que pisó algún infectado; las celdas
  sobrecargadas se cuentan en cada tick de IA (más de `MAX_AGENTES_POR_CELDA` infectados en una
  celda) y se suman.
- **Comprobación de RF-056**: hasta la primera detección las dos partidas de un par tienen que ser
  la misma, porque todavía no hay nada que comunicar. El CSV guarda la huella del estado del tick
  anterior a la detección; si algún par difiere, el programa lo dice y termina con código 4.
- **Resumen por pantalla**: para cada modo, detecciones y contactos con su tiempo medio, cobertura,
  mensajes, celdas sobrecargadas, máximo de infectados a 2 celdas de Mara, duración media y
  victorias/derrotas. Con la semilla 7 y 10 repeticiones la horda coordinada rodea a Mara con 7,4
  infectados de media frente a 5,3 de la aislada y la derrota llega antes (20,6 s frente a 21,5 s).
  (Desde 0.12.0 la tecla C ya no cambia el modo: solo F4, que se puede reasignar.)

### Versión 0.10.0

**Comparar Dijkstra y A\* en vivo y benchmark reproducible** (HU-16, HU-17).

- **F2 en la partida** (RF-024, RF-074): cambia el algoritmo de búsqueda de *todos* los agentes
  (infectados y superviviente) entre A\* y Dijkstra. El panel muestra cuál está activo
  («AEstrella (F2) tick IA…» o «Dijkstra (F2)…») y las búsquedas siguientes ya usan el nuevo. Se
  mantiene al reiniciar con R. Con Dijkstra los nodos expandidos del agente seleccionado se pintan
  en cian; con A\*, en magenta, igual que en el laboratorio.
- **F3 en la partida** (RF-074): con un infectado seleccionado, repite su última consulta con los
  dos algoritmos sobre el mapa actual y muestra una tabla con costo, nodos expandidos y generados,
  tiempo y memoria de cada uno. Debajo dice si el costo coincide (tolerancia 1e-6) y cuántos nodos se
  ahorra A\*. La tabla se rehace sola cada vez que el agente planifica una ruta nueva. Cada
  comparación queda en el registro de IA (`comparar=(x,y)>(x,y) dijkstra=costo/nodos
  aestrella=costo/nodos mismo_costo=si`). Comparar no cambia la simulación.
- **Laboratorio (Tab)**: con F3 ahora también dice si los costos coinciden y el porcentaje de nodos
  que ahorra A\*.
- **Benchmark con archivo de configuración** (RF-080): `refugio --bench bench/config.json` (o
  `refugio_bench`, que no abre ventana). Sin `--salida` escribe
  `results/bench_<fecha>_<semilla>.csv`. Con `bench/config.json` salen 2 × 4 × 20 × 5 = 800 filas.
  Las opciones de la línea de comandos mandan sobre el JSON, y una clave mal escrita o un valor
  fuera de rango se rechazan con un mensaje que dice cuál.
- **Columnas** (RF-080): `algoritmo, mapa, ancho, alto, densidad_obstaculos, semilla, par,
  repeticion, origen_x, origen_y, destino_x, destino_y, encontrada, costo, longitud,
  nodos_expandidos, nodos_generados, max_abierta, tiempo_us, memoria_pico_bytes,
  replanificaciones, costo_inicial, eventos_aplicados`.
- **Metadatos** (RF-081): la cabecera incluye versión y commit, CPU, núcleos, RAM, sistema
  operativo, compilador y opciones de compilación, fecha, semilla maestra y archivo de configuración.
- **Optimalidad** (RF-082): cada consulta compara el costo de A\* con el de Dijkstra; si alguna
  difiere en más de 1e-6 el programa lo dice y termina con código 2. También cuenta las consultas
  en que A\* expandiera más nodos que Dijkstra (con heurística consistente son 0).
- **Escenario dinámico** (RF-083): con `"dinamico"` en el JSON (o `--dinamico`) un agente recorre su
  ruta, una celda por tick, y en el tick 30 aparece una barricada 4 celdas por delante. Si el
  cambio toca la ruta que le queda, replanifica desde donde está. El resultado va a
  `…_dinamico.csv`, con las replanificaciones, el costo recorrido y la suma de nodos y tiempo de
  todas sus búsquedas. Los eventos también pueden ir en una celda fija (`"x"`, `"y"`) o cambiar el
  costo del terreno (`"multiplicador"`).
- **Resumen**: `python tools/resumen_bench.py <csv>` agrupa por mapa y algoritmo, añade las
  replanificaciones en el CSV dinámico y vuelve a comprobar la optimalidad en el estático.

### Versión 0.9.0

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

Partida (escena inicial). Son las teclas por defecto; todas menos Esc, Enter y las flechas se
pueden cambiar en Esc > Opciones > Controles:

| Tecla | Acción |
|---|---|
| WASD / flechas | Mover a Mara |
| Shift / Ctrl (mantener) | Correr (más ruido) / sigilo (casi sin ruido) |
| E (mantener) | Recoger un objeto a 1 celda; en la radio y con los tres objetos, activarla (8 s) y abrir la extracción 60 s |
| Q | Lanzar un señuelo hacia el cursor (8 celdas como máximo) |
| B | Poner una barricada en la celda de al lado, hacia el cursor |
| Esc | Menú de pausa: guardar, cargar, opciones (volumen, tutorial, controles) y salir |
| F5 / F9 | Guardado rápido / carga rápida en la ranura 1 |
| T | Saltar el tutorial |
| F1 | Depuración (sin oscuridad): estado de cada infectado, conos de visión, rutas de A\*, radio del último ruido, flechas del campo de flujo, tareas y celdas de llegada |
| Clic izquierdo | Seleccionar el infectado bajo el cursor y ver su monitor; clic en otro sitio lo suelta |
| N | Mostrar u ocultar los nodos expandidos del agente seleccionado |
| L | Consola del registro de IA: estados y planes → mensajes → todo → oculta |
| F4 | Modo aislado / coordinado de la horda (alertas y Contract Net entre infectados) |
| F2 | Alternar Dijkstra / A\* para todos los agentes |
| F3 | Con un infectado seleccionado: tabla Dijkstra frente a A\* de su última consulta (otra vez F3 la oculta) |
| P | Pausar o reanudar la simulación (el mapa, el monitor, F3 y la consola siguen funcionando) |
| F6 | En pausa: avanzar exactamente un tick de simulación (1/60 s) |
| F7 / F8 | Simulación más lenta / más rápida: 0,25×, 0,5×, 1×, 2× y 4× |
| Z | Cámara: seguir a Mara con zoom o ver el mapa completo |
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
# Dijkstra vs A* con bench/config.json: 4 tamaños x 20 pares x 5 repeticiones, semilla 42 (800 filas)
# y escenario dinámico con una barricada en el tick 30 -> results/bench_<fecha>_42.csv y _dinamico.csv
./build/headless/refugio_bench --bench bench/config.json
python3 tools/resumen_bench.py results/bench_<fecha>_42.csv

# Lo mismo sin archivo, con opciones (mandan sobre el JSON si se dan los dos)
./build/headless/refugio_bench --bench --tamanos 32,64,128,256 --pares 20 --reps 5 --semilla 42 --salida results/bench.csv

# Horda aislada frente a coordinada: 10 partidas por modo con las semillas 7..16 (20 filas)
./build/headless/refugio_bench --coord-exp 7 --reps 10

# Optimizar los pesos de la horda: algoritmo genético (30 x 40) y recocido simulado (2000 iteraciones),
# semilla 11 -> results/opt_ga_11.json y results/opt_sa_11.json
./build/headless/refugio_bench --optimize ga --config opt/ga.json
./build/headless/refugio_bench --optimize sa --config opt/sa.json

# Estrés de las funciones recursivas (profundidad, llamadas, mensajes, pila)
./build/headless/refugio_bench --estres-recursion --agentes 50,500,2000 --profundidades 1,3,10 --salida results/estres.csv
```

El benchmark termina con código 2 si A* y Dijkstra difieren en costo (RF-082); el estrés termina con
código 3 si alguna recursión supera su límite, si un agente recibe la misma alerta dos veces o si la pila
supera 64 KB (RNF-027). El experimento de coordinación termina con código 4 si un par aislado/coordinado
difiere antes de la primera detección (RF-056). La optimización termina con código 5 si la aptitud del
mejor individuo del GA baja entre generaciones (RF-085).

## Integración continua

| Flujo | Cuándo | Qué hace |
|---|---|---|
| `CI` | push a `main` y cada PR | Compila juego y pruebas en Linux (GCC, Clang) y Windows (MSVC); ASan + UBSan; comprueba que el dominio no dependa de raylib; humo de benchmark, estrés, aislados/coordinados y optimización |
| `Rendimiento y estrés` | cada noche y manual (Actions → Run workflow) | Benchmark completo y estrés de recursión en Linux y Windows; tabla en el resumen del run y CSV como artefacto |

## Estructura

```text
src/dominio/       Lógica pura (sin raylib): núcleo, mundo, navegación, agentes (FSM, percepción),
                   juego (partida, tutorial), coordinación, superviviente (Utility, GOAP, árbol),
                   experimentos, config (JSON de opciones, controles e IA), persistencia (partidas), puertos
src/adaptadores/   raylib (arte pixel y luz, vistas y sonidos), vista (lectura visual del mapa, sin raylib) y archivo (registro de IA en logs/, partidas en saves/,
                   configuración en config/); luego Tiled y raygui
src/app/           main del juego, menú de pausa, main del benchmark y CLI
tests/             Pruebas Catch2 (los nombres empiezan por el ID del caso de prueba, p. ej. CP-047)
config/            Parámetros de IA (ia.json); el juego crea juego.json y controles.json
bench/             Configuración del benchmark (bench/config.json)
opt/               Configuración de la optimización (opt/ga.json, opt/sa.json)
docs/              Requisitos (ISO 29148), RNF (ISO 25010), trazabilidad, arc42 + C4 y ADR
tools/             Scripts de análisis y comprobaciones de CI
```

## Documentación

Empieza por [docs/README.md](docs/README.md).
