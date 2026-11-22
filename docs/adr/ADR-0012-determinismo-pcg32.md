# ADR-0012 · Determinismo con PCG32 y hash de estado

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-086, RNF-020, RF-084 |

## Contexto

La guía exige registrar semillas y repetir experimentos (GUIA §7); el storyboard exige comparar aislados y coordinados con la misma semilla (STB §03).

## Decisión

Toda aleatoriedad proviene de **PCG32** con semilla maestra y flujos por subsistema. Se prohíbe `rand()`, `std::random_device` y los motores de `<random>` cuyas distribuciones varían entre implementaciones; las distribuciones se implementan a mano. Se calcula un **hash FNV-1a** del estado cada tick en pruebas.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| std::mt19937 + std::uniform_*_distribution | Estándar | Las distribuciones no son portables entre bibliotecas estándar |
| xoshiro256** | Muy rápido | Estado mayor; similar a PCG para este uso |
| PCG32 | Pequeño (16 B), rápido, flujos independientes | Implementación propia (≈ 40 líneas) |

## Consecuencias

**Positivas**: Experimentos reproducibles y depuración de comportamiento emergente repitiendo la semilla.

**Negativas**: Disciplina de no usar fuentes de aleatoriedad externas; determinismo garantizado solo por binario (RT-03).

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RF-086, RNF-020, RF-084) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
