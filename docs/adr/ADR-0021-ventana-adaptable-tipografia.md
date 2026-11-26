# ADR-0021 · Ventana adaptable con interfaz escalada y tipografía Inter incrustada

| Campo | Valor |
|---|---|
| Estado | Aceptada (0.17.0) |
| Fecha | 2026-11-26 |
| Decisores | Equipo de desarrollo |
| Modifica a | [ADR-0017](ADR-0017-arte-pixel-iluminacion.md) (cómo se coloca el mapa en la ventana) |
| Requisitos relacionados | RF-016, RF-070, RNF-012 |

## Contexto

Hasta 0.16.0 la ventana medía siempre 1280 × 720 píxeles y todo se dibujaba en coordenadas fijas.
En un portátil de 16" con 1920 × 1080 el juego ocupaba un tercio de la pantalla, no se podía
maximizar y los textos usaban la fuente por defecto de raylib: un mapa de bits de 10 px ampliado a
11–20 px, pixelado y difícil de leer, sin tildes ni ñ.

## Decisión

1. **Ventana redimensionable** (`FLAG_WINDOW_RESIZABLE`), abierta maximizada, con tamaño mínimo
   800 × 450 y F11 para pantalla completa sin bordes. F11 queda reservada: no se puede asignar a
   ninguna acción (`config::tecla::kPantallaCompleta`).
2. **Interfaz en unidades**: `vista::calcularDisposicion` (sin raylib, con pruebas) calcula cada
   frame la escala `min(ancho/1280, alto/720)` y reparte la ventana: panel de 256 unidades a la
   derecha, franja de estado de 16 abajo y el resto para el mapa. El HUD se dibuja bajo una
   `Camera2D` con zoom = escala, así el diseño de los paneles no cambia. No se usa
   `FLAG_WINDOW_HIGHDPI`: la escala sale del tamaño real en píxeles, que ya refleja la densidad
   de la pantalla (en Windows GLFW declara el proceso consciente de DPI).
3. **Mapa en píxeles**: la cámara del mundo usa el zoom que hace caber el mapa entero en su zona
   (× 2 al seguir a Mara) y lo centra si sobra espacio. Se recorta a la parte visible del mapa.
4. **Tipografía Inter** (SIL OFL 1.1), Medium para el texto y Bold para los títulos, reducida a
   latín básico + Latin-1 + algunos signos (≈ 17 KB cada una) e incrustada en el ejecutable con
   `cmake/Embeber.cmake`. Cada tamaño se rasteriza al tamaño final en píxeles (tamaño × escala, o
   × zoom para los rótulos del mapa) y se guarda en una caché de hasta 48 tamaños.

## Alternativas consideradas

- **Dibujar a 1280 × 720 en una textura y ampliarla**: simple, pero el texto queda borroso y
  sobran franjas negras en pantallas que no son 16:9.
- **Fuente SDF**: nítida a cualquier escala con una sola textura, pero requiere un shader y raylib
  la dibuja peor a tamaños pequeños que una rasterización directa.
- **Cargar la fuente desde `assets/` en tiempo de ejecución**: falla si el juego se lanza desde
  otra carpeta; incrustarla evita esa dependencia.

## Consecuencias

- El juego se ve bien de 800 × 450 a 4K y en pantallas panorámicas o 16:10.
- Los textos con tildes y ñ ya se muestran; los rótulos del HUD se corrigieron («El último
  refugio», «Batería», «Extracción»…).
- La simulación no cambia: solo el dibujo y la conversión del ratón a coordenadas del mundo.
- Las fuentes se rasterizan de nuevo al cambiar el tamaño de la ventana (unos milisegundos).
