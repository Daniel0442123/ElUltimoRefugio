# ADR-0016 · Guardar la partida como inicio + entradas y repetirla al cargar

| Campo | Valor |
|---|---|
| Estado | Aceptada |
| Fecha | 2026-11-25 |
| Decisores | Equipo de desarrollo |
| Requisitos relacionados | RF-090, RF-091, RF-093, RNF-007, RNF-017, RNF-020, RNF-021 |

## Contexto

ADR-0011 fija el contenedor de las partidas: binario con cabecera `REFG`, versión, longitud y CRC-32, escrito a un temporal y renombrado. Falta decidir qué va dentro. RF-090 pide guardar «el estado completo de la simulación y del generador aleatorio», y RF-091 que al cargar el hash del estado coincida con el del guardado.

El estado de `Partida` tiene más de 200 campos repartidos entre Mara, cada infectado (memoria de evidencias, rutas, tareas de Contract Net, tácticas, mapa de visitas), el superviviente (plan GOAP, árbol de comportamiento), barricadas, señuelos, el campo de flujo, los buscadores con presupuesto y el generador PCG32. Serializarlo a mano obliga a tocar el formato con cada historia nueva, y olvidar un campo hace que la partida cargada diverja sin que nada lo detecte.

## Decisión

La simulación es determinista (ADR-0012): con la misma semilla, los mismos parámetros y las mismas entradas por tick llega siempre al mismo estado. La partida se guarda como lo que la define:

- **Inicio**: semilla, tamaño del mapa, número de infectados, parámetros de `config/ia.json` usados, umbral del campo de flujo, algoritmo y modo de la horda iniciales.
- **Entradas**: la `EntradaJugador` de cada tick junto con el algoritmo (F2) y el modo de la horda (F4) activos, comprimidas en tramos de ticks iguales (codificación por longitud de racha con enteros LEB128). El punto apuntado con el ratón solo se guarda en los ticks con señuelo o barricada, que es cuando se usa.
- **Huella**: FNV-1a del estado al guardar (Mara, infectados, objetos, herramientas, refugio, extracción, superviviente y contadores de registro) y la versión del juego.

Cargar crea la partida desde el inicio, repite las entradas y compara la huella. Si no coincide, la partida se rechaza («La partida guardada está dañada», o «es de otra versión del juego» si la versión guardada es otra).

## Alternativas consideradas

| Alternativa | A favor | En contra |
|---|---|---|
| Serializar cada campo de `Partida` | Carga instantánea | Formato frágil que cambia con cada historia; un campo olvidado no se detecta |
| Inicio + entradas, verificado con huella | Formato pequeño y estable; el estado restaurado es exacto por construcción; la huella detecta cualquier divergencia | La carga repite la partida (unos milisegundos por minuto de juego); un cambio en la lógica de la IA invalida las partidas de versiones anteriores |

## Consecuencias

**Positivas**: 10 minutos de juego ocupan pocos KB (RNF-007, 64 KB como máximo); el generador aleatorio, las memorias y los planes quedan exactamente como estaban (CP-091); el formato no cambia cuando se añaden campos a la simulación.

**Negativas**: Una partida guardada solo se puede continuar con la misma versión del juego que la guardó, y el mensaje lo dice. La carga de una partida muy larga tarda más (se repite a la velocidad de la simulación sin dibujar).

## Verificación

CP-090, CP-091, CP-093, CP-107, CP-116, CP-117 y CP-121 en `tests/unit/test_persistencia.cpp`, según [05_casos_prueba.md](../05_casos_prueba.md).
