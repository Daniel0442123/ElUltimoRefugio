#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "dominio/juego/Partida.hpp"

using namespace refugio;
using juego::EstadoPartida;
using juego::ModoMovimiento;
using juego::MotivoFin;
using juego::Partida;
using E = agentes::EstadoInfectado;

namespace {
/// Mara junto a la radio con los tres objetos ya en el inventario (recogidos en el primer paso).
juego::Escenario conObjetos(mundo::Celda extraccion) {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(30, 10);
    e.inicioMara = {2.5f, 5.5f};
    e.radio = {3, 5};
    e.extraccion = extraccion;
    e.objetos = {{juego::TipoObjeto::Medicamento, {2, 4}, false},
                 {juego::TipoObjeto::Combustible, {2, 6}, false},
                 {juego::TipoObjeto::Bateria, {1, 5}, false}};
    return e;
}
void activarRadio(Partida& p) {
    for (int i = 0; i < 3; ++i) p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
    REQUIRE(p.tieneTodosLosObjetos());
    for (int i = 0; i < 8 * 60 + 2 && p.activacionesRadio() == 0; ++i) p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
    REQUIRE(p.activacionesRadio() == 1);
}
}  // namespace

TEST_CASE("CP-009 La zona de extraccion sigue abierta a 59 s y esta cerrada a 61 s", "[RF-009][HU-04]") {
    Partida p(conObjetos({25, 5}));
    activarRadio(p);
    CHECK(p.zonaAbierta());
    CHECK(p.restanteExtraccion() == Catch::Approx(60.0f).margin(0.05f));
    for (int i = 0; i < 59 * 60; ++i) p.paso({});
    CHECK(p.zonaAbierta());
    CHECK(p.restanteExtraccion() == Catch::Approx(1.0f).margin(0.05f));
    for (int i = 0; i < 2 * 60; ++i) p.paso({});
    CHECK_FALSE(p.zonaAbierta());
    CHECK(p.estado() == EstadoPartida::EnCurso);  // llegar tarde no termina la partida

    // Con la zona cerrada, entrar en ella no gana; se puede volver a activar la radio.
    for (int i = 0; i < 8 * 60 + 2 && p.activacionesRadio() == 1; ++i) p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
    CHECK(p.activacionesRadio() == 2);
    CHECK(p.zonaAbierta());
}

TEST_CASE("CP-010 Llegar a la zona abierta es la victoria; cerrada, la partida continua", "[RF-010][HU-04]") {
    SECTION("A tiempo") {
        Partida p(conObjetos({12, 5}));
        activarRadio(p);
        for (int i = 0; i < 10 * 60 && p.estado() == EstadoPartida::EnCurso; ++i)
            p.paso({1.0f, 0.0f, ModoMovimiento::Caminar, false});
        CHECK(p.estado() == EstadoPartida::Victoria);
        CHECK(p.motivoFin() == MotivoFin::Extraccion);
        CHECK(p.textoResultado() == "Victoria");
    }
    SECTION("Tarde") {
        Partida p(conObjetos({12, 5}));
        activarRadio(p);
        for (int i = 0; i < 61 * 60; ++i) p.paso({});
        for (int i = 0; i < 6 * 60; ++i) p.paso({1.0f, 0.0f, ModoMovimiento::Caminar, false});
        CHECK(p.estado() == EstadoPartida::EnCurso);
        CHECK(p.mara().pos.x > 12.0f);  // pasó por la zona cerrada
    }
}

TEST_CASE("CP-055 Un infectado en Atacar quita 10 de salud por segundo", "[RF-055][HU-05]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(10, 5);
    e.inicioMara = {5.3f, 2.5f};
    e.radio = {9, 4};
    e.extraccion = {0, 0};
    e.infectados.push_back({4.6f, 2.5f});
    Partida p(std::move(e));
    for (int i = 0; i < 60 && p.infectados()[0].estado != E::Atacar; ++i) p.paso({});
    REQUIRE(p.infectados()[0].estado == E::Atacar);
    const float antes = p.mara().salud;
    for (int i = 0; i < 60; ++i) p.paso({});
    CHECK(antes - p.mara().salud == Catch::Approx(10.0f).margin(0.01f));
}

TEST_CASE("CP-063 Un infectado golpeando la puerta 2 s quita 10 de integridad", "[RF-063][HU-05]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(30, 10);
    e.inicioMara = {28.5f, 8.5f};  // lejos: nadie la ve
    e.radio = {10, 5};
    e.extraccion = {28, 0};
    e.infectados.push_back({6.5f, 5.5f});
    Partida p(std::move(e));
    p.enviarAlerta(0, {10, 5}, 1.0f);  // lo mandan a investigar el refugio
    for (int i = 0; i < 5 * 60 && p.golpeandoPuerta() == 0; ++i) p.paso({});
    REQUIRE(p.golpeandoPuerta() == 1);
    const float antes = p.integridad();
    for (int i = 0; i < 2 * 60; ++i) p.paso({});
    CHECK(antes - p.integridad() == Catch::Approx(10.0f).margin(0.01f));
    CHECK(p.infectados()[0].golpeaPuerta);
}

TEST_CASE("CP-012 Con integridad 3 un golpe de la horda inutiliza el refugio", "[RF-012][HU-05]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(30, 10);
    e.inicioMara = {28.5f, 8.5f};
    e.radio = {10, 5};
    e.extraccion = {28, 0};
    e.infectados.push_back({6.5f, 5.5f});
    juego::ConfigPartida cfg;
    cfg.integridadInicial = 3.0f;
    Partida p(std::move(e), cfg);
    p.enviarAlerta(0, {10, 5}, 1.0f);
    for (int i = 0; i < 6 * 60 && p.estado() == EstadoPartida::EnCurso; ++i) p.paso({});
    CHECK(p.estado() == EstadoPartida::Derrota);
    CHECK(p.motivoFin() == MotivoFin::Refugio);
    CHECK(p.textoResultado() == "Derrota: refugio inutilizado");
    CHECK(p.integridad() == 0.0f);
}

TEST_CASE("CP-011 Con 5 de salud y un infectado atacando, en 1 s Mara cae", "[RF-011][RF-055][HU-05]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(10, 5);
    e.inicioMara = {5.3f, 2.5f};
    e.radio = {9, 4};
    e.extraccion = {0, 0};
    e.infectados.push_back({4.6f, 2.5f});
    Partida p(std::move(e));
    for (int i = 0; i < 60 && p.infectados()[0].estado != E::Atacar; ++i) p.paso({});
    REQUIRE(p.infectados()[0].estado == E::Atacar);
    // Llevar la salud a 5 con el propio ataque y medir el último segundo.
    while (p.mara().salud > 5.0f && p.estado() == EstadoPartida::EnCurso) p.paso({});
    for (int i = 0; i < 60 && p.estado() == EstadoPartida::EnCurso; ++i) p.paso({});
    CHECK(p.mara().salud == 0.0f);
    CHECK(p.textoResultado() == "Derrota: Mara ha caído");
}
