#pragma once
/// @file Pcg32.hpp
/// @brief Generador pseudoaleatorio PCG32 (O'Neill, 2014) con semilla y flujo (RF-086, ADR-0012).
///
/// Se usa en lugar de los motores y distribuciones de <random> porque las distribuciones
/// estándar no producen la misma secuencia en todas las bibliotecas, y la reproducibilidad
/// de los experimentos lo exige (RNF-020).

#include <cstdint>

namespace refugio::nucleo {

class Pcg32 {
public:
    /// @param semilla Semilla maestra registrada en los experimentos.
    /// @param flujo   Identificador del subsistema (mapa, agentes, experimentos...).
    explicit Pcg32(std::uint64_t semilla = 42u, std::uint64_t flujo = 54u) noexcept { sembrar(semilla, flujo); }

    void sembrar(std::uint64_t semilla, std::uint64_t flujo) noexcept {
        estado_ = 0u;
        incremento_ = (flujo << 1u) | 1u;
        siguiente();
        estado_ += semilla;
        siguiente();
    }

    /// Siguiente entero de 32 bits.
    std::uint32_t siguiente() noexcept {
        const std::uint64_t anterior = estado_;
        estado_ = anterior * 6364136223846793005ULL + incremento_;
        const auto xorDesplazado = static_cast<std::uint32_t>(((anterior >> 18u) ^ anterior) >> 27u);
        const auto rotacion = static_cast<std::uint32_t>(anterior >> 59u);
        return (xorDesplazado >> rotacion) | (xorDesplazado << ((~rotacion + 1u) & 31u));
    }

    /// Entero uniforme en [0, limite) sin sesgo (método de rechazo de Lemire simplificado).
    std::uint32_t enteroMenorQue(std::uint32_t limite) noexcept {
        if (limite == 0u) return 0u;
        const std::uint32_t umbral = (~limite + 1u) % limite;
        for (;;) {
            const std::uint32_t r = siguiente();
            if (r >= umbral) return r % limite;
        }
    }

    /// Real uniforme en [0, 1).
    double real01() noexcept { return static_cast<double>(siguiente()) * (1.0 / 4294967296.0); }

private:
    std::uint64_t estado_{};
    std::uint64_t incremento_{};
};

}  // namespace refugio::nucleo
