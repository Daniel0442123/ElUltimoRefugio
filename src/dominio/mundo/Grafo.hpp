#pragma once
/// @file Grafo.hpp
/// @brief Grafo implícito sobre una rejilla 8-conexa (RF-020, RF-031, ADR-0006).
///
/// - Vértices V: celdas transitables (multiplicador m(v) >= 1).
/// - Aristas E: hasta 8 vecinos; una diagonal solo existe si las dos celdas ortogonales
///   que rodea son transitables (no se cortan esquinas).
/// - Peso: w(u, v) = d(u, v) * m(v), con d = 1 ortogonal y d = sqrt(2) diagonal. Como m >= 1,
///   todos los pesos son >= 1 > 0, condición de corrección de Dijkstra.
/// - No se guardan listas de adyacencia: |E| <= 8|V| se recorre al vuelo y la memoria es O(V).

#include <cstdint>
#include <vector>

namespace refugio::mundo {

using Nodo = std::uint32_t;

inline constexpr double kRaiz2 = 1.41421356237309504880;
inline constexpr std::uint8_t kBloqueado = 0;   ///< Valor de m(v) para una celda no transitable.
inline constexpr std::uint8_t kCostoMin = 1;
inline constexpr std::uint8_t kCostoMax = 10;

struct Celda {
    int x{};
    int y{};
    friend bool operator==(const Celda&, const Celda&) = default;
};

class GrafoRejilla {
public:
    GrafoRejilla(int ancho, int alto, std::uint8_t costoInicial = kCostoMin);

    [[nodiscard]] int ancho() const noexcept { return ancho_; }
    [[nodiscard]] int alto() const noexcept { return alto_; }
    [[nodiscard]] std::uint32_t numCeldas() const noexcept { return static_cast<std::uint32_t>(costo_.size()); }

    [[nodiscard]] bool dentro(int x, int y) const noexcept { return x >= 0 && y >= 0 && x < ancho_ && y < alto_; }
    [[nodiscard]] Nodo nodo(int x, int y) const noexcept { return static_cast<Nodo>(y * ancho_ + x); }
    [[nodiscard]] Celda celda(Nodo n) const noexcept {
        return {static_cast<int>(n % static_cast<Nodo>(ancho_)), static_cast<int>(n / static_cast<Nodo>(ancho_))};
    }

    [[nodiscard]] std::uint8_t multiplicador(Nodo n) const noexcept { return costo_[n]; }
    [[nodiscard]] bool transitable(Nodo n) const noexcept { return costo_[n] != kBloqueado; }
    [[nodiscard]] bool transitable(int x, int y) const noexcept { return dentro(x, y) && transitable(nodo(x, y)); }

    /// Fija m(v). Valores mayores que kCostoMax se recortan. 0 bloquea la celda (barricada, pared).
    void fijarMultiplicador(int x, int y, std::uint8_t m) noexcept;
    void bloquear(int x, int y) noexcept { fijarMultiplicador(x, y, kBloqueado); }

    /// Recorre los vecinos de @p u llamando a f(v, peso). Sin asignaciones de memoria.
    template <typename F>
    void paraCadaVecino(Nodo u, F&& f) const {
        const Celda c = celda(u);
        for (const auto& d : kDirecciones) {
            const int nx = c.x + d.dx;
            const int ny = c.y + d.dy;
            if (!transitable(nx, ny)) continue;
            if (d.dx != 0 && d.dy != 0 && (!transitable(c.x + d.dx, c.y) || !transitable(c.x, c.y + d.dy))) continue;
            const Nodo v = nodo(nx, ny);
            f(v, d.distancia * static_cast<double>(costo_[v]));
        }
    }

    /// Distancia octil entre dos celdas: costo exacto sin obstáculos y con m = 1 (RF-022).
    [[nodiscard]] double distanciaOctil(Nodo a, Nodo b) const noexcept;

private:
    struct Direccion {
        int dx;
        int dy;
        double distancia;
    };
    static constexpr Direccion kDirecciones[8] = {{1, 0, 1.0},  {-1, 0, 1.0},     {0, 1, 1.0},     {0, -1, 1.0},
                                                  {1, 1, kRaiz2}, {1, -1, kRaiz2}, {-1, 1, kRaiz2}, {-1, -1, kRaiz2}};
    int ancho_;
    int alto_;
    std::vector<std::uint8_t> costo_;
};

}  // namespace refugio::mundo
