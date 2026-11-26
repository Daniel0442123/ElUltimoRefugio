#!/usr/bin/env python3
"""Trazabilidad HU <-> RF <-> RNF <-> CP a partir de una sola fuente: docs/trazabilidad.csv.

Uso:
    python tools/trazabilidad.py comprobar   # revisa la coherencia; código 1 si hay errores
    python tools/trazabilidad.py generar     # reescribe las tablas generadas de 05 y 06
    python tools/trazabilidad.py evidencia CP-047   # pruebas que respaldan un caso

Qué cuenta como evidencia de un caso de prueba CP-nnn:
  * directa: un TEST_CASE / SCENARIO de tests/ cuyo nombre, etiquetas o cuerpo citan CP-nnn;
  * relacionada: un TEST_CASE etiquetado con el requisito del mismo número
    (RF-0nn -> CP-0nn, RNF-0nn -> CP-1nn);
  * CI: la columna Notas del caso empieza por «CI:» (trabajos de .github/workflows).

Reglas que comprueba (ver docs/06_matriz_trazabilidad.md §6):
  1. Los ID no se repiten y todo ID citado en Padre o Enlaces existe.
  2. Un CP en estado «Pasa» tiene evidencia directa, relacionada o de CI.
  3. Todo CP citado en las pruebas existe en el CSV.
  4. Un requisito «Verificado» tiene su CP propio (RF-0nn -> CP-0nn, RNF-0nn -> CP-1nn) en «Pasa»;
     las pruebas de aceptación CP-2nn no bloquean el estado de un requisito.
  5. Un requisito «Aprobado» (pendiente de implementar) no tiene versión; uno implementado sí.
  6. Una historia está «Terminada» solo si ninguno de sus RF sigue «Aprobado».
Solo usa la biblioteca estándar.
"""
from __future__ import annotations

import csv
import re
import sys
from collections import defaultdict
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
CSV = RAIZ / "docs" / "trazabilidad.csv"
DOC_CP = RAIZ / "docs" / "05_casos_prueba.md"
DOC_MATRIZ = RAIZ / "docs" / "06_matriz_trazabilidad.md"
PRUEBAS = RAIZ / "tests"

ESTADOS_REQ = ("Verificado", "Implementado", "Aprobado", "Propuesto", "Retirado")
ESTADOS_CP = ("Pasa", "Parcial", "Pendiente")
ESTADOS_HU = ("Terminada", "Terminada con pendientes", "Pendiente")
ID_RE = re.compile(r"\b(?:EP|HU|RF|RNF|CP)-\d{2,3}\b")
CASO_RE = re.compile(r'^\s*(TEST_CASE|SCENARIO)\s*\(\s*"((?:[^"\\]|\\.)*)"\s*(?:,\s*"([^"]*)")?')


def leer_csv() -> list[dict[str, str]]:
    with CSV.open(encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def leer_pruebas() -> list[dict]:
    """Cada TEST_CASE/SCENARIO con su archivo, línea, nombre, etiquetas e ID citados en el cuerpo."""
    casos: list[dict] = []
    for archivo in sorted(PRUEBAS.rglob("*.cpp")):
        actual = None
        for n, linea in enumerate(archivo.read_text(encoding="utf-8").splitlines(), start=1):
            m = CASO_RE.match(linea)
            if m:
                actual = {
                    "archivo": archivo.relative_to(RAIZ).as_posix(),
                    "linea": n,
                    "nombre": m.group(2),
                    "etiquetas": set(re.findall(r"\[([^\]]+)\]", m.group(3) or "")),
                    "citados": set(ID_RE.findall(m.group(2) + " " + (m.group(3) or ""))),
                }
                casos.append(actual)
            elif actual is not None:
                actual["citados"].update(c for c in ID_RE.findall(linea) if c.startswith("CP-"))
    return casos


def cp_de_requisito(req: str) -> str | None:
    if req.startswith("RF-"):
        return "CP-" + req[3:]
    if req.startswith("RNF-"):
        return "CP-1" + req[-2:]
    return None


def evidencias(casos: list[dict]) -> tuple[dict[str, list[dict]], dict[str, list[dict]]]:
    directa: dict[str, list[dict]] = defaultdict(list)
    relacionada: dict[str, list[dict]] = defaultdict(list)
    for c in casos:
        for cp in sorted(x for x in c["citados"] if x.startswith("CP-")):
            directa[cp].append(c)
        for etiqueta in c["etiquetas"]:
            cp = cp_de_requisito(etiqueta)
            if cp and c not in directa[cp]:
                relacionada[cp].append(c)
    return directa, relacionada


def comprobar(filas: list[dict[str, str]], casos: list[dict]) -> list[str]:
    errores: list[str] = []
    por_id: dict[str, dict[str, str]] = {}
    for f in filas:
        if f["ID"] in por_id:
            errores.append(f"{f['ID']}: ID repetido")
        por_id[f["ID"]] = f
    for f in filas:
        for ref in ([f["Padre"]] if f["Padre"] else []) + f["Enlaces"].split():
            if ref not in por_id:
                errores.append(f"{f['ID']}: cita {ref}, que no existe")
        if f["Tipo"] == "Requirement" and f["Estado"] not in ESTADOS_REQ:
            errores.append(f"{f['ID']}: estado «{f['Estado']}» no válido para un requisito")
        if f["Tipo"] == "Test" and f["Estado"] not in ESTADOS_CP:
            errores.append(f"{f['ID']}: estado «{f['Estado']}» no válido para un caso de prueba")
        if f["Tipo"] == "Story" and f["Estado"] not in ESTADOS_HU:
            errores.append(f"{f['ID']}: estado «{f['Estado']}» no válido para una historia")

    directa, relacionada = evidencias(casos)
    for cp in sorted(directa):
        if cp not in por_id:
            errores.append(f"{cp}: lo citan las pruebas ({directa[cp][0]['archivo']}) pero no está en el CSV")

    for f in filas:
        if f["Tipo"] == "Test" and f["Estado"] == "Pasa":
            if not directa.get(f["ID"]) and not relacionada.get(f["ID"]) and not f["Notas"].startswith("CI:"):
                errores.append(f"{f['ID']}: está en «Pasa» sin ninguna prueba ni trabajo de CI que lo respalde")
        if f["Tipo"] == "Requirement":
            propio = cp_de_requisito(f["ID"])
            propios = [g for g in filas if g["Tipo"] == "Test" and g["ID"] == propio]
            if f["Estado"] == "Verificado":
                for g in propios:
                    if g["Estado"] != "Pasa":
                        errores.append(f"{f['ID']}: está «Verificado» pero {g['ID']} está en «{g['Estado']}»")
            if f["Estado"] == "Aprobado" and f["Version"]:
                errores.append(f"{f['ID']}: pendiente de implementar pero con versión {f['Version']}")
            if f["Estado"] in ("Verificado", "Implementado") and not f["Version"]:
                errores.append(f"{f['ID']}: implementado sin versión")
        if f["Tipo"] == "Story":
            rf = [por_id[r] for r in f["Enlaces"].split() if r in por_id]
            pendientes = [r["ID"] for r in rf if r["Estado"] == "Aprobado"]
            if f["Estado"] == "Terminada" and pendientes:
                errores.append(f"{f['ID']}: «Terminada» con RF pendientes {', '.join(pendientes)}")
            if f["Estado"] == "Terminada con pendientes" and not pendientes:
                errores.append(f"{f['ID']}: «Terminada con pendientes» sin RF pendientes")
    return errores


# ----------------------------------------------------------------------------- generación
def lista(ids) -> str:
    return ", ".join(ids) if ids else "—"


def clave(i: str) -> tuple[str, int]:
    pre, num = i.rsplit("-", 1)
    return pre, int(num)


def bloque_cobertura(filas, casos) -> str:
    hu = [f for f in filas if f["Tipo"] == "Story"]
    rf = [f for f in filas if f["ID"].startswith("RF-")]
    rnf = [f for f in filas if f["ID"].startswith("RNF-")]
    cp = [f for f in filas if f["Tipo"] == "Test"]
    directa, relacionada = evidencias(casos)

    def cuenta(xs, cond):
        return sum(1 for x in xs if cond(x))

    def n_estado(xs, e):
        return cuenta(xs, lambda x: x["Estado"] == e)

    con_cp = lambda r: any(c in r["Enlaces"].split() for c in (f["ID"] for f in cp))  # noqa: E731
    automatizados = cuenta(cp, lambda c: directa.get(c["ID"]) or relacionada.get(c["ID"]) or c["Notas"].startswith("CI:"))
    filas_md = [
        "| Indicador | Valor | Objetivo |",
        "|---|---|---|",
        f"| HU con al menos un RF | {cuenta(hu, lambda h: any(x.startswith('RF-') for x in h['Enlaces'].split()))}/{len(hu)} | 100 % |",
        f"| RF con al menos un CP | {cuenta(rf, con_cp)}/{len(rf)} | 100 % |",
        f"| RNF con al menos un CP | {cuenta(rnf, con_cp)}/{len(rnf)} | 100 % |",
        f"| HU terminadas (con o sin pendientes) | {cuenta(hu, lambda h: h['Estado'].startswith('Terminada'))}/{len(hu)} | 100 % |",
        f"| RF verificados / implementados / pendientes | {n_estado(rf, 'Verificado')} / {n_estado(rf, 'Implementado')} / {n_estado(rf, 'Aprobado')} de {len(rf)} | {len(rf)} / 0 / 0 |",
        f"| RNF verificados / implementados / pendientes | {n_estado(rnf, 'Verificado')} / {n_estado(rnf, 'Implementado')} / {n_estado(rnf, 'Aprobado')} de {len(rnf)} | {len(rnf)} / 0 / 0 |",
        f"| CP que pasan / parciales / pendientes | {n_estado(cp, 'Pasa')} / {n_estado(cp, 'Parcial')} / {n_estado(cp, 'Pendiente')} de {len(cp)} | {len(cp)} / 0 / 0 |",
        f"| CP con alguna prueba automatizada o trabajo de CI | {automatizados}/{len(cp)} | ≥ 80 % |",
        f"| Pruebas automatizadas (TEST_CASE) | {len(casos)} | — |",
    ]
    return "\n".join(filas_md)


def bloque_hu(filas) -> str:
    por_id = {f["ID"]: f for f in filas}
    out = ["| HU | Título | Estado | Versión | RF | RNF relacionados | Casos de prueba |", "|---|---|---|---|---|---|---|"]
    for h in (f for f in filas if f["Tipo"] == "Story"):
        rfs = [r for r in h["Enlaces"].split() if r.startswith("RF-")]
        rnfs, cps = set(), set()
        for r in rfs:
            for x in por_id[r]["Enlaces"].split():
                if x.startswith("RNF-"):
                    rnfs.add(x)
                elif x.startswith("CP-"):
                    cps.add(x)
        for r in rnfs:
            cps.update(x for x in por_id[r]["Enlaces"].split() if x.startswith("CP-"))
        out.append(f"| {h['ID']} | {h['Resumen']} | {h['Estado']} | {h['Version'] or '—'} | {lista(rfs)} | "
                   f"{lista(sorted(rnfs, key=clave))} | {lista(sorted(cps, key=clave))} |")
    return "\n".join(out)


def bloque_rf(filas) -> str:
    hu_de = defaultdict(list)
    for h in (f for f in filas if f["Tipo"] == "Story"):
        for r in h["Enlaces"].split():
            hu_de[r].append(h["ID"])
    out = ["| RF | Título | Prio. | HU | RNF | CP | Estado | Versión | Notas |", "|---|---|---|---|---|---|---|---|---|"]
    for r in (f for f in filas if f["ID"].startswith("RF-")):
        enl = r["Enlaces"].split()
        out.append(f"| {r['ID']} | {r['Resumen']} | {r['Prioridad']} | {lista(hu_de[r['ID']])} | "
                   f"{lista([x for x in enl if x.startswith('RNF-')])} | {lista([x for x in enl if x.startswith('CP-')])} | "
                   f"{r['Estado']} | {r['Version'] or '—'} | {r['Notas'] or ''} |")
    return "\n".join(out)


def bloque_rnf(filas) -> str:
    out = ["| RNF | Título | CP | Estado | Resultado o pendiente |", "|---|---|---|---|---|"]
    for r in sorted((f for f in filas if f["ID"].startswith("RNF-")), key=lambda f: clave(f["ID"])):
        out.append(f"| {r['ID']} | {r['Resumen']} | {lista(r['Enlaces'].split())} | {r['Estado']} | {r['Notas']} |")
    return "\n".join(out)


def bloque_evidencia(filas, casos) -> str:
    directa, relacionada = evidencias(casos)
    out = ["| CP | Verifica | Estado | Pruebas automatizadas (archivo: nombre) | Notas |", "|---|---|---|---|---|"]
    for c in (f for f in filas if f["Tipo"] == "Test"):
        pruebas = [f"`{Path(p['archivo']).name}`: {p['nombre']}" for p in directa.get(c["ID"], [])]
        pruebas += [f"`{Path(p['archivo']).name}`: {p['nombre']} *(relacionada)*" for p in relacionada.get(c["ID"], [])]
        texto = "<br>".join(pruebas) if pruebas else "—"
        out.append(f"| {c['ID']} | {c['Padre']} | {c['Estado']} | {texto} | {c['Notas']} |")
    return "\n".join(out)


def bloque_resumen_cp(filas) -> str:
    grupos = (("Funcionales (CP-0nn)", "CP-0"), ("No funcionales (CP-1nn)", "CP-1"), ("Aceptación (CP-2nn)", "CP-2"))
    out = ["| Grupo | Casos | Pasan | Parciales | Pendientes |", "|---|---|---|---|---|"]
    tot = [0, 0, 0, 0]
    for nombre, pre in grupos:
        xs = [f for f in filas if f["Tipo"] == "Test" and f["ID"].startswith(pre)]
        v = [len(xs)] + [sum(1 for x in xs if x["Estado"] == e) for e in ESTADOS_CP]
        tot = [a + b for a, b in zip(tot, v)]
        out.append(f"| {nombre} | " + " | ".join(map(str, v)) + " |")
    out.append("| **Total** | " + " | ".join(f"**{t}**" for t in tot) + " |")
    return "\n".join(out)


def reemplazar(doc: Path, nombre: str, contenido: str) -> None:
    texto = doc.read_text(encoding="utf-8")
    ini, fin = f"<!-- generado:{nombre} inicio -->", f"<!-- generado:{nombre} fin -->"
    a, b = texto.find(ini), texto.find(fin)
    if a < 0 or b < 0:
        raise SystemExit(f"{doc.name}: faltan los marcadores de «{nombre}»")
    nuevo = texto[: a + len(ini)] + "\n" + contenido + "\n" + texto[b:]
    doc.write_text(nuevo, encoding="utf-8")


def main(argv: list[str]) -> int:
    orden = argv[1] if len(argv) > 1 else "comprobar"
    filas, casos = leer_csv(), leer_pruebas()
    if orden == "comprobar":
        errores = comprobar(filas, casos)
        for e in errores:
            print("ERROR:", e)
        if errores:
            print(f"{len(errores)} problemas de trazabilidad.")
            return 1
        print(f"OK: {len(filas)} elementos y {len(casos)} pruebas automatizadas coherentes.")
        return 0
    if orden == "generar":
        reemplazar(DOC_MATRIZ, "cobertura", bloque_cobertura(filas, casos))
        reemplazar(DOC_MATRIZ, "hu", bloque_hu(filas))
        reemplazar(DOC_MATRIZ, "rf", bloque_rf(filas))
        reemplazar(DOC_MATRIZ, "rnf", bloque_rnf(filas))
        reemplazar(DOC_CP, "evidencia", bloque_evidencia(filas, casos))
        reemplazar(DOC_CP, "resumen", bloque_resumen_cp(filas))
        print("Tablas regeneradas en 05_casos_prueba.md y 06_matriz_trazabilidad.md.")
        return 0
    if orden == "evidencia" and len(argv) > 2:
        directa, relacionada = evidencias(casos)
        for tipo, d in (("directa", directa), ("relacionada", relacionada)):
            for p in d.get(argv[2], []):
                print(f"{tipo}: {p['archivo']}:{p['linea']}  {p['nombre']}")
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
