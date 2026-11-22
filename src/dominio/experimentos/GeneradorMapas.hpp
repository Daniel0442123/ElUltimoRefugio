#pragma once
/// @file GeneradorMapas.hpp
/// @brief Mapas procedurales reproducibles para el benchmark (RF-080, RF-086).

#include <cstdint>
#include <utility>
#include <vector>

#include "dominio/mundo/Grafo.hpp"
#include "dominio/nucleo/Pcg32.hpp"

namespace refugio::experimentos {

struct ParametrosMapa {
    int ancho{64};
    int alto{64};
    double densidadObstaculos{0.25};   ///< Fracción de celdas bloqueadas.
    double fraccionTerrenoCaro{0.10};  ///< Fracción de celdas con m en [2, 5].
};

/// Genera una «ciudad» con manzanas y obstáculos sueltos según la semilla.
[[nodiscard]] mundo::GrafoRejilla generarMapa(const ParametrosMapa& p, nucleo::Pcg32& rng);

/// Elige @p n pares (origen, destino) de celdas transitables y distintas.
[[nodiscard]] std::vector<std::pair<mundo::Nodo, mundo::Nodo>> generarPares(const mundo::GrafoRejilla& g, int n,
                                                                           nucleo::Pcg32& rng);

}  // namespace refugio::experimentos
