#pragma once
/// @file ContractNet.hpp
/// @brief Contract Net de una ronda para repartir tareas de la horda (RF-049, ADR-0009).
///
/// El infectado que ve a Mara actúa como gestor: anuncia tareas (Perseguir, Interceptar(punto),
/// Flanquear(acceso)) a los agentes que alertó; cada postor puja por cada tarea con el costo
/// estimado de llegar a su destino (heurística octil, sin búsqueda); el gestor adjudica las
/// tareas en el orden en que las anunció, cada una a la puja más baja entre los postores que
/// todavía no tienen tarea. Así cada agente recibe a lo sumo una tarea y cada tarea va a un agente.
///
/// Complejidad: O(t · n) pujas y O(t · n) para adjudicar, con t tareas y n postores.
/// Sin asignaciones si los vectores de salida ya tienen capacidad (RNF-006).

#include <cstdint>
#include <string_view>
#include <vector>

#include "dominio/mundo/Grafo.hpp"

namespace refugio::coordinacion {

enum class TipoTarea : std::uint8_t { Perseguir, Interceptar, Flanquear };

[[nodiscard]] std::string_view nombreTarea(TipoTarea t) noexcept;

struct Tarea {
    TipoTarea tipo{};
    mundo::Celda destino{};
};

struct Postor {
    std::uint32_t agente{};
    mundo::Celda celda{};
};

struct Adjudicacion {
    std::uint32_t agente{};
    std::uint32_t tarea{};  ///< Índice en la lista de tareas anunciadas.
    double costo{};
};

struct ResultadoContractNet {
    std::vector<Adjudicacion> adjudicaciones;
    std::uint32_t anuncios{0};  ///< Mensajes gestor -> postor (uno por postor y tarea).
    std::uint32_t pujas{0};     ///< Mensajes postor -> gestor.
};

/// Costo de una puja: distancia octil entre celdas (la misma heurística admisible de A*).
[[nodiscard]] double costoPuja(mundo::Celda desde, mundo::Celda hacia) noexcept;

/// Ejecuta la ronda completa. @p salida se vacía y se rellena reutilizando su memoria.
void ejecutarContractNet(const std::vector<Tarea>& tareas, const std::vector<Postor>& postores,
                         ResultadoContractNet& salida);

}  // namespace refugio::coordinacion
