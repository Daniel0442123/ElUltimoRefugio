#pragma once
/// @file Memoria.hpp
/// @brief Memoria de evidencias del infectado (RF-043) y utilidad de un objetivo (RF-050).
///
/// Capacidad fija MEMORIA_CAPACIDAD = 16 en un arreglo dentro del propio agente: insertar no
/// asigna memoria (RNF-006). La confianza decae con semivida MEMORIA_SEMIVIDA = 10 s; con la
/// memoria llena, una evidencia nueva reemplaza a la de menor confianza actual si la supera.

#include <array>
#include <cstdint>
#include <string_view>

#include "dominio/mundo/Grafo.hpp"

namespace refugio::agentes {

enum class TipoEvidencia : std::uint8_t { Avistamiento, Ruido, Alerta, Senuelo };
[[nodiscard]] std::string_view nombreEvidencia(TipoEvidencia t) noexcept;

struct Evidencia {
    TipoEvidencia tipo{};
    mundo::Celda celda{};
    double tiempo{0.0};       ///< Segundos de partida en que se percibió.
    float confianza{0.0f};    ///< Confianza al percibirla, 0..1.
    std::uint32_t emisor{0};  ///< Quién la originó (el propio agente, otro infectado o una fuente).
};

inline constexpr std::size_t kMemoriaCapacidad = 16;
inline constexpr float kMemoriaSemivida = 10.0f;

/// c · 2^(−edad/semivida), es decir c · e^(−edad/τ) con τ = semivida / ln 2.
[[nodiscard]] float confianzaActual(const Evidencia& e, double ahora, float semivida = kMemoriaSemivida) noexcept;

/// RF-050: U = c · e^(−edad/τ) · 1 / (1 + dist/10), dist en celdas desde el agente.
[[nodiscard]] float utilidad(const Evidencia& e, double ahora, float distancia, float semivida = kMemoriaSemivida) noexcept;

class MemoriaEvidencias {
public:
    /// Inserta; si está llena reemplaza la de menor confianza actual cuando la nueva la supera.
    /// Devuelve false si la descartó.
    bool insertar(const Evidencia& e, double ahora, float semivida = kMemoriaSemivida) noexcept;

    [[nodiscard]] std::size_t tamano() const noexcept { return n_; }
    [[nodiscard]] const Evidencia& operator[](std::size_t i) const noexcept { return datos_[i]; }
    void limpiar() noexcept { n_ = 0; }

private:
    std::array<Evidencia, kMemoriaCapacidad> datos_{};
    std::size_t n_{0};
};

}  // namespace refugio::agentes
