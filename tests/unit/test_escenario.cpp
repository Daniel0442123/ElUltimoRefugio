// Lectura visual del mapa para el arte pixel: clasificación de celdas, edificios, rayos de luz y
// decoración determinista. No usa raylib.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "adaptadores/vista/Escenario.hpp"
#include "dominio/juego/Partida.hpp"

using namespace refugio;
using vista::Suelo;

namespace {
// 6 x 4:  . . # # . .
//         . . # . . ~
//         . . . . . .
//         # . . . . #
mundo::GrafoRejilla mapaPrueba() {
    mundo::GrafoRejilla g(6, 4);
    g.bloquear(2, 0);
    g.bloquear(3, 0);
    g.bloquear(2, 1);
    g.bloquear(0, 3);
    g.bloquear(5, 3);
    g.fijarMultiplicador(5, 1, 4);
    return g;
}
}  // namespace

TEST_CASE("Escenario clasifica muros, aceras, calle y escombros", "[vista]") {
    const auto g = mapaPrueba();
    const auto e = vista::construirEscenario(g, nullptr, 0);
    CHECK(e.en(2, 0) == Suelo::Muro);
    CHECK(e.en(5, 1) == Suelo::Escombros);
    CHECK(e.en(1, 0) == Suelo::Acera);  // junto al edificio
    CHECK(e.en(3, 1) == Suelo::Acera);
    CHECK(e.en(4, 2) == Suelo::Calle);
    CHECK(e.en(0, 0) == Suelo::Calle);
    CHECK(e.muro(-1, 0));  // fuera del mapa cuenta como muro
}

TEST_CASE("Escenario agrupa los muros en edificios 4-conexos", "[vista]") {
    const auto e = vista::construirEscenario(mapaPrueba(), nullptr, 0);
    CHECK(e.numEdificios == 3);
    CHECK(e.edificio[0 * 6 + 2] == e.edificio[1 * 6 + 2]);
    CHECK(e.edificio[0 * 6 + 3] == e.edificio[0 * 6 + 2]);
    CHECK(e.edificio[3 * 6 + 0] != e.edificio[3 * 6 + 5]);
    CHECK(e.edificio[2 * 6 + 2] == 0);
}

TEST_CASE("Una barricada no se dibuja como edificio y conserva el suelo de debajo", "[vista]") {
    auto g = mapaPrueba();
    juego::Barricada b;
    b.celda = {5, 1};
    b.activa = true;
    b.multiplicadorPrevio = 4;
    const auto antes = vista::construirEscenario(g, &b, 1);
    g.bloquear(5, 1);  // así la deja la Partida al colocarla
    const auto e = vista::construirEscenario(g, &b, 1);
    CHECK(e.en(5, 1) == Suelo::Escombros);
    CHECK(e.firma == antes.firma);  // el fondo no se rehace al poner o romper barricadas
    b.activa = false;
    CHECK(vista::construirEscenario(g, &b, 1).en(5, 1) == Suelo::Muro);
}

TEST_CASE("El rayo de luz se detiene en el primer muro y respeta el alcance", "[vista]") {
    const auto e = vista::construirEscenario(mapaPrueba(), nullptr, 0);
    // Desde el centro de (0,1) hacia el este: el muro (2,1) empieza en x = 2.
    CHECK(vista::distanciaRayo(e, 0.5f, 1.5f, 1.0f, 0.0f, 10.0f) == Catch::Approx(1.5f));
    // Hacia el sur por la columna 1: sale del mapa en y = 4.
    CHECK(vista::distanciaRayo(e, 1.5f, 0.5f, 0.0f, 1.0f, 10.0f) == Catch::Approx(3.5f));
    // Alcance menor que la distancia al muro.
    CHECK(vista::distanciaRayo(e, 0.5f, 1.5f, 3.0f, 0.0f, 1.0f) == Catch::Approx(1.0f));
    // Diagonal desde (1,2) hacia el noreste: toca el muro (2,1) en la esquina compartida.
    CHECK(vista::distanciaRayo(e, 1.5f, 2.5f, 1.0f, -1.0f, 10.0f) == Catch::Approx(0.7071f).epsilon(1e-3));
    // Desde dentro de un muro no hay luz.
    CHECK(vista::distanciaRayo(e, 2.5f, 0.5f, 1.0f, 0.0f, 10.0f) == 0.0f);
}

TEST_CASE("La decoracion es determinista y no usa el generador de la partida", "[vista][ADR-0012]") {
    CHECK(vista::hashCelda(42, 3, 7) == vista::hashCelda(42, 3, 7));
    CHECK(vista::hashCelda(42, 3, 7) != vista::hashCelda(43, 3, 7));
    CHECK(vista::hashCelda(42, 3, 7) != vista::hashCelda(42, 7, 3));
    CHECK(vista::hashCelda(42, 3, 7, 1) != vista::hashCelda(42, 3, 7, 2));
}
