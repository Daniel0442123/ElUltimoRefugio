// Tutorial de sigilo (HU-20): RF-017, RNF-011.
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "dominio/juego/Partida.hpp"
#include "dominio/juego/Tutorial.hpp"

using namespace refugio;
using juego::ModoMovimiento;
using juego::PasoTutorial;

namespace {

/// Calle vacía de 40x5 con Mara en (2,2) y un botiquín en (30,2); sin infectados.
juego::Escenario escenario() {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(40, 5);
    e.inicioMara = {2.5f, 2.5f};
    e.objetos = {{juego::TipoObjeto::Medicamento, {30, 2}, false}};
    e.radio = {38, 4};
    return e;
}

void andar(juego::Partida& p, juego::Tutorial& t, ModoMovimiento modo, float ejeX, int ticks, bool interactuar = false) {
    juego::EntradaJugador e;
    e.ejeX = ejeX;
    e.modo = modo;
    e.interactuar = interactuar;
    for (int i = 0; i < ticks; ++i) {
        p.paso(e);
        t.observar(p);
    }
}

}  // namespace

TEST_CASE("CP-017 las indicaciones salen en orden y se retiran al hacer la accion", "[tutorial][CP-017]") {
    juego::Partida p(escenario());
    juego::Tutorial t(true);
    CHECK(t.activo());
    CHECK(t.paso() == PasoTutorial::Mover);

    andar(p, t, ModoMovimiento::Caminar, 0.0f, 30);  // quieta: no avanza
    CHECK(t.paso() == PasoTutorial::Mover);
    CHECK(t.progreso() == 0.0f);

    andar(p, t, ModoMovimiento::Caminar, 1.0f, 50);  // 3 celdas/s * 0,83 s = 2,5 celdas
    CHECK(t.paso() == PasoTutorial::Sigilo);
    CHECK(t.completados() == 1);

    // Correr antes de tiempo no cuenta para el paso de correr: primero hay que hacer el sigilo.
    andar(p, t, ModoMovimiento::Correr, 1.0f, 40);
    CHECK(t.paso() == PasoTutorial::Sigilo);
    andar(p, t, ModoMovimiento::Sigilo, 1.0f, 30);  // 1,5 celdas/s * 0,5 s = 0,75
    CHECK(t.paso() == PasoTutorial::Sigilo);
    CHECK(t.progreso() > 0.4f);
    andar(p, t, ModoMovimiento::Sigilo, 1.0f, 40);
    CHECK(t.paso() == PasoTutorial::Correr);

    andar(p, t, ModoMovimiento::Correr, 1.0f, 35);  // 6 celdas/s * 0,58 s = 3,5
    CHECK(t.paso() == PasoTutorial::Recoger);

    // Ir hasta el botiquín y recogerlo con E.
    for (int i = 0; i < 600 && p.mara().pos.x < 29.8f; ++i) andar(p, t, ModoMovimiento::Correr, 1.0f, 1);
    CHECK(t.paso() == PasoTutorial::Recoger);
    andar(p, t, ModoMovimiento::Caminar, 0.0f, 1, true);
    CHECK(p.mara().inventario[0]);
    CHECK(t.paso() == PasoTutorial::Terminado);
    CHECK_FALSE(t.activo());
    CHECK(t.completados() == juego::kNumPasosTutorial);
    CHECK_FALSE(t.saltado());
}

TEST_CASE("CP-017 saltar el tutorial quita las indicaciones", "[tutorial][CP-017]") {
    juego::Partida p(escenario());
    juego::Tutorial t(true);
    andar(p, t, ModoMovimiento::Caminar, 1.0f, 50);
    REQUIRE(t.paso() == PasoTutorial::Sigilo);
    t.saltar();
    CHECK_FALSE(t.activo());
    CHECK(t.saltado());
    CHECK(t.completados() == 1);
    andar(p, t, ModoMovimiento::Sigilo, 1.0f, 120);  // ya no vuelve a salir
    CHECK(t.paso() == PasoTutorial::Terminado);
}

TEST_CASE("RF-017 con el tutorial desactivado no hay indicaciones", "[tutorial][RF-017]") {
    juego::Partida p(escenario());
    juego::Tutorial t(false);
    CHECK_FALSE(t.activo());
    andar(p, t, ModoMovimiento::Caminar, 1.0f, 120);
    CHECK(t.paso() == PasoTutorial::Terminado);
    CHECK(t.completados() == 0);
}

TEST_CASE("RNF-011 el tutorial mide su duracion y no cambia la simulacion", "[tutorial][RNF-011]") {
    juego::Partida conTutorial(escenario());
    juego::Partida sinTutorial(escenario());
    juego::Tutorial t(true);
    juego::Tutorial nada(false);
    andar(conTutorial, t, ModoMovimiento::Caminar, 1.0f, 60);
    andar(sinTutorial, nada, ModoMovimiento::Caminar, 1.0f, 60);
    CHECK(conTutorial.mara().pos.x == sinTutorial.mara().pos.x);
    CHECK(std::fabs(t.segundos() - 1.0) < 1e-6);  // 60 pasos a 60 Hz
    t.saltar();
    andar(conTutorial, t, ModoMovimiento::Caminar, 1.0f, 60);
    CHECK(std::fabs(t.segundos() - 1.0) < 1e-6);  // congelado al saltar
}
