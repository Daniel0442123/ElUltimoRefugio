# ADR-0019 · Mapas generados por código con semilla, mapas de Tiled opcionales y lector JSON propio

| Campo | Valor |
|---|---|
| Estado | Aceptada (registra lo implementado desde 0.1.0; actualizada en 0.18.0 con el cargador de Tiled) |
| Fecha | 2026-11-25 (revisada el 2026-11-26) |
| Decisores | Equipo de desarrollo |
| Sustituye a | [ADR-0013](ADR-0013-mapas-tiled-json.md) |
| Requisitos relacionados | RF-001, RF-006, RF-031, RF-048, RF-080, RF-086, RF-091, RES-03, IS-01, RNF-016 |

## Contexto

ADR-0013 preveía mapas diseñados en Tiled y leídos con nlohmann/json. El benchmark necesitaba desde el principio
mapas de 32² a 256² generados con semilla (RF-080), y el juego necesitaba un mapa con calles, edificios y
escombros que cambie con la semilla para probar a los agentes en situaciones distintas. Hasta la 0.17.0 los
únicos JSON que leía el programa eran la configuración y los experimentos, y RF-001 quedaba pendiente.

En 0.18.0 se pidió cerrar RF-001: poder jugar en un mapa diseñado a mano sin perder la ciudad generada, que
usan el juego, el benchmark, el experimento de coordinación y la optimización.

## Decisión

* La ciudad del juego la sigue generando `juego::crearEscenarioAleatorio` (32 × 22 celdas, 10 infectados,
  objetos, radio, almacén, extracción y, desde 0.18.0, dos medicamentos de más, hasta dos puertas metálicas y
  un explorador de cada tres infectados) y los mapas del benchmark `experimentos::GeneradorMapas`, ambos con
  PCG32 y semilla. Es la opción por defecto. Los escombros dan el multiplicador de terreno m ∈ [1, 10] (RF-031).
* **RF-001 (0.18.0)**: `juego::leerMapaTiled` lee un mapa de Tiled 1.10+ en JSON y construye el **mismo
  `Escenario`** que el generador, así que la partida, la IA y la vista no cambian. Capas de IS-01 por nombre:
  `suelo` (decorativa), `colision` (patrón ≠ 0 es muro), `costo` (m = gid − firstgid + 1), y las de objetos
  `objetos`, `spawns` y `zonas`. Solo se admiten capas de patrones en CSV (lo que Tiled escribe por defecto en
  JSON) y mapas finitos y ortogonales; base64, zlib o mapas infinitos se rechazan con un mensaje que dice qué
  cambiar en Tiled. El juego lo usa con `--mapa-tiled <archivo.json>`.
* El lector está en el dominio porque solo analiza texto (como `config::leerParametrosIa`); leer el archivo es
  cosa del adaptador (`adaptadores::leerArchivo`). Un mapa inválido nunca lanza: devuelve el error con la capa,
  el objeto y su id, y el juego se abre en la ciudad generada con un aviso (RNF-016).
* Una partida en un mapa de Tiled guarda el texto del mapa dentro del archivo (formato de partida 3, hasta
  4 MB), para que cargarla no dependa de que el `.json` siga en su sitio (ADR-0016).
* Los JSON se leen con un lector propio (`dominio/nucleo/Json`, ≈ 260 líneas) que devuelve errores con la
  posición y el campo, en lugar de nlohmann/json.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Tiled + nlohmann/json (ADR-0013) | Diseño visual de niveles | Dependencia que el dominio no puede incluir (RNF-023) |
| Solo generación con semilla (hasta 0.17.0) | Infinitos mapas reproducibles; el mismo código sirve al juego, al benchmark y a los experimentos | Los niveles no se diseñan a mano; RF-001 sin cumplir |
| Generación por defecto + Tiled opcional con el lector propio (0.18.0) | Las dos cosas; sin dependencias nuevas; el resto del juego no cambia | Solo CSV y mapas finitos; el mapa de Tiled no se usa en el benchmark |
| Leer el mapa en el adaptador de archivo | Separa E/S y análisis | El análisis no hace E/S y así se prueba sin archivos, como la configuración |

## Consecuencias

**Positivas**: un mapa distinto por semilla y, además, mapas diseñados a mano (`maps/puerto.json`, que se
genera desde un dibujo en texto con `tools/crear_mapa_tiled.py` y se edita en Tiled); el lector JSON propio
cabe en el dominio y pasa el corpus de CP-116.

**Negativas**: las cinco escenas del storyboard siguen siendo momentos de una misma partida (ver StRS §4.4),
aunque cada una se puede montar como un mapa de Tiled. Las partidas de 0.13.0 a 0.17.0 se leen, pero la
ciudad generada cambió, así que al cargarlas se avisa de que son de otra versión.

## Verificación

CP-001 (capas, mapa de ejemplo, errores y guardado con el mapa), CP-031 (costos), CP-080 (benchmark con mapas
generados), CP-116 (corpus de JSON inválidos).
