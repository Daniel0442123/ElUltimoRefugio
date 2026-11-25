# ADR-0017 · Arte pixel generado por código con luz 2D y sombras

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-25 |
| Decisores | Pedro (equipo del proyecto) |
| Requisitos relacionados | RF-016, RF-071, RNF-006, RNF-012, RNF-023, ADR-0003, ADR-0005, ADR-0012 |

## Contexto

Hasta la 0.15.0 el mundo se dibujaba con primitivas planas: una rejilla gris con cuadros y círculos
de colores. Funcionaba para depurar la IA, pero el juego no parecía de zombis. Se compararon tres
maquetas sobre el mismo mapa: (A) pixel art cenital con linterna, (B) estilo vectorial oscuro con
luces de color y (C) un pack de arte CC0 externo con la misma iluminación. Se eligió la A.

## Decisión

- **Arte original generado por código** (`adaptadores/raylib/ArteMundo`): los sprites de 16 x 16 px
  (infectados en 4 variantes y 2 fotogramas, Mara, superviviente, objetos, radio, almacén, barricada,
  bengala, sangre) están escritos como rejillas de caracteres en el código y se convierten en un atlas
  al abrir la ventana. No hay archivos de imagen ni licencias de terceros.
- **Lectura visual del mapa sin raylib** (`adaptadores/vista/Escenario`): edificios (celdas bloqueadas
  que no son barricadas, agrupadas en componentes 4-conexos), aceras junto a ellos, calle y escombros
  (multiplicador > 1). La decoración sale de `vista::hashCelda(semilla, x, y)`, nunca del PCG32 de la
  simulación, así que dibujar no cambia la partida (ADR-0012).
- **Fondo en caché**: el suelo y los edificios se pintan una vez por mapa en una textura; se rehacen
  solo si cambia la firma de la rejilla visible (poner o romper barricadas no la cambia).
- **Luz 2D con sombras**: cada luz es un abanico de rayos que se detienen en los muros
  (`vista::distanciaRayo`, DDA sobre la rejilla). Se suman en una textura a media resolución que
  multiplica la escena. Luces: linterna de Mara (cono, más corto en sigilo), su halo, la radio, el
  superviviente, la extracción abierta, las bengalas y las ventanas encendidas.
- **La información de la IA va encima del arte**: nodos expandidos, rutas, conos, flow field, alertas,
  selección y monitor se dibujan después de la luz. F1 quita la oscuridad y añade un anillo con el
  color del estado de cada infectado; sin F1 se ve «?» (oyó o busca) o «!» (la ve o ataca).
- **Cámara** (tecla Z, reasignable): sigue a Mara con zoom 2× o enseña el mapa completo. El ratón se
  convierte a coordenadas del mapa con la cámara, así que el clic del monitor y el señuelo siguen
  apuntando a la celda correcta.

## Alternativas consideradas

- **B, vectorial oscuro**: menos esfuerzo y sin atlas, pero menos reconocible como juego de zombis.
- **C, pack CC0**: arte más trabajado, pero obliga a descargar y versionar imágenes y a mantener
  créditos; el estilo de los packs disponibles es más claro que el buscado.
- **Sombras con shader**: más suaves, pero añaden GLSL a mantener en tres plataformas; los rayos
  en CPU cuestan menos de 1 ms con el número de luces del prototipo.

## Consecuencias

- El juego tiene identidad visual sin tocar el dominio: la IA, el determinismo y las partidas
  guardadas no cambian.
- Las capturas para la documentación se pueden regenerar con
  `refugio --captura archivo.png [--frames N] [--mapa] [--depuracion]`.
- Cambiar el arte es editar las rejillas de `ArteMundo.cpp`.

## Verificación

- `tests/unit/test_escenario.cpp`: clasificación de celdas, edificios, barricadas que no son muros,
  rayos de luz contra muros y alcance, y decoración determinista.
- Revisión visual con `--captura` en las vistas con zoom, mapa completo y depuración.
