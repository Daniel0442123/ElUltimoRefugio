#pragma once
/// @file CampoFlujo.hpp
/// @brief Campo de flujo compartido: un Dijkstra inverso desde el destino guía a toda la horda (RF-028, ADR-0007).
///
/// En vez de una búsqueda A* por agente, se calcula una vez el costo de llegar al destino desde
/// cada celda, c(v) = min sobre vecinos u de [ w(v, u) + c(u) ], con c(destino) = 0, y se guarda
/// para cada celda el vecino que realiza el mínimo (siguiente paso). Con k agentes hacia el mismo
/// destino el costo pasa de k búsquedas O((V + E) log V) a una sola, y cada paso de un agente es O(1).
///
/// Dijkstra inverso: los pesos del grafo son dirigidos, w(v, u) = d(v, u) · m(u) (se paga el
/// terreno de la celda a la que se entra). Al cerrar u con c(u) se relaja cada vecino v con
/// c(u) + d(v, u) · m(u). La relación de vecindad es simétrica (la regla de no cortar esquinas
/// lo es), así que recorrer los vecinos de u da exactamente los predecesores de u.
///
/// Toda la memoria se reserva con reservar(); calcular() no asigna (RNF-006).

#include <cstdint>
#include <limits>
#include <vector>

#include "dominio/mundo/Grafo.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"

namespace refugio::navegacion {

class CampoFlujo {
public:
    static constexpr double kInfinito = std::numeric_limits<double>::infinity();

    /// Reserva para un mapa de @p numNodos celdas.
    void reservar(std::uint32_t numNodos);

    /// Calcula el campo hacia @p destino. Devuelve false si el destino no es transitable.
    bool calcular(const mundo::GrafoRejilla& g, Nodo destino);

    [[nodiscard]] bool valido() const noexcept { return valido_; }
    [[nodiscard]] Nodo destino() const noexcept { return destino_; }
    /// Costo mínimo desde @p n hasta el destino (infinito si no hay camino).
    [[nodiscard]] double costo(Nodo n) const noexcept { return costo_[n]; }
    [[nodiscard]] bool alcanzable(Nodo n) const noexcept { return costo_[n] != kInfinito; }
    /// Siguiente celda desde @p n hacia el destino (el propio n si es el destino o no hay camino).
    [[nodiscard]] Nodo siguiente(Nodo n) const noexcept { return siguiente_[n]; }

    [[nodiscard]] std::uint32_t expandidos() const noexcept { return expandidos_; }
    [[nodiscard]] double tiempoUs() const noexcept { return tiempoUs_; }

private:
    std::vector<double> costo_;
    std::vector<Nodo> siguiente_;
    std::vector<std::uint8_t> cerrado_;
    MonticuloBinario abierta_;
    Nodo destino_{0};
    bool valido_{false};
    std::uint32_t expandidos_{0};
    double tiempoUs_{0.0};
};

}  // namespace refugio::navegacion
