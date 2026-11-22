#pragma once
/// @file Benchmark.hpp
/// @brief Comparación experimental de Dijkstra y A* (RF-080 a RF-082, GUIA §7).
///
/// Mismos mapas, orígenes, destinos y costos para ambos algoritmos; varios tamaños;
/// al menos 5 repeticiones por condición; semilla registrada.

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace refugio::experimentos {

struct ConfigBenchmark {
    std::vector<int> tamanos{32, 64, 128, 256};
    int pares{20};
    int repeticiones{5};
    double densidad{0.25};
    std::uint64_t semilla{42};
};

struct ResumenBenchmark {
    std::uint64_t mediciones{0};
    std::uint64_t discrepanciasOptimalidad{0};  ///< |costo A* - costo Dijkstra| > 1e-6 (RF-082).
    std::uint64_t consultasSinRuta{0};
};

/// Ejecuta el benchmark y escribe el CSV en @p csv. Las líneas de @p metadatos se escriben
/// como cabecera con prefijo «# » (RF-081). El dominio no sabe de qué equipo se trata.
ResumenBenchmark ejecutarBenchmark(const ConfigBenchmark& cfg, const std::vector<std::string>& metadatos,
                                   std::ostream& csv);

}  // namespace refugio::experimentos
