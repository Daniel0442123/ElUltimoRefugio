#!/usr/bin/env bash
# RNF-023: el dominio no puede depender de raylib, raygui, del sistema operativo ni de E/S de ventana.
set -euo pipefail
raiz="$(cd "$(dirname "$0")/.." && pwd)"
prohibidos='#include[[:space:]]*[<"](raylib|raygui|rlgl|raymath|windows|unistd|GLFW|nlohmann)'
if grep -RInE "$prohibidos" "$raiz/src/dominio"; then
  echo "ERROR: el dominio incluye cabeceras prohibidas (ver ADR-0003)." >&2
  exit 1
fi
echo "OK: src/dominio no depende de raylib ni del sistema operativo."
