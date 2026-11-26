# ADR-0020 · Ajustar la interfaz y la estrategia de calidad a las herramientas usadas

| Campo | Valor |
|---|---|
| Estado | Aceptada (registra lo implementado hasta 0.16.0) |
| Fecha | 2026-11-25 |
| Decisores | Equipo de desarrollo |
| Modifica a | [ADR-0002](ADR-0002-cpp20-raylib.md) (raygui) y [ADR-0014](ADR-0014-estrategia-calidad.md) (herramientas y CI) |
| Requisitos relacionados | RES-02, RES-10, RNF-001 a RNF-005, RNF-009, RNF-016, RNF-024, RNF-025 |

## Contexto

ADR-0002 y ADR-0014 nombraban raygui, Tracy, Valgrind, libFuzzer, llvm-cov, clang-tidy, Doxygen y una matriz
de CI de cuatro compiladores. Durante el desarrollo se priorizaron las historias del plan y la verificación
funcional; varias de esas herramientas no llegaron a integrarse.

## Decisión

Se documenta lo que realmente se usa:

| Área | Previsto | Usado en 0.16.0 |
|---|---|---|
| Interfaz | raygui | Panel y menús dibujados con raylib 5.5 (`VistaPartida`, `MenuPausa`) |
| Pruebas | Catch2 v3 con carpetas por nivel | Catch2 v3.7.1, un ejecutable `refugio_tests` con 172 casos |
| CI | Windows (MSVC, MinGW) y Ubuntu (GCC, Clang) | Ubuntu GCC, Ubuntu Clang y Windows MSVC; ASan + UBSan; dependencias del dominio; trazabilidad |
| Rendimiento | Tracy, `fps_report.py`, `recursos.py` | `refugio_bench` (benchmark, estrés de recursión, experimentos) y el flujo nocturno `rendimiento.yml` |
| Robustez | libFuzzer 10 min | Corpus de entradas inválidas en las pruebas (CP-116) |
| Calidad estática | clang-tidy, lizard, Doxygen, cobertura | `-Wall -Wextra -Werror` en CI; clang-tidy a mano |

Lo que falta se considera deuda técnica (arc42 §11) y se prioriza solo si el informe final lo necesita:
primero la medición de FPS, tiempo de tick y CPU (RNF-001, RNF-002, RNF-004), que también sirve para el informe.

## Consecuencias

**Positivas**: la documentación dice qué se puede reproducir hoy con un clon limpio.

**Negativas**: RNF-001, RNF-004, RNF-008, RNF-024 y RNF-025 no se pueden marcar como verificados; la matriz de CI
cubre 3 de 4 compiladores (umbral de RNF-009 cumplido).

## Verificación

`python tools/trazabilidad.py comprobar` en CI mantiene los estados coherentes con las pruebas existentes.
