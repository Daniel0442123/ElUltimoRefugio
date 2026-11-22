# Documentación · El último refugio

> Videojuego 2D de vista cenital con agentes inteligentes autónomos.
> Proyecto final de **Algoritmos Computacionales** · Fundación Universitaria de Popayán.
> Proyecto asignado: **2. El último refugio: apocalipsis zombi y coordinación**.

| Campo | Valor |
|---|---|
| Versión de la documentación | 0.1.0 (línea base inicial) |
| Fecha | 2026-11-21 |
| Estado | Borrador para revisión del equipo y del docente |
| Responsable | Pedro (equipo de desarrollo) |
| Fuentes primarias | `00_guia_proyecto_final.pdf` (guía general), `02_refugio.pdf` (storyboard del proyecto) |

## 1. Propósito de este conjunto de documentos

Este directorio contiene la especificación completa del sistema desde la necesidad del docente
hasta las decisiones de arquitectura, con trazabilidad bidireccional entre cada nivel. Su objetivo
es que cualquier requisito del enunciado pueda seguirse hasta el código y la prueba que lo verifica,
y que cualquier prueba pueda justificarse por un requisito.

## 2. Marco normativo y prácticas aplicadas

| Artefacto | Estándar o práctica | Documento |
|---|---|---|
| Visión y requisitos de negocio | Estructura **StRS** de ISO/IEC/IEEE 29148 (versión ligera) | [01_vision_StRS.md](01_vision_StRS.md) |
| Historias de usuario | Formato **Connextra** + verificación **INVEST** | [02_historias_usuario.md](02_historias_usuario.md) |
| Criterios de aceptación | **Gherkin** en español (`Dado / Cuando / Entonces`) | [02_historias_usuario.md](02_historias_usuario.md) |
| Requisitos funcionales formales o críticos | **EARS** + atributos de ISO 29148 (ID, prioridad, fuente, verificación…) | [03_SRS_requisitos_funcionales.md](03_SRS_requisitos_funcionales.md) |
| Requisitos no funcionales | Clasificados con **ISO/IEC 25010** y siempre medibles | [04_requisitos_no_funcionales.md](04_requisitos_no_funcionales.md) |
| Casos de prueba | Estructura derivada de ISO/IEC/IEEE 29119-3 (ligera) | [05_casos_prueba.md](05_casos_prueba.md) |
| Trazabilidad | Matriz **HU ↔ RF ↔ RNF ↔ CP**, exportable a Jira / Azure DevOps | [06_matriz_trazabilidad.md](06_matriz_trazabilidad.md) y [trazabilidad.csv](trazabilidad.csv) |
| Arquitectura | **arc42** + modelo **C4** (PlantUML) | [07_arquitectura_arc42.md](07_arquitectura_arc42.md) |
| Decisiones de arquitectura | **ADR** (formato Nygard extendido / MADR) | [adr/](adr/README.md) |

### 2.1 Sobre las versiones de ISO/IEC/IEEE 29148

| Versión citada | Situación | Uso en este proyecto |
|---|---|---|
| ISO/IEC/IEEE 29148:2018 (distribuida por SAI Global) | **Vigente**. Sustituye a la edición 2011. | Base normativa principal: procesos, características de un buen requisito y contenido de StRS/SRS. |
| ISO/IEC/IEEE 29148:2011 (revisada, SNV) | Reemplazada por la de 2018. | Referencia histórica. Sus plantillas de StRS y SRS se mantienen casi iguales en 2018, por lo que la estructura usada es compatible con ambas. |
| IEEE 29148-2011 (SNV) | Adopción IEEE de la misma edición 2011. | Igual que la anterior. Reemplazó a IEEE 830-1998. |
| ISO/IEC/IEEE DIS 29148 (SNV) | Borrador de la siguiente revisión (no publicada como norma final). | Solo informativo. No se toma ninguna cláusula del borrador como obligatoria. |

Características de requisito individual que se exigen (29148:2018, §5.2.5): **necesario, apropiado,
inequívoco, completo, singular, factible, verificable, correcto y conforme**. Características del
conjunto (§5.2.6): **completo, consistente, factible, comprensible y validable**.

## 3. Esquema de identificadores

| Prefijo | Significado | Ejemplo |
|---|---|---|
| `NEG-nn` | Requisito de negocio / objetivo del docente | NEG-03 |
| `STK-nn` | Parte interesada | STK-01 |
| `StR-nn` | Requisito de parte interesada | StR-07 |
| `EP-nn` | Épica | EP-03 |
| `HU-nn` | Historia de usuario | HU-09 |
| `RF-nnn` | Requisito funcional del sistema | RF-046 |
| `RNF-nnn` | Requisito no funcional | RNF-003 |
| `CP-nnn` | Caso de prueba | CP-046 |
| `ADR-nnnn` | Registro de decisión de arquitectura | ADR-0007 |
| `RES-nn` | Restricción de diseño | RES-02 |

Los identificadores **nunca se reutilizan**. Un requisito eliminado se marca como `Retirado` y conserva su ID.

## 4. Convenciones de redacción

* Palabras clave normativas (29148:2018 §5.2.4): **deberá** = obligatorio, **debería** = recomendado,
  **podrá** = opcional. No se usan términos ambiguos como «rápido», «amigable», «etc.» sin métrica.
* Prioridad con **MoSCoW**: `M` (Must, debe), `S` (Should, debería), `C` (Could, podría), `W` (Won't, no en esta versión).
* Métodos de verificación (29148:2018 §6.5): **I** inspección, **A** análisis, **D** demostración, **P** prueba.
* Estados de un requisito: `Propuesto → Aprobado → Implementado → Verificado` (o `Retirado`).

## 5. Orden de lectura recomendado

1. [01_vision_StRS.md](01_vision_StRS.md) para entender el problema y el alcance.
2. [02_historias_usuario.md](02_historias_usuario.md) para ver el juego desde el jugador y el docente.
3. [03_SRS_requisitos_funcionales.md](03_SRS_requisitos_funcionales.md) y [04_requisitos_no_funcionales.md](04_requisitos_no_funcionales.md) para el detalle verificable.
4. [07_arquitectura_arc42.md](07_arquitectura_arc42.md) y [adr/](adr/README.md) para el cómo.
5. [05_casos_prueba.md](05_casos_prueba.md) y [06_matriz_trazabilidad.md](06_matriz_trazabilidad.md) para la verificación.

## 6. Control de cambios

| Versión | Fecha | Autor | Cambio |
|---|---|---|---|
| 0.1.0 | 2026-11-21 | Equipo | Línea base inicial a partir de la guía y el storyboard del docente. |
