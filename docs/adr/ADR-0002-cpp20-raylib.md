# ADR-0002 · C++20 con raylib 5 y raygui

| Campo | Valor |
|---|---|
| Estado | Aceptada; raygui sustituido por [ADR-0020](ADR-0020-herramientas-reales.md) |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RES-01, RES-02, RNF-001 a RNF-008, RNF-009, RNF-028 |

## Contexto

El juego debe ser muy eficiente en CPU, memoria y disco y correr en equipos modestos (NEG-07). La guía permite cualquier tecnología (GUIA §7). El equipo necesita control fino de memoria para los algoritmos y medir tiempos reales sin interferencia de un recolector de basura.

## Decisión

Se usa **C++20** (GCC 12+, Clang 15+, MSVC 2022) con **raylib 5** para ventana, entrada, render 2D y audio, y **raygui** para el panel de monitoreo. Las dependencias se obtienen con `FetchContent` de CMake fijando versiones exactas y se enlazan de forma estática.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Python + Pygame | Desarrollo rápido; sugerido en la guía | Rendimiento bajo con 100+ agentes; difícil medir memoria; GC |
| Godot / Unity | Editor completo, física, UI | Ejecutable y memoria grandes (Unity > 100 MB); la IA queda mezclada con el motor; menos control para medir |
| C++ con SDL2 | Muy portable | Más código de infraestructura que raylib para 2D |
| C++ con raylib | Ejecutable < 5 MB; API simple; sin dependencias del sistema | Menos herramientas de editor; UI limitada (raygui) |

## Consecuencias

**Positivas**: Binario pequeño, arranque rápido, memoria predecible, mediciones fiables con Tracy; cumplimiento de RNF-001 a RNF-008.

**Negativas**: Mayor riesgo de errores de memoria (mitigado con ASan/UBSan, ADR-0014); curva de aprendizaje de C++ moderno.

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RES-01, RES-02, RNF-001 a RNF-008, RNF-009, RNF-028) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
