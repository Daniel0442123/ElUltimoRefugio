#pragma once
/// @file Escenario.hpp
/// @brief Lectura visual del mapa para el arte pixel (sin raylib, se prueba en CI).
///
/// Traduce la rejilla de la Partida a lo que se dibuja: edificios (celdas bloqueadas que no son
/// barricadas), aceras junto a ellos, calle y escombros (multiplicador > 1). También da el rayo
/// de luz que usan la linterna y las demás luces: se detiene en los muros y atraviesa las
/// barricadas, que son tablas.
///
/// Todo es de solo lectura y no usa el generador de la simulación: la decoración sale de un hash
/// de la semilla y la celda, así que el dibujo no cambia el resultado de la partida (ADR-0012).

#include <cstdint>
#include <vector>

#include "dominio/juego/Partida.hpp"
#include "dominio/mundo/Grafo.hpp"

namespace refugio::vista {

enum class Suelo : std::uint8_t { Calle, Acera, Escombros, Muro };

struct Escenario {
    int ancho{0};
    int alto{0};
    std::vector<Suelo> suelo;             ///< Una entrada por celda, en fila mayor.
    std::vector<std::uint16_t> edificio;  ///< Componente 4-conexo de cada muro (1..n); 0 si no es muro.
    std::uint16_t numEdificios{0};
    std::uint64_t firma{0};  ///< Cambia si cambia cualquier celda: el fondo se rehace solo entonces.

    [[nodiscard]] bool dentro(int x, int y) const noexcept { return x >= 0 && y >= 0 && x < ancho && y < alto; }
    [[nodiscard]] Suelo en(int x, int y) const noexcept {
        return suelo[static_cast<std::size_t>(y) * static_cast<std::size_t>(ancho) + static_cast<std::size_t>(x)];
    }
    /// Fuera del mapa cuenta como muro: las luces no salen del mapa.
    [[nodiscard]] bool muro(int x, int y) const noexcept { return !dentro(x, y) || en(x, y) == Suelo::Muro; }
};

/// Clasifica la rejilla @p g. Las celdas de @p barricadas activas se tratan como el suelo que
/// había debajo (su multiplicador previo), no como muro.
[[nodiscard]] Escenario construirEscenario(const mundo::GrafoRejilla& g, const juego::Barricada* barricadas,
                                           std::size_t numBarricadas);
[[nodiscard]] Escenario construirEscenario(const juego::Partida& p);

/// Firma rápida de la rejilla visible, para saber sin reconstruir si el fondo sigue valiendo.
[[nodiscard]] std::uint64_t firmaEscenario(const juego::Partida& p) noexcept;

/// Distancia (en celdas) que recorre un rayo desde (@p ox, @p oy) en la dirección (@p dx, @p dy)
/// hasta el primer muro, o @p maximo si no lo encuentra antes. Recorre la rejilla con DDA, así que
/// no se salta esquinas. La dirección no hace falta que sea unitaria.
[[nodiscard]] float distanciaRayo(const Escenario& e, float ox, float oy, float dx, float dy, float maximo) noexcept;

/// Hash entero de (semilla, x, y, sal) para decorar sin tocar el generador de la simulación.
[[nodiscard]] std::uint32_t hashCelda(std::uint64_t semilla, int x, int y, std::uint32_t sal = 0) noexcept;

}  // namespace refugio::vista
