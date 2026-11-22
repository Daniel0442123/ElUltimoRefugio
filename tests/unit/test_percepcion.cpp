#include <catch2/catch_test_macros.hpp>

#include "dominio/agentes/Percepcion.hpp"

using namespace refugio;
using agentes::Vec2;

TEST_CASE("CP-041 Vision: cono, radio y linea de vision con Bresenham", "[RF-041][HU-08]") {
    mundo::GrafoRejilla g(20, 20);
    const Vec2 ojo{2.5f, 10.5f};
    const Vec2 haciaEste{1.0f, 0.0f};

    SECTION("Delante y dentro del radio: se ve") { CHECK(agentes::ve(g, ojo, haciaEste, {7.5f, 10.5f}, 6.0f, 110.0f)); }
    SECTION("Fuera del radio: no se ve") { CHECK_FALSE(agentes::ve(g, ojo, haciaEste, {9.5f, 10.5f}, 6.0f, 110.0f)); }
    SECTION("Detras del agente: no se ve") { CHECK_FALSE(agentes::ve(g, ojo, haciaEste, {0.5f, 10.5f}, 6.0f, 110.0f)); }
    SECTION("Fuera de la apertura del cono: no se ve") {
        CHECK_FALSE(agentes::ve(g, ojo, haciaEste, {3.5f, 14.5f}, 6.0f, 110.0f));
    }
    SECTION("Una pared en medio tapa la vision") {
        g.bloquear(5, 10);
        CHECK_FALSE(agentes::ve(g, ojo, haciaEste, {7.5f, 10.5f}, 6.0f, 110.0f));
        CHECK(agentes::paredesEntre(g, {2, 10}, {7, 10}) == 1);
    }
    SECTION("Los escombros no tapan la vision") {
        g.fijarMultiplicador(5, 10, 5);
        CHECK(agentes::ve(g, ojo, haciaEste, {7.5f, 10.5f}, 6.0f, 110.0f));
    }
}

TEST_CASE("CP-042 Oido: cada pared cruzada resta 2 celdas de alcance", "[RF-042][HU-08]") {
    mundo::GrafoRejilla g(20, 5);
    const Vec2 ruido{2.5f, 2.5f};
    CHECK(agentes::oye(g, {7.5f, 2.5f}, ruido, 6.0f));        // 5 celdas, sin paredes
    CHECK_FALSE(agentes::oye(g, {9.5f, 2.5f}, ruido, 6.0f));  // 7 celdas > 6
    g.bloquear(5, 2);
    CHECK_FALSE(agentes::oye(g, {7.5f, 2.5f}, ruido, 6.0f));  // 5 > 6 - 2
    CHECK(agentes::oye(g, {6.5f, 2.5f}, ruido, 6.0f));        // 4 <= 6 - 2
}
