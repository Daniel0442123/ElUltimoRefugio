#!/usr/bin/env python3
"""Crea el mapa de ejemplo de Tiled (RF-001) a partir de un dibujo en texto.

    python tools/crear_mapa_tiled.py            # escribe maps/puerto.json y maps/patrones_*.png

El resultado es un mapa de Tiled 1.10 normal: se abre y se edita con Tiled (las imágenes de los
conjuntos de patrones están al lado) y el juego lo carga con --mapa-tiled maps/puerto.json. Este
script solo sirve para tener el mapa en un dibujo legible; después se puede editar en Tiled.

Leyenda del dibujo:
    #  muro                    .  calle               :  escombros (m = 4)
    D  puerta metálica         M  Mara                S  superviviente
    R  radio (y refugio)       A  almacén de tablas   X  zona de extracción
    m  medicamento             c  combustible         b  batería
    e  infectado explorador    p  infectado perseguidor
Solo usa la biblioteca estándar y, para las imágenes de los patrones, Pillow si está instalado.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
SALIDA = RAIZ / "maps"
T = 32  # píxeles por celda en Tiled

DIBUJO = """
################################
#M.....#........#.......:::....#
#......#..####..#..###..:::.c..#
#..##..#..#m.#..#..#b#.........#
#..##..D..#..#..D..#.#..####...#
#......#..##.#..#..#.#..#..#...#
###.####.......####.....#.p#...#
#........:::.......e....##D#...#
#........:::..................##
#......####...######..###......#
####D###..#...#....#..#A#..p...#
#.........#...#.S..#..#.#......#
#..::.....D...#.R..D..#.#..#####
#..::..e..#...#....#....#..#...#
#.........#...###.##.......#.m.#
#####.#####..............p.D...#
#.........#..:::...#####...#...#
#..e...m..#..:::...#...#...#####
#.........D........#.X.........#
#...###...#..####..#...#..p....#
#...###...#........##.##.......#
################################
"""

COSTO_ESCOMBROS = 4
OBJETOS = {"m": "medicamento", "c": "combustible", "b": "bateria", "R": "radio", "A": "almacen", "D": "puerta"}
SPAWNS = {"M": ("mara", None), "S": ("superviviente", None), "e": ("infectado", "explorador"), "p": ("infectado", "perseguidor")}


def comprobar(filas: list[str]) -> None:
    alto, ancho = len(filas), len(filas[0])
    assert all(len(f) == ancho for f in filas), "todas las filas deben tener el mismo ancho"
    muro = lambda x, y: not (0 <= x < ancho and 0 <= y < alto) or filas[y][x] in "#D"
    for y, f in enumerate(filas):
        for x, c in enumerate(f):
            if c == "D":
                horizontal = not muro(x - 1, y) and not muro(x + 1, y) and muro(x, y - 1) and muro(x, y + 1)
                vertical = not muro(x, y - 1) and not muro(x, y + 1) and muro(x - 1, y) and muro(x + 1, y)
                assert horizontal or vertical, f"la puerta ({x}, {y}) no está en un hueco de muro"
    # Con las puertas abiertas todo lo importante es alcanzable desde Mara (8 vecinos sin cortar esquinas).
    inicio = next((x, y) for y, f in enumerate(filas) for x, c in enumerate(f) if c == "M")
    libre = lambda x, y: 0 <= x < ancho and 0 <= y < alto and filas[y][x] != "#"
    visto, pila = {inicio}, [inicio]
    while pila:
        x, y = pila.pop()
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                n = (x + dx, y + dy)
                if n in visto or not libre(*n) or (dx and dy and not (libre(x + dx, y) and libre(x, y + dy))):
                    continue
                visto.add(n)
                pila.append(n)
    for y, f in enumerate(filas):
        for x, c in enumerate(f):
            if c in "mcbRAXS":
                assert (x, y) in visto, f"«{c}» en ({x}, {y}) no se alcanza desde Mara"


def imagenes() -> None:
    try:
        from PIL import Image, ImageDraw
    except ImportError:
        print("Pillow no está instalado: no se rehacen las imágenes de los patrones", file=sys.stderr)
        return
    base = Image.new("RGBA", (2 * T, T))
    d = ImageDraw.Draw(base)
    d.rectangle([0, 0, T - 1, T - 1], fill=(58, 60, 66, 255))
    d.rectangle([T, 0, 2 * T - 1, T - 1], fill=(120, 92, 70, 255), outline=(70, 52, 40, 255), width=3)
    base.save(SALIDA / "patrones_ciudad.png")
    costos = Image.new("RGBA", (10 * T, T))
    d = ImageDraw.Draw(costos)
    for k in range(10):
        g = 230 - 18 * k
        d.rectangle([k * T, 0, (k + 1) * T - 1, T - 1], fill=(g, 200 - 14 * k, 80, 140))
        d.text((k * T + 10, 9), str(k + 1), fill=(20, 20, 20, 255))
    costos.save(SALIDA / "patrones_costo.png")


def capa(nombre: str, datos: list[int], ancho: int, alto: int, ident: int, visible: bool = True) -> dict:
    return {"id": ident, "name": nombre, "type": "tilelayer", "width": ancho, "height": alto, "x": 0, "y": 0,
            "opacity": 1 if visible else 0.5, "visible": visible, "data": datos}


def main() -> int:
    filas = [f for f in DIBUJO.strip("\n").splitlines()]
    comprobar(filas)
    alto, ancho = len(filas), len(filas[0])
    gid_calle, gid_muro, primer_costo = 1, 2, 3
    suelo, colision, costo = [], [], []
    objetos, spawns, zonas = [], [], []
    ident = 1
    for y, f in enumerate(filas):
        for x, c in enumerate(f):
            # Una puerta va en la capa «objetos»; en «colision» queda libre: el juego la cierra (RF-006).
            suelo.append(gid_muro if c == "#" else gid_calle)
            colision.append(gid_muro if c == "#" else 0)
            costo.append(primer_costo + COSTO_ESCOMBROS - 1 if c == ":" else 0)
            punto = {"x": x * T + T / 2, "y": y * T + T / 2, "width": 0, "height": 0, "point": True, "rotation": 0,
                     "visible": True}
            if c in OBJETOS:
                objetos.append({"id": ident, "name": "", "type": OBJETOS[c], **punto})
            elif c in SPAWNS:
                tipo, rol = SPAWNS[c]
                o = {"id": ident, "name": "", "type": tipo, **punto}
                if rol:
                    o["properties"] = [{"name": "rol", "type": "string", "value": rol}]
                spawns.append(o)
            elif c == "X":
                zonas.append({"id": ident, "name": "", "type": "extraccion", "x": (x - 1) * T, "y": (y - 1) * T,
                              "width": 3 * T, "height": 3 * T, "rotation": 0, "visible": True})
            else:
                continue
            ident += 1
    mapa = {
        "type": "map", "version": "1.10", "tiledversion": "1.10.2", "orientation": "orthogonal",
        "renderorder": "right-down", "infinite": False, "width": ancho, "height": alto, "tilewidth": T,
        "tileheight": T, "nextlayerid": 7, "nextobjectid": ident, "compressionlevel": -1,
        "properties": [{"name": "descripcion", "type": "string",
                        "value": "Puerto: mapa de ejemplo de El último refugio (RF-001). Ver docs/03 IS-01."}],
        "tilesets": [
            {"firstgid": 1, "name": "ciudad", "image": "patrones_ciudad.png", "imagewidth": 2 * T, "imageheight": T,
             "tilewidth": T, "tileheight": T, "tilecount": 2, "columns": 2, "margin": 0, "spacing": 0},
            {"firstgid": 3, "name": "costo", "image": "patrones_costo.png", "imagewidth": 10 * T, "imageheight": T,
             "tilewidth": T, "tileheight": T, "tilecount": 10, "columns": 10, "margin": 0, "spacing": 0},
        ],
        "layers": [
            capa("suelo", suelo, ancho, alto, 1),
            capa("colision", colision, ancho, alto, 2),
            capa("costo", costo, ancho, alto, 3, visible=False),
            {"id": 4, "name": "objetos", "type": "objectgroup", "draworder": "topdown", "opacity": 1, "visible": True,
             "x": 0, "y": 0, "objects": objetos},
            {"id": 5, "name": "spawns", "type": "objectgroup", "draworder": "topdown", "opacity": 1, "visible": True,
             "x": 0, "y": 0, "objects": spawns},
            {"id": 6, "name": "zonas", "type": "objectgroup", "draworder": "topdown", "opacity": 1, "visible": True,
             "x": 0, "y": 0, "objects": zonas},
        ],
    }
    SALIDA.mkdir(exist_ok=True)
    (SALIDA / "puerto.json").write_text(json.dumps(mapa, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    imagenes()
    print(f"maps/puerto.json: {ancho} x {alto}, {len(objetos)} objetos, {len(spawns)} apariciones, {len(zonas)} zonas")
    return 0


if __name__ == "__main__":
    sys.exit(main())
