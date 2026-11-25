#!/usr/bin/env python3
"""Resume un CSV de refugio --bench en tablas Markdown (para el informe y el resumen de CI).

Sirve para el CSV estático y para el dinámico (_dinamico.csv, RF-083): agrupa por mapa y
algoritmo y, si hay replanificaciones, añade sus columnas. También comprueba la optimalidad
(RF-082): para cada consulta, el costo de A* debe coincidir con el de Dijkstra.
"""
import csv
import statistics as st
import sys
from collections import defaultdict


def p95(valores):
    v = sorted(valores)
    return v[min(len(v) - 1, int(0.95 * len(v)))]


def main(ruta: str) -> int:
    with open(ruta, encoding="utf-8") as f:
        meta, lineas = [], []
        for linea in f:
            (meta if linea.startswith("#") else lineas).append(linea)
        filas = list(csv.DictReader(lineas))
    dinamico = any(int(r.get("replanificaciones", "0") or 0) > 0 for r in filas)
    grupos = defaultdict(list)
    for r in filas:
        grupos[(int(r["ancho"]), r.get("mapa", r["ancho"]), r["algoritmo"])].append(r)

    print("### Dijkstra vs A*" + (" (escenario dinámico)" if dinamico else "") + "\n")
    for m in meta:
        print(f"- `{m[1:].strip()}`")
    cab = "| Mapa | Algoritmo | n | Costo (media) | Nodos expandidos (media) | Tiempo µs (mediana) | Tiempo µs (p95) | Memoria KB (media) |"
    sep = "|---|---|---|---|---|---|---|---|"
    if dinamico:
        cab += " Replanificaciones (media) | Sin llegar |"
        sep += "---|---|"
    print("\n" + cab + "\n" + sep)
    for (_, mapa, alg), v in sorted(grupos.items()):
        tiempos = [float(x["tiempo_us"]) for x in v]
        fila = (f"| {mapa} | {alg} | {len(v)} | {st.mean(float(x['costo']) for x in v):.2f} | "
                f"{st.mean(int(x['nodos_expandidos']) for x in v):.0f} | {st.median(tiempos):.1f} | {p95(tiempos):.1f} | "
                f"{st.mean(int(x['memoria_pico_bytes']) for x in v) / 1024:.1f} |")
        if dinamico:
            fila += (f" {st.mean(int(x['replanificaciones']) for x in v):.2f} |"
                     f" {sum(1 for x in v if x['encontrada'] == '0')} |")
        print(fila)

    if not dinamico:
        # RF-082: misma consulta (mapa, par, repetición) con el mismo costo en ambos algoritmos.
        costos = defaultdict(dict)
        for r in filas:
            costos[(r.get("mapa", r["ancho"]), r["par"], r["repeticion"])][r["algoritmo"]] = float(r["costo"])
        malas = sum(1 for c in costos.values() if len(c) == 2 and abs(max(c.values()) - min(c.values())) > 1e-6)
        print(f"\nOptimalidad (RF-082): {len(costos)} consultas, {malas} con costo distinto entre A* y Dijkstra.")
        return 1 if malas else 0
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
