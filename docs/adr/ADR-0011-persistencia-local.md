# ADR-0011 · Persistencia solo local: JSON y binario con CRC

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-21 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-090 a RF-093, RNF-007, RNF-016, RNF-017, RNF-021, RNF-022 |

## Contexto

No hay backend ni nube (RES-05). Las partidas deben ser pequeñas y robustas, y la configuración editable a mano.

## Decisión

Configuración en **JSON** legible (`config/*.json`). Partidas en **binario** versionado con cabecera `REFG`, versión, longitud y **CRC32**, escritas a un temporal y renombradas de forma atómica. Registros en texto con rotación; resultados en CSV y JSON. No se abre ninguna conexión de red.

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| SQLite | Consultas, transacciones | Dependencia y tamaño innecesarios |
| JSON también para partidas | Legible | 3–5 veces más grande y lento; fácil de manipular |
| Binario + CRC y JSON para configuración | Pequeño, rápido, íntegro; configuración editable | Requiere versionar el formato binario |

## Consecuencias

**Positivas**: Partidas ≤ 64 KB, recuperables ante cortes, sin dependencias.

**Negativas**: Migraciones de versión del formato binario a cargo del equipo.

## Verificación

La decisión se considera válida mientras se cumplan los requisitos relacionados (RF-090 a RF-093, RNF-007, RNF-016, RNF-017, RNF-021, RNF-022) según sus casos de prueba en [05_casos_prueba.md](../05_casos_prueba.md).
