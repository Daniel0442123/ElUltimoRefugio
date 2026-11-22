#!/usr/bin/env python3
"""Resume un CSV de refugio_bench --bench en una tabla Markdown (para el informe y el resumen de CI)."""
import csv
import statistics as st
import sys
from collections import defaultdict


def main(ruta: str) -> None:
    with open(ruta, encoding="utf-8") as f:
        meta = []
        filas = []
        lineas = []
        for linea in f:
            (meta if linea.startswith("#") else lineas).append(linea)
        filas = list(csv.DictReader(lineas))
    grupos = defaultdict(list)
    for r in filas:
        grupos[(int(r["ancho"]), r["algoritmo"])].append(r)

    print("### Dijkstra vs A*\n")
    for m in meta:
        print(f"- `{m[1:].strip()}`")
    print("\n| Mapa | Algoritmo | n | Nodos expandidos (media) | Tiempo µs (mediana) | Tiempo µs (p95) | Memoria KB (media) |")
    print("|---|---|---|---|---|---|---|")
    for (tam, alg), v in sorted(grupos.items()):
        tiempos = sorted(float(x["tiempo_us"]) for x in v)
        p95 = tiempos[min(len(tiempos) - 1, int(0.95 * len(tiempos)))]
        print(f"| {tam}×{tam} | {alg} | {len(v)} | {st.mean(int(x['nodos_expandidos']) for x in v):.0f} | "
              f"{st.median(tiempos):.1f} | {p95:.1f} | {st.mean(int(x['memoria_pico_bytes']) for x in v) / 1024:.1f} |")


if __name__ == "__main__":
    main(sys.argv[1])
