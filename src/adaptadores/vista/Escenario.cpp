#include "adaptadores/vista/Escenario.hpp"

#include <cmath>
#include <limits>

namespace refugio::vista {
namespace {

std::uint64_t mezclar(std::uint64_t h, std::uint64_t v) noexcept {
    h ^= v + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
    return h;
}

/// Multiplicador visible de cada celda: el de la rejilla, salvo bajo una barricada activa.
std::uint8_t multiplicadorVisible(const mundo::GrafoRejilla& g, int x, int y, const juego::Barricada* b,
                                  std::size_t nb) noexcept {
    for (std::size_t i = 0; i < nb; ++i)
        if (b[i].activa && b[i].celda.x == x && b[i].celda.y == y) return b[i].multiplicadorPrevio;
    return g.multiplicador(g.nodo(x, y));
}

}  // namespace

Escenario construirEscenario(const mundo::GrafoRejilla& g, const juego::Barricada* barricadas,
                             std::size_t numBarricadas) {
    Escenario e;
    e.ancho = g.ancho();
    e.alto = g.alto();
    const auto n = static_cast<std::size_t>(e.ancho) * static_cast<std::size_t>(e.alto);
    e.suelo.assign(n, Suelo::Calle);
    e.edificio.assign(n, 0);
    std::uint64_t firma = 1469598103934665603ull;
    for (int y = 0; y < e.alto; ++y)
        for (int x = 0; x < e.ancho; ++x) {
            const std::uint8_t m = multiplicadorVisible(g, x, y, barricadas, numBarricadas);
            const auto i =
                static_cast<std::size_t>(y) * static_cast<std::size_t>(e.ancho) + static_cast<std::size_t>(x);
            e.suelo[i] = m == mundo::kBloqueado ? Suelo::Muro : m > mundo::kCostoMin ? Suelo::Escombros : Suelo::Calle;
            firma = mezclar(firma, m);
        }
    e.firma = firma;
    // Aceras: calle con un edificio al lado (4-vecinos). Los escombros siguen siendo escombros.
    for (int y = 0; y < e.alto; ++y)
        for (int x = 0; x < e.ancho; ++x) {
            const auto i =
                static_cast<std::size_t>(y) * static_cast<std::size_t>(e.ancho) + static_cast<std::size_t>(x);
            if (e.suelo[i] != Suelo::Calle) continue;
            const auto esMuro = [&e](int cx, int cy) { return e.dentro(cx, cy) && e.en(cx, cy) == Suelo::Muro; };
            if (esMuro(x + 1, y) || esMuro(x - 1, y) || esMuro(x, y + 1) || esMuro(x, y - 1)) e.suelo[i] = Suelo::Acera;
        }
    // Edificios: componentes 4-conexos de muro, con una pila explícita (sin recursión).
    std::vector<std::size_t> pila;
    pila.reserve(n);
    for (std::size_t s = 0; s < n; ++s) {
        if (e.suelo[s] != Suelo::Muro || e.edificio[s] != 0) continue;
        if (e.numEdificios == std::numeric_limits<std::uint16_t>::max()) break;
        const auto id = ++e.numEdificios;
        e.edificio[s] = id;
        pila.push_back(s);
        while (!pila.empty()) {
            const std::size_t c = pila.back();
            pila.pop_back();
            const int cx = static_cast<int>(c % static_cast<std::size_t>(e.ancho));
            const int cy = static_cast<int>(c / static_cast<std::size_t>(e.ancho));
            const int vx[4] = {cx + 1, cx - 1, cx, cx};
            const int vy[4] = {cy, cy, cy + 1, cy - 1};
            for (int k = 0; k < 4; ++k) {
                if (!e.dentro(vx[k], vy[k])) continue;
                const auto v = static_cast<std::size_t>(vy[k]) * static_cast<std::size_t>(e.ancho) +
                               static_cast<std::size_t>(vx[k]);
                if (e.suelo[v] == Suelo::Muro && e.edificio[v] == 0) {
                    e.edificio[v] = id;
                    pila.push_back(v);
                }
            }
        }
    }
    return e;
}

Escenario construirEscenario(const juego::Partida& p) {
    return construirEscenario(p.mapa(), p.barricadas().data(), p.barricadas().size());
}

std::uint64_t firmaEscenario(const juego::Partida& p) noexcept {
    const auto& g = p.mapa();
    const auto& b = p.barricadas();
    std::uint64_t firma = 1469598103934665603ull;
    for (int y = 0; y < g.alto(); ++y)
        for (int x = 0; x < g.ancho(); ++x) firma = mezclar(firma, multiplicadorVisible(g, x, y, b.data(), b.size()));
    return firma;
}

float distanciaRayo(const Escenario& e, float ox, float oy, float dx, float dy, float maximo) noexcept {
    const float largo = std::sqrt(dx * dx + dy * dy);
    if (largo <= 0.0f || maximo <= 0.0f) return 0.0f;
    dx /= largo;
    dy /= largo;
    int cx = static_cast<int>(std::floor(ox));
    int cy = static_cast<int>(std::floor(oy));
    if (e.muro(cx, cy)) return 0.0f;
    const int pasoX = dx > 0.0f ? 1 : -1;
    const int pasoY = dy > 0.0f ? 1 : -1;
    constexpr float kInf = std::numeric_limits<float>::infinity();
    const float deltaX = dx != 0.0f ? std::fabs(1.0f / dx) : kInf;
    const float deltaY = dy != 0.0f ? std::fabs(1.0f / dy) : kInf;
    float tX =
        dx != 0.0f ? (dx > 0.0f ? (static_cast<float>(cx + 1) - ox) : (ox - static_cast<float>(cx))) * deltaX : kInf;
    float tY =
        dy != 0.0f ? (dy > 0.0f ? (static_cast<float>(cy + 1) - oy) : (oy - static_cast<float>(cy))) * deltaY : kInf;
    for (;;) {
        float t;
        if (tX < tY) {
            t = tX;
            tX += deltaX;
            cx += pasoX;
        } else {
            t = tY;
            tY += deltaY;
            cy += pasoY;
        }
        if (t >= maximo) return maximo;
        if (e.muro(cx, cy)) return t;
    }
}

std::uint32_t hashCelda(std::uint64_t semilla, int x, int y, std::uint32_t sal) noexcept {
    // splitmix64 sobre la combinación: barato y sin estado.
    std::uint64_t z = semilla ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32) ^
                      static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) ^
                      (static_cast<std::uint64_t>(sal) << 48);
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    return static_cast<std::uint32_t>(z >> 16);
}

}  // namespace refugio::vista
