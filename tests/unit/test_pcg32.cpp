#include <catch2/catch_test_macros.hpp>

#include "dominio/nucleo/Pcg32.hpp"

using refugio::nucleo::Pcg32;

TEST_CASE("CP-086 PCG32 es reproducible con la misma semilla", "[RF-086][RNF-020]") {
    Pcg32 a(42, 1), b(42, 1), c(43, 1), d(42, 2);
    bool igualC = true, igualD = true;
    for (int i = 0; i < 1000; ++i) {
        const auto va = a.siguiente();
        REQUIRE(va == b.siguiente());
        igualC = igualC && va == c.siguiente();
        igualD = igualD && va == d.siguiente();
    }
    CHECK_FALSE(igualC);  // otra semilla -> otra secuencia
    CHECK_FALSE(igualD);  // otro flujo -> otra secuencia
}

TEST_CASE("PCG32: enteroMenorQue respeta el limite", "[RF-086]") {
    Pcg32 r(7);
    for (int i = 0; i < 10000; ++i) REQUIRE(r.enteroMenorQue(10) < 10u);
    for (int i = 0; i < 1000; ++i) {
        const double x = r.real01();
        REQUIRE(x >= 0.0);
        REQUIRE(x < 1.0);
    }
}
