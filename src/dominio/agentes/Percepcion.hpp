#pragma once
/// @file Percepcion.hpp
/// @brief Percepción local de los infectados: visión en cono con línea de visión y oído (RF-041, RF-042).
///
/// Ningún agente lee el estado del mundo directamente: la partida construye una PercepcionInfectado
/// con estas funciones y la máquina de estados solo ve ese objeto (no omnisciencia, arc42 §8.3).

#include <cstdint>

#include "dominio/mundo/Grafo.hpp"
#include "dominio/navegacion/Quadtree.hpp"

namespace refugio::agentes {

using navegacion::Vec2;

/// Recorre con Bresenham las celdas entre @p a y @p b (sin incluir los extremos) y devuelve
/// cuántas son paredes o barricadas. Complejidad O(max(|dx|, |dy|)), sin asignaciones.
[[nodiscard]] int paredesEntre(const mundo::GrafoRejilla& g, mundo::Celda a, mundo::Celda b) noexcept;

/// Línea de visión libre: ninguna celda bloqueada entre los dos extremos (RF-041).
[[nodiscard]] inline bool lineaDeVisionLibre(const mundo::GrafoRejilla& g, mundo::Celda a, mundo::Celda b) noexcept {
    return paredesEntre(g, a, b) == 0;
}

/// ¿Está @p objetivo dentro del cono (radio en celdas, apertura total en grados) que sale de
/// @p origen mirando hacia @p orientacion (vector unitario)?
[[nodiscard]] bool dentroDelCono(Vec2 origen, Vec2 orientacion, Vec2 objetivo, float radio, float aperturaGrados) noexcept;

/// Visión completa: cono + línea de visión sobre la rejilla.
[[nodiscard]] bool ve(const mundo::GrafoRejilla& g, Vec2 origen, Vec2 orientacion, Vec2 objetivo, float radio,
                      float aperturaGrados) noexcept;

/// Oído (RF-042): se oye si la distancia es <= radio - 2 · paredes cruzadas.
[[nodiscard]] bool oye(const mundo::GrafoRejilla& g, Vec2 oyente, Vec2 origenRuido, float radio) noexcept;

[[nodiscard]] inline mundo::Celda celdaDe(Vec2 p) noexcept {
    return {static_cast<int>(p.x >= 0.0f ? p.x : p.x - 1.0f), static_cast<int>(p.y >= 0.0f ? p.y : p.y - 1.0f)};
}
[[nodiscard]] inline Vec2 centroDe(mundo::Celda c) noexcept {
    return {static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.5f};
}

}  // namespace refugio::agentes
