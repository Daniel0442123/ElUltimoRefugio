#!/usr/bin/env python3
"""Mide FPS, tiempo de tick, CPU, memoria y arranque del juego con ventana y actualiza la documentación.

RNF-001 (FPS), RNF-002 (tick), RNF-004 (CPU), RNF-005 (memoria) y RNF-008 (arranque); casos CP-101,
CP-102, CP-104, CP-105 y CP-108.

Uso (desde la raíz del repositorio, con el juego compilado en Release):
    python tools/rendimiento.py medir              # 5 arranques + 2 partidas de 30 s + 5 min (≈ 12 min)
    python tools/rendimiento.py medir --rapido     # 5 arranques + 2 partidas de 10 s + 60 s (≈ 3 min)
    python tools/rendimiento.py medir --exe build/Release/refugio.exe
    python tools/rendimiento.py informe [carpeta]  # rehace la documentación con una medición ya hecha

«medir» ejecuta `refugio --medir-rendimiento` varias veces: cada arranque abre el juego, espera al
primer frame y lo cierra; luego juega sola dos partidas (la normal, de 10 infectados, y una horda de
100 en el mismo mapa) siguiendo un guion fijo. Mientras mide, no toques el teclado ni minimices la
ventana. Los resultados quedan en results/rendimiento/<fecha>/ y «informe» los copia a
docs/informe/datos/rendimiento/ y actualiza:
  * docs/trazabilidad.csv (estado y notas de esos RNF y CP) y las tablas generadas de 05 y 06;
  * docs/04_requisitos_no_funcionales.md (fila «Estado» de cada RNF y bloque de mediciones);
  * docs/informe_final.md (§9.1, bloque generado).
Solo usa la biblioteca estándar; si hay matplotlib, dibuja además docs/informe/figuras/rendimiento.png.
"""
from __future__ import annotations

import argparse
import csv
import io
import json
import os
import re
import shutil
import statistics as st
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
RESULTADOS = RAIZ / "results" / "rendimiento"
DATOS_DOC = RAIZ / "docs" / "informe" / "datos" / "rendimiento"
FIGURA = RAIZ / "docs" / "informe" / "figuras" / "rendimiento.png"
CSV_TRAZ = RAIZ / "docs" / "trazabilidad.csv"
DOC_RNF = RAIZ / "docs" / "04_requisitos_no_funcionales.md"
DOC_INFORME = RAIZ / "docs" / "informe_final.md"
RESUMEN_REL = "docs/informe/datos/rendimiento/resumen.json"

CANDIDATOS = [
    "build/release/refugio.exe", "build/release/refugio",
    "build/Release/refugio.exe", "build/refugio.exe", "build/refugio",
    "build/x64-Release/refugio.exe", "out/build/x64-Release/refugio.exe",
    "build/RelWithDebInfo/refugio.exe", "build/Debug/refugio.exe", "build/debug/refugio",
]

# Con VSync el juego no pasa de la frecuencia del monitor y el reloj fluctúa un poco alrededor de
# 16,67 ms: se cuenta como «60 FPS» un frame de 17,5 ms o menos (57,1 FPS).
FPS_TOLERANCIA = 1000.0 / 17.5


# ----------------------------------------------------------------------------- medir
def buscar_exe(dado: str | None) -> Path:
    if dado:
        p = Path(dado)
        if not p.is_absolute():
            p = RAIZ / p
        if not p.is_file():
            raise SystemExit(f"No existe {p}")
        return p
    for c in CANDIDATOS:
        if (RAIZ / c).is_file():
            if "debug" in c.lower():
                print(f"AVISO: solo encontré {c}; las mediciones deben hacerse en Release (CM-03).")
            return RAIZ / c
    raise SystemExit("No encontré el juego compilado. Compílalo en Release o pasa --exe RUTA.\n"
                     "Probé: " + ", ".join(CANDIDATOS))


def correr(exe: Path, args: list[str], limite: float) -> None:
    orden = [str(exe), "--medir-rendimiento", *args]
    print("  $", " ".join(orden[1:]), flush=True)
    r = subprocess.run(orden, cwd=RAIZ, timeout=limite, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       text=True, encoding="utf-8", errors="replace")
    for linea in r.stdout.splitlines():
        if linea.strip() and not linea.startswith("INFO:"):
            print("   ", linea)
    if r.returncode != 0:
        print(r.stderr[-2000:])
        raise SystemExit(f"El juego terminó con código {r.returncode}")


def medir(a: argparse.Namespace) -> Path:
    exe = buscar_exe(a.exe)
    calentamiento, segundos = (10, 60) if a.rapido else (a.calentamiento, a.segundos)
    carpeta = RESULTADOS / datetime.now().strftime("%Y%m%d-%H%M%S")
    carpeta.mkdir(parents=True, exist_ok=True)
    total = a.arranques * 3 + 2 * (calentamiento + segundos + 3)
    print(f"Juego: {exe.relative_to(RAIZ) if exe.is_relative_to(RAIZ) else exe}")
    print(f"Resultados en {carpeta.relative_to(RAIZ)}. Tardará unos {total / 60:.0f} min: "
          "no toques el teclado ni minimices la ventana.\n")
    print(f"1/3 · {a.arranques} arranques (RNF-008)")
    for i in range(1, a.arranques + 1):
        correr(exe, ["--segundos", "0", "--calentamiento", "0", "--escenario", f"arranque_{i}",
                     "--salida", str(carpeta / f"arranque_{i}")], 120)
        time.sleep(1.0)
    print(f"2/3 · partida normal, 10 infectados: {calentamiento} s de calentamiento + {segundos} s medidos")
    correr(exe, ["--segundos", str(segundos), "--calentamiento", str(calentamiento), "--escenario", "juego",
                 "--salida", str(carpeta / "juego")], calentamiento + segundos + 120)
    print(f"3/3 · horda de 100 infectados: {calentamiento} s + {segundos} s")
    correr(exe, ["--segundos", str(segundos), "--calentamiento", str(calentamiento), "--infectados", "100",
                 "--escenario", "horda100", "--salida", str(carpeta / "horda100")], calentamiento + segundos + 120)
    print()
    return carpeta


# ----------------------------------------------------------------------------- evaluar
def veredicto(valor: float | None, objetivo: float, umbral: float, menor_mejor: bool = True) -> str:
    if valor is None:
        return "Sin medir"
    ok = (lambda v, l: v <= l) if menor_mejor else (lambda v, l: v >= l)
    if ok(valor, objetivo):
        return "Cumple"
    if ok(valor, umbral):
        return "Cumple el umbral"
    return "No cumple"


def meta(j: dict) -> dict[str, str]:
    d: dict[str, str] = {}
    for linea in j.get("meta", []):
        if linea.startswith("juego="):  # «juego=… version=… commit=…» en una línea
            for m in re.finditer(r'(\w+)=(\S+)', linea):
                d[m.group(1)] = m.group(2)
        elif "=" in linea:  # el resto: una clave por línea, el valor puede tener espacios
            k, v = linea.split("=", 1)
            d[k.strip()] = v.strip().strip('"')
    return d


def coma(v: float, dec: int = 1) -> str:
    """Número con coma decimal, como el resto de la documentación."""
    return f"{v:,.{dec}f}".replace(",", " ").replace(".", ",")


def evaluar(carpeta: Path) -> dict:
    arranques = sorted(carpeta.glob("arranque_*.json"), key=lambda p: int(re.sub(r"\D", "", p.stem) or 0))
    if not arranques and not (carpeta / "juego.json").exists():
        raise SystemExit(f"{carpeta} no tiene resultados de --medir-rendimiento")
    leer = lambda p: json.loads(p.read_text(encoding="utf-8"))  # noqa: E731
    arr = [leer(p) for p in arranques]
    esc = {n: leer(carpeta / f"{n}.json") for n in ("juego", "horda100") if (carpeta / f"{n}.json").exists()}
    base = arr[0] if arr else next(iter(esc.values()))
    m, ent = meta(base), (esc.get("juego") or base).get("entorno", {})

    def t_arranque(j):
        a = j["arranque"]
        return a["primer_frame_desde_proceso_ms"] if a.get("primer_frame_desde_proceso_ms") is not None \
            else a["primer_frame_desde_main_ms"]

    r: dict = {
        "formato": "refugio-rendimiento-resumen-1",
        "carpeta": carpeta.name,
        "equipo": {"cpu": m.get("cpu", "?"), "nucleos_logicos": m.get("nucleos_logicos", "?"),
                   "ram_mb": m.get("ram_mb", "?"), "so": m.get("so", "?"), "gpu": ent.get("gpu", "?"),
                   "opengl": ent.get("opengl", "?"), "compilador": m.get("compilador", "?"),
                   "opciones": m.get("opciones", "?"), "version": m.get("version", "?"),
                   "commit": m.get("commit", "?"), "fecha_utc": m.get("fecha_utc", "?"),
                   "ventana": ent.get("ventana", "?"), "monitor_hz": ent.get("monitor_hz", "?")},
        "escenarios": {},
        "rnf": {},
    }
    if arr:
        ts = [t_arranque(j) for j in arr]
        escenas = [j["arranque"]["escenas_ms"][0] for j in arr if j["arranque"]["escenas_ms"]]
        r["arranque"] = {"n": len(ts), "ms": ts, "media_ms": st.mean(ts), "max_ms": max(ts), "primero_ms": ts[0],
                         "escena_media_ms": st.mean(escenas) if escenas else None,
                         "escena_max_ms": max(escenas) if escenas else None,
                         "fases_ms_primero": arr[0]["arranque"]["fases_ms"]}
    for n, j in esc.items():
        r["escenarios"][n] = {
            "infectados": j["infectados"], "segundos": j["medicion_s"], "frames": j["frames"],
            "reinicios": j["reinicios"], "fps_p5": j["fps"]["p5"], "fps_media": j["fps"]["media"],
            "fps_min": j["fps"]["min"], "frames_lentos_pct": 100.0 * j["frames_mas_de_25ms"] / max(1, j["frames"]),
            "frame_ms_p95": j["frame_ms"]["p95"], "trabajo_ms_p95": j["trabajo_ms"]["p95"],
            "dibujar_ms_p95": j["dibujar_ms"]["p95"], "paso_ms_p99": j["paso_ms"]["p99"],
            "paso_ms_max": j["paso_ms"]["max"], "busqueda_ia_us_p95": j["busqueda_ia_us"]["p95"],
            "cpu_media_pct": j["cpu_media_pct"], "cpu_max_pct": j["cpu_pct"]["max"],
            "memoria_pico_mb": j["memoria_pico_mb"],
        }

    peor = lambda clave, f=max: f(e[clave] for e in r["escenarios"].values()) if r["escenarios"] else None  # noqa: E731
    fps = peor("fps_p5", min)
    r["rnf"]["RNF-001"] = {"valor": fps, "objetivo": "p5 ≥ 60 FPS", "umbral": "p5 ≥ 30 FPS",
                           "veredicto": veredicto(fps, FPS_TOLERANCIA, 30.0, menor_mejor=False)}
    paso = peor("paso_ms_p99")
    r["rnf"]["RNF-002"] = {"valor": paso, "objetivo": "p99 ≤ 4 ms", "umbral": "p99 ≤ 8 ms",
                           "veredicto": veredicto(paso, 4.0, 8.0)}
    cpu = peor("cpu_media_pct")
    r["rnf"]["RNF-004"] = {"valor": cpu, "objetivo": "≤ 35 % de un núcleo", "umbral": "≤ 60 %",
                           "veredicto": veredicto(cpu, 35.0, 60.0)}
    mem = peor("memoria_pico_mb")
    r["rnf"]["RNF-005"] = {"valor": mem, "objetivo": "≤ 128 MB", "umbral": "≤ 256 MB",
                           "veredicto": veredicto(mem, 128.0, 256.0)}
    if "arranque" in r:
        a = r["arranque"]
        v1 = veredicto(a["media_ms"], 3000.0, 6000.0)
        v2 = veredicto(a["escena_media_ms"], 2000.0, 4000.0)
        orden = ["Cumple", "Cumple el umbral", "No cumple"]
        r["rnf"]["RNF-008"] = {"valor": a["media_ms"], "objetivo": "≤ 3 s y escena ≤ 2 s",
                               "umbral": "≤ 6 s y escena ≤ 4 s", "veredicto": max(v1, v2, key=orden.index)}
    else:
        r["rnf"]["RNF-008"] = {"valor": None, "objetivo": "≤ 3 s", "umbral": "≤ 6 s", "veredicto": "Sin medir"}
    return r


# ----------------------------------------------------------------------------- textos
def textos(r: dict) -> dict[str, str]:
    """Frase de resultado de cada RNF para la trazabilidad y el documento de RNF."""
    e, j, h = r["equipo"], r["escenarios"].get("juego"), r["escenarios"].get("horda100")
    t: dict[str, str] = {}
    par = lambda clave, dec, unidad="": " y ".join(  # noqa: E731
        f"{coma(x[clave], dec)}{unidad} ({n})" for n, x in (("10 infectados", j), ("100 infectados", h)) if x)
    if j or h:
        t["RNF-001"] = (f"FPS p5 {par('fps_p5', 1)}; frames de más de 25 ms: "
                        f"{par('frames_lentos_pct', 2, ' %')}.")
        t["RNF-002"] = f"Paso de simulación p99 {par('paso_ms_p99', 2, ' ms')}."
        t["RNF-004"] = f"CPU media {par('cpu_media_pct', 1, ' %')} de un núcleo lógico, con VSync."
        t["RNF-005"] = (f"Memoria residente pico del juego {par('memoria_pico_mb', 1, ' MB')}; "
                        "benchmark 256²: 13,7 MB (≤ 64 MB).")
    if "arranque" in r:
        a = r["arranque"]
        t["RNF-008"] = (f"Primer frame de la partida en {coma(a['media_ms'] / 1000, 2)} s de media en {a['n']} "
                        f"arranques (máx. {coma(a['max_ms'] / 1000, 2)} s); escena en "
                        f"{coma(a['escena_media_ms'], 1)} ms. No hay menú principal: cuenta el primer frame.")
    t["_equipo"] = (f"{e['cpu']}, {e['nucleos_logicos']} hilos, {coma(int(e['ram_mb']) / 1024, 1) if e['ram_mb'].isdigit() else e['ram_mb']} GB, "
                    f"{e['gpu']}, {e['so']}, {e['compilador']} {e['opciones'].split(' ')[0]}, ventana {e['ventana']}")
    return t


def bloque_informe(r: dict) -> str:
    e, t = r["equipo"], textos(r)
    fecha = e["fecha_utc"][:10]
    filas = [
        "### 9.1 Rendimiento con ventana en el equipo de la sustentación",
        "",
        f"Medido con `python tools/rendimiento.py medir` (versión {e['version']}, commit {e['commit']}, {fecha}) en "
        f"**{t['_equipo']}**. El juego se jugó solo con un guion fijo de movimientos (Mara recorre las 8 "
        "direcciones, camina, corre y va en sigilo) y VSync; cuando la partida termina empieza otra. Se midieron "
        "dos escenarios en el mapa del juego (32 × 22): la partida normal con 10 infectados y una horda de 100, "
        "que es la carga de CM-02 en número de agentes (el juego con ventana no tiene el mapa de 128 × 128).",
        "",
        "| RNF | Objetivo / umbral | Resultado | Estado |",
        "|---|---|---|---|",
    ]
    nombres = {"RNF-001": "FPS", "RNF-002": "Tiempo de tick", "RNF-004": "CPU", "RNF-005": "Memoria del juego",
               "RNF-008": "Arranque"}
    for k, v in r["rnf"].items():
        filas.append(f"| {k} {nombres[k]} | {v['objetivo']} / {v['umbral']} | {t.get(k, '—')} | {v['veredicto']} |")
    filas.append("")
    if r["escenarios"]:
        filas += ["| Escenario | Frames | FPS medio | FPS p5 | Trabajo por frame p95 | Paso p99 | Búsquedas IA p95 | CPU media | Memoria pico | Partidas reiniciadas |",
                  "|---|---|---|---|---|---|---|---|---|---|"]
        for n, x in r["escenarios"].items():
            filas.append(f"| {x['infectados']} infectados, {coma(x['segundos'], 0)} s | {x['frames']} | {coma(x['fps_media'], 1)} | "
                         f"{coma(x['fps_p5'], 1)} | {coma(x['trabajo_ms_p95'], 2)} ms | {coma(x['paso_ms_p99'], 2)} ms | "
                         f"{coma(x['busqueda_ia_us_p95'], 0)} µs | {coma(x['cpu_media_pct'], 1)} % | "
                         f"{coma(x['memoria_pico_mb'], 1)} MB | {x['reinicios']} |")
        filas.append("")
    filas += [
        "«Trabajo por frame» es la actualización más el dibujo, sin la espera de VSync: lo que queda hasta 16,7 ms es "
        "margen. La CPU es tiempo de CPU del proceso (todos los hilos, incluido el controlador gráfico) entre tiempo "
        "real, en % de un núcleo lógico. Con VSync a 60 Hz el juego no puede pasar de 60 FPS y el reloj fluctúa, así "
        "que un frame de hasta 17,5 ms cuenta como 60 FPS.",
        "",
        f"Datos: [`resumen.json`](informe/datos/rendimiento/resumen.json) y los JSON y CSV por frame y por segundo "
        f"de cada corrida en [`informe/datos/rendimiento/`](informe/datos/rendimiento/).",
    ]
    if FIGURA.exists():
        filas += ["", "![Rendimiento con ventana](informe/figuras/rendimiento.png)"]
    return "\n".join(filas)


def bloque_rnf(r: dict) -> str:
    t = textos(r)
    filas = [f"Mediciones con ventana (`tools/rendimiento.py`, {r['equipo']['fecha_utc'][:10]}, {t['_equipo']}):", "",
             "| RNF | Resultado | Estado |", "|---|---|---|"]
    for k, v in r["rnf"].items():
        filas.append(f"| {k} | {t.get(k, '—')} | {v['veredicto']} |")
    return "\n".join(filas)


# ----------------------------------------------------------------------------- escribir
def reemplazar_bloque(doc: Path, nombre: str, contenido: str) -> None:
    texto = doc.read_text(encoding="utf-8")
    ini, fin = f"<!-- generado:{nombre} inicio -->", f"<!-- generado:{nombre} fin -->"
    a, b = texto.find(ini), texto.find(fin)
    if a < 0 or b < 0:
        raise SystemExit(f"{doc.name}: faltan los marcadores de «{nombre}»")
    doc.write_text(texto[: a + len(ini)] + "\n" + contenido + "\n" + texto[b:], encoding="utf-8")


def actualizar_estado_rnf(r: dict) -> None:
    """Fila «Estado (...)» de cada RNF medido en docs/04_requisitos_no_funcionales.md."""
    texto = DOC_RNF.read_text(encoding="utf-8")
    t = textos(r)
    for k, v in r["rnf"].items():
        if k not in t:
            continue
        sec = re.search(rf"^### {k} .*?(?=^### |^## |\Z)", texto, flags=re.S | re.M)
        if not sec:
            continue
        estado = "Verificado" if v["veredicto"] == "Cumple" else "Implementado"
        nueva = f"| Estado (medido) | **{estado}**. {t[k]} {v['veredicto']} ({RESUMEN_REL}). |"
        cuerpo = re.sub(r"^\| Estado \(.*$", lambda _m: nueva, sec.group(0), count=1, flags=re.M)
        texto = texto[: sec.start()] + cuerpo + texto[sec.end():]
    DOC_RNF.write_text(texto, encoding="utf-8")


def actualizar_trazabilidad(r: dict) -> None:
    texto = CSV_TRAZ.read_text(encoding="utf-8")
    filas = list(csv.reader(io.StringIO(texto)))
    cab = filas[0]
    i_id, i_est, i_ver, i_not = (cab.index(c) for c in ("ID", "Estado", "Version", "Notas"))
    t, version = textos(r), r["equipo"]["version"]
    for f in filas[1:]:
        rid = f[i_id]
        req = rid if rid.startswith("RNF-") else ("RNF-0" + rid[4:] if rid.startswith("CP-1") else None)
        if req not in r["rnf"] or req not in t:
            continue
        v = r["rnf"][req]["veredicto"]
        nota = f"Medición: {RESUMEN_REL}. {t[req]} {v}."
        if rid.startswith("RNF-"):
            f[i_est] = "Verificado" if v == "Cumple" else "Implementado"
            if v == "Cumple":
                f[i_ver] = version
            f[i_not] = nota
        else:
            f[i_est] = {"Cumple": "Pasa", "Cumple el umbral": "Parcial"}.get(v, "Pendiente")
            f[i_not] = nota
    o = io.StringIO()
    csv.writer(o, lineterminator="\n").writerows(filas)
    CSV_TRAZ.write_text(o.getvalue(), encoding="utf-8")


def figura(r: dict, carpeta: Path) -> None:
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("(sin matplotlib: no se dibuja la figura)")
        return
    escenarios = [n for n in ("juego", "horda100") if (carpeta / f"{n}_frames.csv").exists()]
    if not escenarios:
        return

    def leer(nombre):
        with open(carpeta / nombre, encoding="utf-8") as f:
            return list(csv.DictReader(linea for linea in f if not linea.startswith("#")))

    fig, ejes = plt.subplots(1, 2, figsize=(10, 3.4))
    maximo = 0.0
    colores = {"juego": "#2a9d8f", "horda100": "#e76f51"}
    nombres = {"juego": "10 infectados", "horda100": "100 infectados"}
    for n in escenarios:
        fr = leer(f"{n}_frames.csv")
        trabajo = [float(x["ms_actualizar"]) + float(x["ms_dibujar"]) for x in fr]
        maximo = max([maximo, *trabajo])
        ejes[0].plot([float(x["t_s"]) for x in fr], trabajo, lw=0.5, color=colores[n], alpha=0.8, label=nombres[n])
        sg = leer(f"{n}_segundos.csv")
        ejes[1].plot([float(x["t_s"]) for x in sg], [float(x["cpu_pct"]) for x in sg], color=colores[n], label=nombres[n])
    ejes[0].axhline(1000 / 60, color="#888", ls="--", lw=0.8, label="16,7 ms (60 FPS)")
    # Un tirón aislado aplasta la escala: se recorta a 25 ms y se indica el máximo.
    if maximo > 25.0:
        ejes[0].set_ylim(0, 25)
        ejes[0].text(0.99, 0.97, f"máx. {coma(maximo, 0)} ms (fuera de escala)", transform=ejes[0].transAxes,
                     ha="right", va="top", fontsize=7, color="#555")
    ejes[0].set(xlabel="tiempo (s)", ylabel="ms por frame (sin VSync)", title="Trabajo por frame")
    ejes[1].axhline(35, color="#888", ls="--", lw=0.8, label="objetivo 35 %")
    ejes[1].set(xlabel="tiempo (s)", ylabel="% de un núcleo", title="CPU del proceso", ylim=(0, None))
    for ax in ejes:
        ax.legend(fontsize=7, frameon=False, loc="upper left")
        ax.spines[["top", "right"]].set_visible(False)
    fig.tight_layout()
    FIGURA.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIGURA, dpi=150)
    print(f"Figura: {FIGURA.relative_to(RAIZ)}")


def informe(carpeta: Path) -> int:
    r = evaluar(carpeta)
    if DATOS_DOC.exists():
        shutil.rmtree(DATOS_DOC)
    DATOS_DOC.mkdir(parents=True)
    for p in sorted(carpeta.iterdir()):
        if p.suffix in (".json", ".csv"):
            shutil.copy2(p, DATOS_DOC / p.name)
    (DATOS_DOC / "resumen.json").write_text(json.dumps(r, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    figura(r, carpeta)
    reemplazar_bloque(DOC_INFORME, "rendimiento", bloque_informe(r))
    reemplazar_bloque(DOC_RNF, "rendimiento", bloque_rnf(r))
    actualizar_estado_rnf(r)
    actualizar_trazabilidad(r)
    sys.path.insert(0, str(RAIZ / "tools"))
    import trazabilidad  # noqa: E402
    trazabilidad.main(["trazabilidad.py", "generar"])
    codigo = trazabilidad.main(["trazabilidad.py", "comprobar"])

    print("\nResultado:")
    t = textos(r)
    for k, v in r["rnf"].items():
        print(f"  {k}: {v['veredicto']:<17} {t.get(k, '')}")
    print(f"\nActualizados: {DATOS_DOC.relative_to(RAIZ)}/, docs/trazabilidad.csv, docs/04, 05, 06 y docs/informe_final.md.")
    print("Revisa los cambios con «git diff --stat» y súbelos con git add / commit / push.")
    return codigo


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="orden", required=True)
    m = sub.add_parser("medir", help="ejecuta el juego, mide y actualiza la documentación")
    m.add_argument("--exe", help="ruta del juego (por defecto se busca en build/)")
    m.add_argument("--rapido", action="store_true", help="10 s de calentamiento y 60 s por escenario")
    m.add_argument("--segundos", type=float, default=300.0, help="segundos medidos por escenario (CM-04: 300)")
    m.add_argument("--calentamiento", type=float, default=30.0, help="segundos que no cuentan (CM-04: 30)")
    m.add_argument("--arranques", type=int, default=5, help="arranques para RNF-008 (5)")
    m.add_argument("--sin-informe", action="store_true", help="solo medir, sin tocar la documentación")
    i = sub.add_parser("informe", help="actualiza la documentación con una medición ya hecha")
    i.add_argument("carpeta", nargs="?", help="carpeta de results/rendimiento (por defecto la más reciente)")
    a = ap.parse_args(argv[1:])
    if a.orden == "medir":
        carpeta = medir(a)
        return 0 if a.sin_informe else informe(carpeta)
    if a.carpeta:
        carpeta = Path(a.carpeta)
        carpeta = carpeta if carpeta.is_absolute() else Path.cwd() / carpeta
    else:
        hechas = sorted(p for p in RESULTADOS.glob("*") if p.is_dir()) if RESULTADOS.exists() else []
        if not hechas:
            raise SystemExit("No hay mediciones en results/rendimiento/. Ejecuta primero: python tools/rendimiento.py medir")
        carpeta = hechas[-1]
    return informe(carpeta)


if __name__ == "__main__":
    if os.name == "nt":  # consola de Windows: que las tildes no rompan la salida
        try:
            sys.stdout.reconfigure(encoding="utf-8")  # type: ignore[attr-defined]
        except Exception:
            pass
    sys.exit(main(sys.argv))
