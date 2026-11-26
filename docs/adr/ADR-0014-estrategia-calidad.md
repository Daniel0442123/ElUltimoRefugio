# ADR-0014 · Catch2, sanitizadores, Tracy y CI

| Campo | Valor |
|---|---|
| Estado | Aceptada; herramientas y CI modificados por [ADR-0020](ADR-0020-herramientas-reales.md) |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RNF-015, RNF-016, RNF-024, RNF-025, RNF-009, RNF-028 |

## Contexto

Los RNF de fiabilidad, mantenibilidad y rendimiento deben verificarse de forma continua, y la guía pide registros y resultados de pruebas como entregable (GUIA §8).

## Decisión

Pruebas con **Catch2 v3** (incluye `SCENARIO/GIVEN/WHEN/THEN` para los criterios Gherkin) ejecutadas con CTest. CI en **GitHub Actions** con matriz Windows (MSVC, MinGW) y Ubuntu (GCC, Clang); trabajos de **ASan + UBSan**, cobertura (llvm-cov), clang-tidy, comprobación de dependencias y tamaño. Perfilado con **Tracy** en la compilación `Profile`; **Valgrind** (Massif/Memcheck) opcional en Linux. Documentación con **Doxygen** y diagramas **PlantUML**.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| GoogleTest | Muy usado, mocks con gMock | Sintaxis BDD menos natural para Gherkin |
| doctest | Muy ligero | Menos soporte de BDD y generadores |
| Catch2 v3 | BDD nativo, generadores para esquemas de escenario, buen informe | Compilación algo más lenta que doctest |

## Consecuencias

**Positivas**: Criterios de aceptación ejecutables, regresiones detectadas en cada PR, métricas de RNF reproducibles.

**Negativas**: Tiempo de CI ≈ 10 min por PR.

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RNF-015, RNF-016, RNF-024, RNF-025, RNF-009, RNF-028) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
