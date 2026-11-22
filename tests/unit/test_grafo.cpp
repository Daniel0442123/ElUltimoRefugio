#include <catch2/catch_test_macros.hpp>

#include "dominio/mundo/Grafo.hpp"

using namespace refugio::mundo;

namespace {
int contarVecinos(const GrafoRejilla& g, int x, int y) {
    int n = 0;
    g.paraCadaVecino(g.nodo(x, y), [&](Nodo, double w) {
        REQUIRE(w >= 1.0);  // pesos no negativos y >= 1 (RF-020)
        ++n;
    });
    return n;
}
}  // namespace

TEST_CASE("CP-020 Grafo 8-conexo sin cortar esquinas", "[RF-020]") {
    GrafoRejilla g(5, 5);
    CHECK(contarVecinos(g, 2, 2) == 8);
    CHECK(contarVecinos(g, 0, 0) == 3);

    // Pared en (3,2): elimina el vecino este y las diagonales (3,1) y (3,3) que la rodean.
    g.bloquear(3, 2);
    CHECK(contarVecinos(g, 2, 2) == 5);
}

TEST_CASE("CP-031 Multiplicadores de terreno en [1,10]", "[RF-031]") {
    GrafoRejilla g(3, 1);
    g.fijarMultiplicador(1, 0, 5);
    g.fijarMultiplicador(2, 0, 200);  // se recorta al máximo
    CHECK(g.multiplicador(g.nodo(1, 0)) == 5);
    CHECK(g.multiplicador(g.nodo(2, 0)) == kCostoMax);
    double peso = 0.0;
    g.paraCadaVecino(g.nodo(0, 0), [&](Nodo v, double w) {
        if (v == g.nodo(1, 0)) peso = w;
    });
    CHECK(peso == 5.0);
}

TEST_CASE("Distancia octil", "[RF-022]") {
    GrafoRejilla g(10, 10);
    CHECK(g.distanciaOctil(g.nodo(0, 0), g.nodo(3, 0)) == 3.0);
    const double d = g.distanciaOctil(g.nodo(0, 0), g.nodo(3, 3));
    CHECK(d > 3 * kRaiz2 - 1e-9);
    CHECK(d < 3 * kRaiz2 + 1e-9);
}
