#pragma once
/// @file Steering.hpp
/// @brief Comportamientos de dirección (Reynolds) para mover a la horda (RF-029).
///
/// Fuerza = w_seguir · (v_deseada − v) + w_sep · separación + w_ali · alineación
///        + w_coh · cohesión + w_evit · evitación de paredes,
/// recortada a la aceleración máxima; después v se recorta a la velocidad máxima. Los pesos son
/// configurables (y optimizables por RF-085). Sin asignaciones: los vecinos llegan como ids.

#include <cstdint>
#include <vector>

#include "dominio/mundo/Grafo.hpp"
#include "dominio/navegacion/Quadtree.hpp"

namespace refugio::navegacion {

struct ParametrosSteering {
    float pesoSeguir{8.0f};
    float pesoSeparacion{14.0f};
    float pesoAlineacion{0.4f};
    float pesoCohesion{0.2f};
    float pesoEvitacion{20.0f};
    float radioSeparacion{0.9f};  ///< Celdas: por debajo de esta distancia se empujan.
    float radioVecindad{2.0f};    ///< Celdas: vecinos para alineación y cohesión.
    float aceleracionMax{30.0f};  ///< Celdas/s².
    // Anticipación: si con las velocidades actuales dos agentes van a pasar más cerca que
    // distanciaSegura dentro de horizonteAnticipacion, se apartan ya (cruces y choques de frente).
    float pesoAnticipacion{30.0f};
    float horizonteAnticipacion{1.2f};  ///< Segundos.
    float distanciaSegura{0.9f};       ///< Celdas, entre centros.
};

struct Movil {
    Vec2 pos{};
    Vec2 vel{};
};

/// Fuerza de dirección para @p todos[yo] hacia @p velDeseada. @p vecinos son índices en
/// @p todos (puede incluir a yo; se ignora).
[[nodiscard]] Vec2 fuerzaSteering(std::uint32_t yo, const Movil* todos, const std::vector<std::uint32_t>& vecinos,
                                  Vec2 velDeseada, const mundo::GrafoRejilla& g, float radioCuerpo,
                                  const ParametrosSteering& p) noexcept;

/// Aplica la fuerza durante @p dt y devuelve la nueva velocidad recortada a @p velMax.
[[nodiscard]] Vec2 integrarVelocidad(Vec2 vel, Vec2 fuerza, float velMax, float dt) noexcept;

/// Recorta @p v a longitud máxima @p maximo.
[[nodiscard]] Vec2 recortar(Vec2 v, float maximo) noexcept;

}  // namespace refugio::navegacion
