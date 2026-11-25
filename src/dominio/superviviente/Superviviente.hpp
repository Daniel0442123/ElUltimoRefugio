#pragma once
/// @file Superviviente.hpp
/// @brief Metas, hechos y acciones del superviviente de la radio (HU-13, RF-060, RF-061).
///
/// Utility AI elige la meta (RF-060), GOAP encadena las acciones que la cumplen (RF-061) y el
/// árbol de comportamiento las ejecuta (RF-062). Aquí está el vocabulario común a las tres
/// piezas; la partida aporta la percepción y la ejecución sobre el mapa.

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "dominio/superviviente/Goap.hpp"

namespace refugio::superviviente {

enum class Meta : std::uint8_t { RefugioSeguro, AyudarInstalacion, PonerseASalvo, Vigilar };
inline constexpr std::size_t kNumMetas = 4;
[[nodiscard]] std::string_view nombreMeta(Meta m) noexcept;

/// Hechos del estado del mundo, vistos por el superviviente.
namespace hecho {
inline constexpr Hechos EnAlmacen = 1u << 0;
inline constexpr Hechos EnPuerta = 1u << 1;
inline constexpr Hechos TieneTablas = 1u << 2;
inline constexpr Hechos PuertaReparada = 1u << 3;
inline constexpr Hechos Ayudando = 1u << 4;
inline constexpr Hechos Vigilando = 1u << 5;
inline constexpr Hechos ASalvo = 1u << 6;
}  // namespace hecho

/// Hecho que cumple cada meta.
[[nodiscard]] Hechos hechoDeMeta(Meta m) noexcept;

enum class Accion : std::uint8_t { IrAlmacen, RecogerTablas, ArrancarTablas, IrPuerta, RepararPuerta, AyudarInstalacion, Vigilar, Huir };
inline constexpr std::size_t kNumAcciones = 8;
[[nodiscard]] std::string_view nombreAccion(Accion a) noexcept;

/// Catálogo GOAP con costos en segundos estimados (ninguna acción añade más de un hecho).
struct CostosAcciones {
    float irAlmacen{3.0f};
    float recoger{1.0f};
    float arrancar{8.0f};
    float irPuerta{3.0f};
    float reparar{5.0f};
    float ayudar{1.0f};
    float vigilar{1.0f};
    float huir{1.0f};
    bool almacenAccesible{true};
};
[[nodiscard]] std::array<AccionGoap, kNumAcciones> catalogoAcciones(const CostosAcciones& c) noexcept;

// ------------------------------------------------------------------ Utilidad (RF-060)
struct ParametrosUtilidad {
    float distanciaPeligro{3.0f};  ///< Punto medio de la curva logística del peligro, celdas.
    float pendientePeligro{2.0f};  ///< Lo abrupta que es esa curva.
    float ayuda{0.8f};             ///< Utilidad de ayudar mientras Mara instala.
    float vigilar{0.2f};           ///< Utilidad de base: vigilar cuando no hay nada más.
    float histeresis{0.1f};        ///< RF-060: la nueva meta debe superar a la actual en 0,1.
};

/// Lo que percibe el superviviente (nada global: distancia al infectado más cercano que ve).
struct EntradaUtilidad {
    float integridad01{1.0f};    ///< Integridad del refugio en [0, 1].
    float distanciaAmenaza{1e9f};  ///< Celdas al infectado visible más cercano (1e9 si no ve ninguno).
    bool maraInstalando{false};
};

/// Curvas normalizadas en [0, 1]:
/// - peligro p(d) = 1 / (1 + e^{k (d - d0)})   (logística decreciente con la distancia)
/// - PonerseASalvo = p
/// - RefugioSeguro = (1 - integridad) · (1 - p)
/// - AyudarInstalacion = ayuda · (1 - p) si Mara instala, si no 0
/// - Vigilar = constante
[[nodiscard]] std::array<float, kNumMetas> puntuarMetas(const EntradaUtilidad& e, const ParametrosUtilidad& p = {}) noexcept;

/// Meta de mayor puntuación; solo sustituye a @p actual si la supera en más de la histéresis.
[[nodiscard]] Meta elegirMeta(const std::array<float, kNumMetas>& u, Meta actual, float histeresis) noexcept;

}  // namespace refugio::superviviente
