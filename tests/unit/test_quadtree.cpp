#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

#include "dominio/navegacion/Quadtree.hpp"
#include "dominio/nucleo/Pcg32.hpp"

using namespace refugio::navegacion;

TEST_CASE("CP-030 Quadtree coincide con fuerza bruta", "[RF-030]") {
    refugio::nucleo::Pcg32 rng(3);
    std::vector<Vec2> pts;
    Quadtree qt({0, 0, 100, 100});
    for (std::uint32_t i = 0; i < 1000; ++i) {
        pts.push_back({static_cast<float>(rng.real01() * 100.0), static_cast<float>(rng.real01() * 100.0)});
        REQUIRE(qt.insertar(i, pts.back()));
    }
    for (int q = 0; q < 50; ++q) {
        const Vec2 c{static_cast<float>(rng.real01() * 100.0), static_cast<float>(rng.real01() * 100.0)};
        const float r = 2.0f + static_cast<float>(rng.real01() * 10.0);
        std::vector<std::uint32_t> obtenidos;
        qt.consultarRadio(c, r, obtenidos);
        std::vector<std::uint32_t> esperados;
        for (std::uint32_t i = 0; i < pts.size(); ++i) {
            const float dx = pts[i].x - c.x, dy = pts[i].y - c.y;
            if (dx * dx + dy * dy <= r * r) esperados.push_back(i);
        }
        std::sort(obtenidos.begin(), obtenidos.end());
        REQUIRE(obtenidos == esperados);
    }
    CHECK(qt.profundidadMaxima() <= Quadtree::kProfundidadMax);
}

TEST_CASE("CP-127 Quadtree con 10000 puntos coincidentes no supera la profundidad máxima", "[RF-030][RNF-027]") {
    Quadtree qt({0, 0, 64, 64});
    for (std::uint32_t i = 0; i < 10000; ++i) qt.insertar(i, {10.0f, 10.0f});
    CHECK(qt.profundidadMaxima() == Quadtree::kProfundidadMax);
    std::vector<std::uint32_t> salida;
    qt.consultarRadio({10.0f, 10.0f}, 0.5f, salida);
    CHECK(salida.size() == 10000);
}

TEST_CASE("Quadtree rechaza puntos fuera de los límites y se puede limpiar", "[RF-030]") {
    Quadtree qt({0, 0, 10, 10});
    CHECK_FALSE(qt.insertar(0, {11.0f, 5.0f}));
    CHECK(qt.insertar(1, {5.0f, 5.0f}));
    qt.limpiar();
    std::vector<std::uint32_t> s;
    qt.consultarRadio({5.0f, 5.0f}, 3.0f, s);
    CHECK(s.empty());
}
