#pragma once
/// @file AsignadorAccesos.hpp
/// @brief Reparto de celdas de llegada alrededor de un objetivo común (RF-051, ADR-0009).
///
/// Los accesos son las celdas transitables que rodean al objetivo (anillo de radio 1). Cada
/// acceso admite como máximo MAX_AGENTES_POR_CELDA agentes. Se busca minimizar la suma de
/// distancias con un voraz por costo: se ordenan todos los pares (agente, acceso) por distancia
/// octil y se acepta cada par si el agente no tiene acceso y el acceso no está lleno.
/// Complejidad O(n · s log(n · s)) con s <= 8 accesos. El voraz no siempre es óptimo (el Húngaro
/// sí, en O(n³)), pero con s <= 8 la diferencia medida es pequeña y es fácil de explicar.

#include <cstdint>
#include <vector>

#include "dominio/mundo/Grafo.hpp"

namespace refugio::coordinacion {

struct Asignacion {
    std::uint32_t agente{};
    mundo::Celda acceso{};
    bool asignado{false};  ///< false si no quedaban accesos con capacidad.
};

class AsignadorAccesos {
public:
    /// Reserva para @p maxAgentes agentes (8 accesos como máximo).
    void reservar(std::size_t maxAgentes);

    /// @param agentes ids y celdas de los agentes con el mismo objetivo.
    /// @param salida  una entrada por agente, en el mismo orden.
    void asignar(const mundo::GrafoRejilla& g, mundo::Celda objetivo, const std::vector<std::uint32_t>& ids,
                 const std::vector<mundo::Celda>& celdas, int maxPorCelda, std::vector<Asignacion>& salida);

private:
    struct Par {
        double costo;
        std::uint32_t agente;  ///< Índice local en ids.
        std::uint32_t acceso;
    };
    std::vector<mundo::Celda> accesos_;
    std::vector<int> ocupacion_;
    std::vector<Par> pares_;
};

}  // namespace refugio::coordinacion
