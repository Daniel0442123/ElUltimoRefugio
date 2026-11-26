#!/usr/bin/env python3
"""Genera las figuras y estadísticas del informe final (docs/informe_final.md).

Uso: python tools/graficas_informe.py [carpeta_datos] [carpeta_figuras]
Lee bench_final.csv, bench_final_dinamico.csv, coord_final.csv, opt_ga_final.json y
opt_sa_final.json (salidas de refugio_bench) y escribe PNG. Requiere matplotlib y scipy.
"""
import csv
import json
import statistics as st
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from scipy import stats

DATOS = Path(sys.argv[1] if len(sys.argv) > 1 else "docs/informe/datos")
FIG = Path(sys.argv[2] if len(sys.argv) > 2 else "docs/informe/figuras")
FIG.mkdir(parents=True, exist_ok=True)
COL = {"Dijkstra": "#2a9d8f", "AEstrella": "#e76f51", "aislado": "#8d99ae", "coordinado": "#e9c46a"}
plt.rcParams.update({"figure.dpi": 150, "font.size": 9, "axes.spines.top": False, "axes.spines.right": False})


def leer_csv(nombre):
    with open(DATOS / nombre, encoding="utf-8") as f:
        return list(csv.DictReader(l for l in f if not l.startswith("#")))


def p95(v):
    v = sorted(v)
    return v[min(len(v) - 1, int(0.95 * len(v)))]


# 1. Benchmark estático: nodos y tiempo frente al tamaño
filas = leer_csv("bench_final.csv")
g = defaultdict(list)
for r in filas:
    g[(r["algoritmo"], int(r["ancho"]))].append(r)
tam = sorted({k[1] for k in g})
fig, ax = plt.subplots(1, 2, figsize=(8, 3.2))
for alg, etq in (("Dijkstra", "Dijkstra"), ("AEstrella", "A*")):
    nodos = [st.mean(float(x["nodos_expandidos"]) for x in g[(alg, t)]) for t in tam]
    med = [st.median(float(x["tiempo_us"]) for x in g[(alg, t)]) for t in tam]
    q95 = [p95([float(x["tiempo_us"]) for x in g[(alg, t)]]) for t in tam]
    ax[0].plot(tam, nodos, "o-", color=COL[alg], label=etq)
    ax[1].plot(tam, med, "o-", color=COL[alg], label=f"{etq} mediana")
    ax[1].plot(tam, q95, "--", color=COL[alg], alpha=0.6, label=f"{etq} p95")
for a, t in zip(ax, ("Nodos expandidos (media)", "Tiempo de búsqueda (µs)")):
    a.set_xscale("log", base=2)
    a.set_yscale("log")
    a.set_xticks(tam, [f"{t}²" for t in tam])
    a.set_xlabel("Tamaño del mapa (celdas)")
    a.set_title(t)
    a.grid(alpha=0.3)
    a.legend(frameon=False, fontsize=7)
fig.tight_layout()
fig.savefig(FIG / "bench_tamano.png")

# Pendiente log-log del tiempo mediano frente a V (crecimiento empírico)
import math
print("Pendientes log-log tiempo mediano vs celdas:")
for alg in ("Dijkstra", "AEstrella"):
    xs = [math.log(t * t) for t in tam]
    ys = [math.log(st.median(float(x["tiempo_us"]) for x in g[(alg, t)])) for t in tam]
    print(f"  {alg}: {stats.linregress(xs, ys).slope:.2f}")

# Razón de nodos por consulta
print("Ahorro de nodos de A* (media por consulta):")
pares = defaultdict(dict)
for r in filas:
    pares[(r["ancho"], r["par"], r["repeticion"])][r["algoritmo"]] = r
for t in tam:
    rs = [1 - float(p["AEstrella"]["nodos_expandidos"]) / float(p["Dijkstra"]["nodos_expandidos"])
          for k, p in pares.items() if int(k[0]) == t]
    print(f"  {t}²: {100 * st.mean(rs):.1f} %")

# 2. Experimento aislado / coordinado
coord = leer_csv("coord_final.csv")
por = defaultdict(dict)
for r in coord:
    por[int(r["repeticion"])][r["modo"]] = r
reps = sorted(por)
fig, ax = plt.subplots(1, 2, figsize=(8, 3.2))
w = 0.38
for i, modo in enumerate(("aislado", "coordinado")):
    ax[0].bar([r + (i - 0.5) * w for r in reps], [int(por[r][modo]["max_infectados_cerca"]) for r in reps],
              w, color=COL[modo], label=modo)
ax[0].set_title("Máx. infectados a ≤ 2 celdas de Mara")
ax[0].set_xlabel("Repetición (semilla 7 + r)")
ax[0].set_xticks(reps)
ax[0].legend(frameon=False)
met = [("max_infectados_cerca", "Rodean\na Mara"), ("celdas_sobrecargadas", "Celdas\nsobrecargadas"),
       ("duracion_s", "Duración (s)"), ("cobertura", "Cobertura (%)")]
for i, modo in enumerate(("aislado", "coordinado")):
    vals = []
    for c, _ in met:
        v = st.mean(float(por[r][modo][c]) for r in reps)
        vals.append(v * 100 if c == "cobertura" else v)
    ax[1].bar([j + (i - 0.5) * w for j in range(len(met))], vals, w, color=COL[modo], label=modo)
ax[1].set_xticks(range(len(met)), [m[1] for m in met], fontsize=7)
ax[1].set_title("Medias por modo (10 pares)")
fig.tight_layout()
fig.savefig(FIG / "coordinacion.png")

print("Pruebas pareadas (coordinado - aislado), Wilcoxon de rangos con signo:")
for c, n in met + [("t_contacto_s", "Contacto (s)")]:
    a = [float(por[r]["aislado"][c]) for r in reps]
    b = [float(por[r]["coordinado"][c]) for r in reps]
    d = [y - x for x, y in zip(a, b)]
    try:
        p = stats.wilcoxon(b, a).pvalue
    except ValueError:
        p = float("nan")
    print(f"  {n}: aislado {st.mean(a):.3f} coordinado {st.mean(b):.3f} dif media {st.mean(d):+.3f} p={p:.3f}")

# 3. GA y SA
ga = json.load(open(DATOS / "opt_ga_final.json", encoding="utf-8"))
sa = json.load(open(DATOS / "opt_sa_final.json", encoding="utf-8"))
fig, ax = plt.subplots(1, 2, figsize=(8, 3.2))
gen = ga["generaciones"]
x = [e["generacion"] for e in gen]
ax[0].plot(x, [e["mejor"] for e in gen], color="#e76f51", label="mejor")
ax[0].plot(x, [e["media"] for e in gen], color="#2a9d8f", label="media")
ax[0].plot(x, [e["peor"] for e in gen], color="#8d99ae", label="peor")
ax[0].axhline(ga["base"]["metricas"]["aptitud"], ls=":", color="k", lw=0.8, label="pesos actuales")
ax[0].set_title("Algoritmo genético (30 × 40)")
ax[0].set_xlabel("Generación")
ax[0].set_ylabel("Aptitud")
ax[0].legend(frameon=False, fontsize=7)
it = sa["iteraciones"]
x = [e["iteracion"] for e in it]
ax[1].plot(x, [e["actual"] for e in it], color="#2a9d8f", lw=0.7, label="actual")
ax[1].plot(x, [e["mejor"] for e in it], color="#e76f51", label="mejor")
ax[1].axhline(sa["base"]["metricas"]["aptitud"], ls=":", color="k", lw=0.8, label="pesos actuales")
ax2 = ax[1].twinx()
ax2.plot(x, [e["temperatura"] for e in it], color="#8d99ae", ls="--", lw=0.8)
ax2.set_ylabel("Temperatura", color="#8d99ae")
ax[1].set_title("Recocido simulado (2000 iteraciones)")
ax[1].set_xlabel("Iteración")
ax[1].legend(frameon=False, fontsize=7, loc="lower right")
fig.tight_layout()
fig.savefig(FIG / "optimizacion.png")
print("Figuras en", FIG)
