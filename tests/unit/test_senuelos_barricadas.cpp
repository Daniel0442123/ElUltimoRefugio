#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

#include "contador_asignaciones.hpp"
#include "dominio/agentes/Memoria.hpp"
#include "dominio/juego/Partida.hpp"

using namespace refugio;
using agentes::Evidencia;
using agentes::TipoEvidencia;
using juego::Partida;
using E = agentes::EstadoInfectado;
using mundo::Celda;

namespace {
juego::Escenario abierto(int ancho, int alto, navegacion::Vec2 mara, std::vector<navegacion::Vec2> infectados) {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(ancho, alto);
    e.inicioMara = mara;
    e.radio = {ancho - 1, alto - 1};
    e.infectados = std::move(infectados);
    return e;
}
juego::ConfigPartida sinDanio() {
    juego::ConfigPartida c;
    c.danioPorSegundo = 0.0f;
    return c;
}
juego::EntradaJugador lanzar(float x, float y) {
    juego::EntradaJugador e;
    e.lanzarSenuelo = true;
    e.apuntar = {x, y};
    return e;
}
bool rutaPasaPor(const Partida& p, const juego::Infectado& z, Celda c) {
    for (std::size_t k = z.indiceRuta; k < z.ruta.size(); ++k)
        if (p.mapa().celda(z.ruta[k]) == c) return true;
    return false;
}
int cambiosDe(const Partida& p, juego::MotivoPlan m) {
    int n = 0;
    for (std::size_t i = 0; i < p.numCambiosPlan(); ++i)
        if (p.ultimoCambioPlan(i).motivo == m) ++n;
    return n;
}
}  // namespace

// ------------------------------------------------------------------------------- RF-013
TEST_CASE("CP-013 Senuelo a 6 y a 12 celdas: cae a 6 y a 8, y suena 10 veces con radio 8", "[RF-013][HU-06]") {
    Partida p(abierto(30, 20, {5.5f, 10.5f}, {}), sinDanio(), 1);
    REQUIRE(p.mara().senuelos == 2);
    // Ruidos de radio 8 que llegan a los ticks de IA (justo después de cada uno).
    int ruidos8 = 0;
    auto paso = [&](const juego::EntradaJugador& e) {
        p.paso(e);
        if (p.tick() % 6 == 1)
            for (const auto& r : p.ruidosUltimoTickIa())
                if (r.senuelo && r.radio == 8.0f) ++ruidos8;
    };
    paso(lanzar(11.5f, 10.5f));
    CHECK(p.senuelos()[0].activo);
    CHECK(p.senuelos()[0].celda == Celda{11, 10});
    CHECK(p.mara().senuelos == 1);
    paso(lanzar(17.5f, 10.5f));  // a 12: se queda en el alcance de 8
    CHECK(p.senuelos()[1].celda == Celda{13, 10});
    CHECK(p.mara().senuelos == 0);
    paso(lanzar(17.5f, 10.5f));  // sin señuelos no pasa nada
    CHECK_FALSE(p.senuelos()[2].activo);

    // Cada señuelo: 10 ruidos de radio 8, uno por segundo.
    for (int i = 0; i < 12 * 60; ++i) paso({});
    CHECK(p.ruidosSenuelo() == 20);
    CHECK(ruidos8 == 20);
    CHECK_FALSE(p.senuelos()[0].activo);
    CHECK_FALSE(p.senuelos()[1].activo);
}

TEST_CASE("CP-013 El senuelo no atraviesa paredes: cae en la ultima celda libre", "[RF-013]") {
    auto e = abierto(30, 20, {5.5f, 10.5f}, {});
    for (int y = 0; y < 20; ++y) e.mapa.bloquear(9, y);
    Partida p(std::move(e), sinDanio(), 1);
    p.paso(lanzar(11.5f, 10.5f));
    CHECK(p.senuelos()[0].celda == Celda{8, 10});
}

// ------------------------------------------------------------------------------- RF-014
TEST_CASE("CP-014 Barricada en celda libre adyacente; la ocupada se rechaza", "[RF-014][HU-07]") {
    Partida p(abierto(30, 20, {5.5f, 10.5f}, {{5.5f, 12.5f}}), sinDanio(), 1);
    REQUIRE(p.mara().barricadas == 3);
    juego::EntradaJugador e;
    e.colocarBarricada = true;
    e.apuntar = {9.0f, 10.5f};  // hacia el este: la celda (6,10)
    p.paso(e);
    CHECK_FALSE(p.mapa().transitable(6, 10));
    CHECK(p.cambiosMapa() == 1);  // CambioDeMapa
    CHECK(p.mara().barricadas == 2);
    CHECK(p.barricadas()[0].activa);
    CHECK(p.barricadas()[0].durabilidad == 100.0f);

    // Celda donde hay un infectado: no se coloca.
    const Celda ocupada = agentes::celdaDe(p.infectados()[0].pos);
    CHECK_FALSE(p.colocarBarricadaEn(ocupada));
    CHECK(p.mapa().transitable(ocupada.x, ocupada.y));
    // Ni sobre Mara ni sobre una pared.
    CHECK_FALSE(p.colocarBarricadaEn(agentes::celdaDe(p.mara().pos)));
    CHECK_FALSE(p.colocarBarricadaEn({6, 10}));
    CHECK(p.cambiosMapa() == 1);
}

// --------------------------------------------------------------------------- RF-025, RF-052
TEST_CASE("CP-025 Barricada en la ruta con rodeo barato: replanifica sin pasar por ella", "[RF-025][RF-052][HU-07]") {
    Partida p(abierto(40, 20, {38.5f, 18.5f}, {{5.5f, 10.5f}}), sinDanio(), 1);
    p.enviarAlerta(0, {30, 10}, 1.0f);
    for (int i = 0; i < 12; ++i) p.paso({});
    REQUIRE(rutaPasaPor(p, p.infectados()[0], {15, 10}));
    REQUIRE(p.colocarBarricadaEn({15, 10}));
    for (int i = 0; i < 4 * 60; ++i) p.paso({});
    const auto& z = p.infectados()[0];
    CHECK_FALSE(rutaPasaPor(p, z, {15, 10}));
    CHECK(z.estado != E::AtacarBarricada);
    CHECK(cambiosDe(p, juego::MotivoPlan::Rodeo) == 1);
    const auto& r = p.ultimoCambioPlan(0);
    CHECK(r.agente == 0);
    CHECK(r.valorNuevo <= 2.0f * r.valorAnterior);
}

TEST_CASE("CP-052 Barricada sin alternativa: AtacarBarricada baja 10 por segundo y al romperla sigue", "[RF-052][HU-11]") {
    auto e = abierto(40, 20, {38.5f, 18.5f}, {{5.5f, 10.5f}});
    for (int y = 0; y < 20; ++y)
        if (y != 10) e.mapa.bloquear(15, y);  // un muro con una sola abertura
    Partida p(std::move(e), sinDanio(), 1);
    p.enviarAlerta(0, {30, 10}, 1.0f);
    for (int i = 0; i < 12; ++i) p.paso({});
    REQUIRE(p.colocarBarricadaEn({15, 10}));

    int pasos = 0;
    while (pasos < 10 * 60 && !(p.infectados()[0].estado == E::AtacarBarricada && p.barricadas()[0].durabilidad < 100.0f)) {
        p.paso({});
        ++pasos;
    }
    REQUIRE(p.infectados()[0].estado == E::AtacarBarricada);
    CHECK(cambiosDe(p, juego::MotivoPlan::AtacarBarricada) == 1);
    const float antes = p.barricadas()[0].durabilidad;
    for (int i = 0; i < 60; ++i) p.paso({});
    CHECK(antes - p.barricadas()[0].durabilidad == Catch::Approx(10.0f).margin(0.2f));

    for (int i = 0; i < 12 * 60 && p.barricadas()[0].activa; ++i) p.paso({});
    CHECK_FALSE(p.barricadas()[0].activa);
    CHECK(p.mapa().transitable(15, 10));
    for (int i = 0; i < 8 * 60; ++i) p.paso({});
    CHECK(p.infectados()[0].pos.x > 16.0f);  // cruzó por donde estaba la barricada
}

TEST_CASE("CP-052 Rodeo de mas del doble: ataca la barricada en vez de rodear", "[RF-052]") {
    auto e = abierto(40, 20, {38.5f, 18.5f}, {{5.5f, 10.5f}});
    for (int y = 1; y < 20; ++y)
        if (y != 10) e.mapa.bloquear(15, y);  // abertura en y = 10 y otra lejos, en y = 0
    Partida p(std::move(e), sinDanio(), 1);
    p.enviarAlerta(0, {18, 10}, 1.0f);
    for (int i = 0; i < 12; ++i) p.paso({});
    REQUIRE(p.colocarBarricadaEn({15, 10}));
    for (int i = 0; i < 4 * 60 && p.infectados()[0].estado != E::AtacarBarricada; ++i) p.paso({});
    CHECK(p.infectados()[0].estado == E::AtacarBarricada);
    const auto& r = p.ultimoCambioPlan(0);
    CHECK(r.motivo == juego::MotivoPlan::AtacarBarricada);
    CHECK(r.valorNuevo > 2.0f * r.valorAnterior);  // el rodeo existía pero salía caro
}

// ------------------------------------------------------------------------------- RF-043
TEST_CASE("CP-043 Memoria: 20 evidencias dejan las 16 de mayor confianza y la confianza cae a la mitad en 10 s", "[RF-043][HU-08]") {
    agentes::MemoriaEvidencias m;
    for (int k = 0; k < 20; ++k)
        m.insertar({TipoEvidencia::Ruido, {k, 0}, 0.0, 0.05f * static_cast<float>(k + 1), 0}, 0.0);
    REQUIRE(m.tamano() == agentes::kMemoriaCapacidad);
    float minimo = 1.0f;
    for (std::size_t i = 0; i < m.tamano(); ++i) minimo = std::min(minimo, m[i].confianza);
    CHECK(minimo == Catch::Approx(0.25f));  // se fueron las 4 más débiles (0,05 a 0,20)
    for (std::size_t i = 0; i < m.tamano(); ++i)
        CHECK(agentes::confianzaActual(m[i], 10.0) == Catch::Approx(m[i].confianza * 0.5f));

    // Una evidencia más débil que todas no entra; la misma celda y tipo se refresca.
    CHECK_FALSE(m.insertar({TipoEvidencia::Ruido, {50, 0}, 0.0, 0.1f, 0}, 0.0));
    CHECK(m.insertar({TipoEvidencia::Ruido, {19, 0}, 5.0, 1.0f, 0}, 5.0));
    CHECK(m.tamano() == agentes::kMemoriaCapacidad);

    // Utilidad de RF-050: c = 0,3 de hace 20 s en la misma celda -> 0,3 · 1/4.
    const Evidencia vieja{TipoEvidencia::Alerta, {0, 0}, 0.0, 0.3f, 0};
    CHECK(agentes::utilidad(vieja, 20.0, 0.0f) == Catch::Approx(0.075f));
    CHECK(agentes::utilidad(vieja, 0.0, 10.0f) == Catch::Approx(0.15f));
}

// ------------------------------------------------------------------------------- RF-050
TEST_CASE("CP-050 Un senuelo cercano supera a una evidencia debil y cambia el objetivo en 5 ticks de IA", "[RF-050][RF-072][HU-11]") {
    Partida p(abierto(40, 20, {20.5f, 10.5f}, {{10.5f, 10.5f}}), sinDanio(), 1);
    p.enviarAlerta(0, {1, 1}, 0.3f);
    p.paso({});
    REQUIRE(p.infectados()[0].estado == E::Investigar);
    REQUIRE(p.infectados()[0].objetivo == Celda{1, 1});
    p.paso(lanzar(14.5f, 15.5f));
    const Celda senuelo = p.senuelos()[0].celda;
    int pasos = 0;
    while (pasos < 5 * 6 && !(p.infectados()[0].objetivo == senuelo)) {
        p.paso({});
        ++pasos;
    }
    CHECK(p.infectados()[0].objetivo == senuelo);
    CHECK(pasos <= 5 * 6);
    REQUIRE(p.totalCambiosPlan() == 1);
    const auto& r = p.ultimoCambioPlan(0);
    CHECK(r.motivo == juego::MotivoPlan::Senuelo);
    CHECK(r.anterior == Celda{1, 1});
    CHECK(r.nuevo == senuelo);
    CHECK(r.valorNuevo > r.valorAnterior);
}

TEST_CASE("CP-050 Con Mara a la vista un senuelo no le hace cambiar", "[RF-050]") {
    auto cfg = sinDanio();
    cfg.visionAngulo = 360.0f;
    Partida p(abierto(40, 20, {13.5f, 10.5f}, {{10.5f, 10.5f}}), cfg, 1);
    p.paso({});
    REQUIRE(p.infectados()[0].estado == E::Perseguir);
    p.paso(lanzar(10.5f, 16.5f));
    for (int i = 0; i < 60; ++i) {
        p.paso({});
        const auto est = p.infectados()[0].estado;
        CHECK((est == E::Perseguir || est == E::Atacar));
    }
    CHECK(cambiosDe(p, juego::MotivoPlan::Senuelo) == 0);
}

TEST_CASE("CP-072 Tres cambios de plan quedan registrados con tick, agente, plan anterior, nuevo y motivo", "[RF-072][HU-11]") {
    Partida p(abierto(40, 20, {20.5f, 10.5f}, {{10.5f, 9.5f}, {10.5f, 11.5f}, {9.5f, 10.5f}}), sinDanio(), 1);
    for (std::size_t k = 0; k < 3; ++k) p.enviarAlerta(k, {1, 1}, 0.3f);
    p.paso({});
    p.paso(lanzar(14.5f, 15.5f));
    for (int i = 0; i < 30; ++i) p.paso({});
    REQUIRE(p.totalCambiosPlan() == 3);
    std::vector<int> agentes;
    for (std::size_t i = 0; i < 3; ++i) {
        const auto& r = p.ultimoCambioPlan(i);
        CHECK(r.tick > 0);
        CHECK(r.anterior == Celda{1, 1});
        CHECK(r.nuevo == p.senuelos()[0].celda);
        CHECK(r.motivo == juego::MotivoPlan::Senuelo);
        CHECK(juego::nombreMotivoPlan(r.motivo) == "senuelo");
        agentes.push_back(r.agente);
    }
    std::sort(agentes.begin(), agentes.end());
    CHECK(agentes == std::vector<int>{0, 1, 2});
}

TEST_CASE("CP-106 Senuelos, barricadas y memoria no asignan memoria en el bucle", "[RNF-006][RF-013][RF-014][RF-043]") {
    auto e = abierto(40, 20, {20.5f, 10.5f}, {});
    for (int k = 0; k < 10; ++k) e.infectados.push_back({3.5f + static_cast<float>(k) * 3.0f, 3.5f});
    Partida p(std::move(e), sinDanio(), 5);
    for (int i = 0; i < 30; ++i) p.paso({});
    std::uint64_t asignaciones = 0;
    {
        refugio::pruebas::VentanaAsignaciones ventana;
        p.paso(lanzar(26.5f, 14.5f));
        juego::EntradaJugador b;
        b.colocarBarricada = true;
        b.apuntar = {20.5f, 12.5f};
        p.paso(b);
        for (int i = 0; i < 20 * 60; ++i) {
            const float a = static_cast<float>(i) * 0.02f;
            p.paso({std::cos(a), std::sin(a), juego::ModoMovimiento::Correr, false});
        }
        asignaciones = ventana.asignaciones();
    }
    CHECK(p.ruidosSenuelo() == 10);
    CHECK(p.cambiosMapa() >= 1);
    CHECK(asignaciones == 0);
}
