#include "dominio/experimentos/GeneradorMapas.hpp"

#include <stdexcept>

namespace refugio::experimentos {

mundo::GrafoRejilla generarMapa(const ParametrosMapa& p, nucleo::Pcg32& rng) {
    mundo::GrafoRejilla g(p.ancho, p.alto);
    const auto total = static_cast<double>(p.ancho) * static_cast<double>(p.alto);
    const auto bloqueadasObjetivo = static_cast<long>(total * p.densidadObstaculos);
    long bloqueadas = 0;

    // Manzanas rectangulares (≈ 70 % de los obstáculos) separadas por calles.
    int intentos = 0;
    while (bloqueadas < bloqueadasObjetivo * 7 / 10 && intentos++ < 100000) {
        const int w = 2 + static_cast<int>(rng.enteroMenorQue(5));
        const int h = 2 + static_cast<int>(rng.enteroMenorQue(5));
        const int x0 = static_cast<int>(rng.enteroMenorQue(static_cast<std::uint32_t>(p.ancho)));
        const int y0 = static_cast<int>(rng.enteroMenorQue(static_cast<std::uint32_t>(p.alto)));
        for (int y = y0; y < y0 + h; ++y)
            for (int x = x0; x < x0 + w; ++x)
                if (g.transitable(x, y)) {
                    g.bloquear(x, y);
                    ++bloqueadas;
                }
    }
    // Obstáculos sueltos hasta la densidad pedida.
    intentos = 0;
    while (bloqueadas < bloqueadasObjetivo && intentos++ < 1000000) {
        const int x = static_cast<int>(rng.enteroMenorQue(static_cast<std::uint32_t>(p.ancho)));
        const int y = static_cast<int>(rng.enteroMenorQue(static_cast<std::uint32_t>(p.alto)));
        if (g.transitable(x, y)) {
            g.bloquear(x, y);
            ++bloqueadas;
        }
    }
    // Terreno caro (escombros): m en [2, 5].
    for (int y = 0; y < p.alto; ++y)
        for (int x = 0; x < p.ancho; ++x)
            if (g.transitable(x, y) && rng.real01() < p.fraccionTerrenoCaro)
                g.fijarMultiplicador(x, y, static_cast<std::uint8_t>(2u + rng.enteroMenorQue(4)));
    return g;
}

std::vector<std::pair<mundo::Nodo, mundo::Nodo>> generarPares(const mundo::GrafoRejilla& g, int n,
                                                              nucleo::Pcg32& rng) {
    std::vector<mundo::Nodo> libres;
    for (mundo::Nodo v = 0; v < g.numCeldas(); ++v)
        if (g.transitable(v)) libres.push_back(v);
    if (libres.size() < 2) throw std::runtime_error("generarPares: el mapa no tiene suficientes celdas libres");
    std::vector<std::pair<mundo::Nodo, mundo::Nodo>> pares;
    pares.reserve(static_cast<std::size_t>(n));
    const auto m = static_cast<std::uint32_t>(libres.size());
    while (static_cast<int>(pares.size()) < n) {
        const mundo::Nodo a = libres[rng.enteroMenorQue(m)];
        const mundo::Nodo b = libres[rng.enteroMenorQue(m)];
        if (a != b) pares.emplace_back(a, b);
    }
    return pares;
}

}  // namespace refugio::experimentos
