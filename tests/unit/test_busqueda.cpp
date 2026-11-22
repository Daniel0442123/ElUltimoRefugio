#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <cmath>
#include <limits>
#include <vector>

#include "dominio/experimentos/GeneradorMapas.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"

using namespace refugio;
using navegacion::Algoritmo;

namespace {
constexpr double kInf = std::numeric_limits<double>::infinity();

/// Referencia independiente: Bellman-Ford (relajación repetida), O(V * E). Solo para mapas pequeños.
std::vector<double> bellmanFord(const mundo::GrafoRejilla& g, mundo::Nodo origen) {
    std::vector<double> dist(g.numCeldas(), kInf);
    dist[origen] = 0.0;
    for (std::uint32_t it = 0; it < g.numCeldas(); ++it) {
        bool cambio = false;
        for (mundo::Nodo u = 0; u < g.numCeldas(); ++u) {
            if (dist[u] == kInf) continue;
            g.paraCadaVecino(u, [&](mundo::Nodo v, double w) {
                if (dist[u] + w < dist[v] - 1e-12) {
                    dist[v] = dist[u] + w;
                    cambio = true;
                }
            });
        }
        if (!cambio) break;
    }
    return dist;
}

double costoRuta(const mundo::GrafoRejilla& g, const std::vector<mundo::Nodo>& ruta) {
    double c = 0.0;
    for (std::size_t i = 1; i < ruta.size(); ++i) {
        double w = kInf;
        g.paraCadaVecino(ruta[i - 1], [&](mundo::Nodo v, double peso) {
            if (v == ruta[i]) w = peso;
        });
        c += w;
    }
    return c;
}
}  // namespace

TEST_CASE("CP-021 / CP-022 Dijkstra y A* son optimos frente a Bellman-Ford", "[RF-021][RF-022][RNF-019]") {
    const auto semilla = GENERATE(Catch::Generators::range(1, 11));  // 10 mapas
    nucleo::Pcg32 rng(static_cast<std::uint64_t>(semilla), 1u);
    const auto g = experimentos::generarMapa({16, 16, 0.25, 0.15}, rng);
    const auto pares = experimentos::generarPares(g, 10, rng);
    auto dij = navegacion::crearBuscador(Algoritmo::Dijkstra);
    auto ast = navegacion::crearBuscador(Algoritmo::AEstrella);

    for (const auto& [o, d] : pares) {
        const auto ref = bellmanFord(g, o);
        const auto rd = dij->buscar(g, o, d);
        const auto ra = ast->buscar(g, o, d);
        REQUIRE(rd.encontrada == (ref[d] != kInf));
        REQUIRE(ra.encontrada == rd.encontrada);
        if (!rd.encontrada) continue;
        CHECK(std::abs(rd.costo - ref[d]) < 1e-9);
        CHECK(std::abs(ra.costo - ref[d]) < 1e-9);
        CHECK(std::abs(costoRuta(g, ra.ruta) - ra.costo) < 1e-9);  // la ruta devuelta cuesta lo que dice
        CHECK(ra.ruta.front() == o);
        CHECK(ra.ruta.back() == d);
        CHECK(ra.expandidos <= rd.expandidos);  // A* nunca expande más que Dijkstra con h consistente
    }
}

TEST_CASE("CP-022 La heuristica octil es admisible y consistente", "[RF-022]") {
    nucleo::Pcg32 rng(99, 1u);
    const auto g = experimentos::generarMapa({12, 12, 0.25, 0.2}, rng);
    const auto destino = experimentos::generarPares(g, 1, rng).front().second;
    auto dij = navegacion::crearBuscador(Algoritmo::Dijkstra);

    for (mundo::Nodo n = 0; n < g.numCeldas(); ++n) {
        if (!g.transitable(n)) continue;
        const double h = g.distanciaOctil(n, destino);
        const auto r = dij->buscar(g, n, destino);
        if (r.encontrada) CHECK(h <= r.costo + 1e-9);  // admisible: h <= h*
        g.paraCadaVecino(n, [&](mundo::Nodo v, double w) {
            CHECK(h <= w + g.distanciaOctil(v, destino) + 1e-9);  // consistente
        });
    }
}

TEST_CASE("CP-023 Metricas de busqueda completas", "[RF-023]") {
    mundo::GrafoRejilla g(20, 20);
    auto ast = navegacion::crearBuscador(Algoritmo::AEstrella);
    const auto r = ast->buscar(g, g.nodo(0, 0), g.nodo(19, 19), true);
    REQUIRE(r.encontrada);
    CHECK(r.ruta.size() == 20);  // diagonal pura
    CHECK(r.expandidos > 0);
    CHECK(r.generados >= r.expandidos);
    CHECK(r.maxAbierta > 0);
    CHECK(r.memoriaBytes > 0);
    CHECK(r.tiempoUs >= 0.0);
    CHECK(r.ordenExpansion.size() == r.expandidos);
}

TEST_CASE("CP-027 Destino encerrado devuelve SinRuta", "[RF-027]") {
    mundo::GrafoRejilla g(9, 9);
    for (int i = 3; i <= 5; ++i) {
        g.bloquear(i, 3);
        g.bloquear(i, 5);
        g.bloquear(3, i);
        g.bloquear(5, i);
    }
    for (auto alg : {Algoritmo::Dijkstra, Algoritmo::AEstrella}) {
        auto b = navegacion::crearBuscador(alg);
        const auto r = b->buscar(g, g.nodo(0, 0), g.nodo(4, 4));
        CHECK_FALSE(r.encontrada);
        CHECK(r.ruta.empty());
    }
}

TEST_CASE("CP-025 Una barricada cambia la ruta en la siguiente busqueda", "[RF-025]") {
    mundo::GrafoRejilla g(10, 3);
    auto ast = navegacion::crearBuscador(Algoritmo::AEstrella);
    const auto antes = ast->buscar(g, g.nodo(0, 1), g.nodo(9, 1));
    g.bloquear(5, 1);
    const auto despues = ast->buscar(g, g.nodo(0, 1), g.nodo(9, 1));
    REQUIRE(despues.encontrada);
    CHECK(despues.costo > antes.costo);
    for (auto n : despues.ruta) CHECK(n != g.nodo(5, 1));
}

TEST_CASE("Reutilizar el buscador no contamina busquedas siguientes", "[RF-021]") {
    nucleo::Pcg32 rng(5, 1u);
    const auto g = experimentos::generarMapa({32, 32, 0.2, 0.1}, rng);
    const auto pares = experimentos::generarPares(g, 30, rng);
    auto reutilizado = navegacion::crearBuscador(Algoritmo::AEstrella);
    for (const auto& [o, d] : pares) {
        auto nuevo = navegacion::crearBuscador(Algoritmo::AEstrella);
        const auto a = reutilizado->buscar(g, o, d);
        const auto b = nuevo->buscar(g, o, d);
        REQUIRE(a.encontrada == b.encontrada);
        CHECK(a.costo == b.costo);
        CHECK(a.expandidos == b.expandidos);
    }
}
