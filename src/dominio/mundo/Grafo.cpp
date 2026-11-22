#include "dominio/mundo/Grafo.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace refugio::mundo {

GrafoRejilla::GrafoRejilla(int ancho, int alto, std::uint8_t costoInicial) : ancho_(ancho), alto_(alto) {
    if (ancho <= 0 || alto <= 0) throw std::invalid_argument("GrafoRejilla: dimensiones invalidas");
    costo_.assign(static_cast<std::size_t>(ancho) * static_cast<std::size_t>(alto),
                  std::min<std::uint8_t>(costoInicial, kCostoMax));
}

void GrafoRejilla::fijarMultiplicador(int x, int y, std::uint8_t m) noexcept {
    if (!dentro(x, y)) return;
    costo_[nodo(x, y)] = std::min<std::uint8_t>(m, kCostoMax);
}

double GrafoRejilla::distanciaOctil(Nodo a, Nodo b) const noexcept {
    const Celda ca = celda(a);
    const Celda cb = celda(b);
    const double dx = std::abs(ca.x - cb.x);
    const double dy = std::abs(ca.y - cb.y);
    // h = (dx + dy) + (sqrt(2) - 2) * min(dx, dy) = max + (sqrt(2) - 1) * min
    return (dx + dy) + (kRaiz2 - 2.0) * std::min(dx, dy);
}

}  // namespace refugio::mundo
