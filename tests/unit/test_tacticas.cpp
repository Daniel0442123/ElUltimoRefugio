#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "dominio/agentes/MaquinaEstados.hpp"
#include "dominio/agentes/Tactica.hpp"
#include "dominio/juego/Partida.hpp"

using namespace refugio;
using agentes::EntradaTactica;
using agentes::Tactica;
using agentes::TipoEvidencia;
using E = agentes::EstadoInfectado;
using M = agentes::MotivoTransicion;
using mundo::Celda;

namespace {
float puntos(const agentes::ResultadoTactica& r, Tactica t) { return r.puntuacion[static_cast<std::size_t>(t)]; }
}  // namespace

// ------------------------------------------------------------------------------- RF-054
TEST_CASE("CP-054 La tactica depende de la evidencia: los 4 ejemplos de HU-12", "[RF-054][HU-12]") {
    const agentes::ParametrosTactica par{};

    // Avistamiento cercano, edad 0 -> Perseguir.
    EntradaTactica cerca;
    cerca.hayEvidencia = true;
    cerca.tipo = TipoEvidencia::Avistamiento;
    cerca.confianza = 1.0f;
    cerca.distancia = 2.0f;
    cerca.tieneRumbo = true;
    cerca.velocidadMara = 3.0f;
    cerca.hayCorte = true;
    cerca.distanciaCorte = 3.0f;
    const auto r1 = agentes::elegirTactica(cerca, par);
    CHECK(r1.tactica == Tactica::Perseguir);
    CHECK(puntos(r1, Tactica::Perseguir) == Catch::Approx(1.0f / 1.2f));

    // Avistamiento con rumbo de hace 2 s -> Interceptar (Mara ya se movió unas 6 celdas).
    EntradaTactica rumbo = cerca;
    rumbo.edad = 2.0f;
    rumbo.confianza = agentes::confianzaActual({TipoEvidencia::Avistamiento, {}, 0.0, 1.0f, 0}, 2.0);
    rumbo.distancia = 4.0f;
    const auto r2 = agentes::elegirTactica(rumbo, par);
    CHECK(r2.tactica == Tactica::Interceptar);
    CHECK(puntos(r2, Tactica::Interceptar) > puntos(r2, Tactica::Perseguir));

    // Alerta de 3 saltos (c = 0,7³) de hace 15 s -> Explorar.
    EntradaTactica alerta;
    alerta.hayEvidencia = true;
    alerta.tipo = TipoEvidencia::Alerta;
    alerta.edad = 15.0f;
    alerta.confianza = agentes::confianzaActual({TipoEvidencia::Alerta, {}, 0.0, 0.343f, 0}, 15.0);
    alerta.distancia = 5.0f;
    const auto r3 = agentes::elegirTactica(alerta, par);
    CHECK(r3.tactica == Tactica::Explorar);
    CHECK(puntos(r3, Tactica::Interceptar) == 0.0f);  // sin rumbo no hay corte

    // Ninguna evidencia -> Explorar.
    const auto r4 = agentes::elegirTactica(EntradaTactica{}, par);
    CHECK(r4.tactica == Tactica::Explorar);
    CHECK(puntos(r4, Tactica::Explorar) == Catch::Approx(par.utilidadExplorar));
    CHECK(puntos(r4, Tactica::Perseguir) == 0.0f);
}

TEST_CASE("CP-054 La FSM sigue a la tactica elegida", "[RF-054][RF-044]") {
    const agentes::ParametrosFsm par{};
    agentes::PercepcionInfectado ve{true, 6.0f, false};
    agentes::PercepcionInfectado veInterceptar = ve;
    veInterceptar.tactica = Tactica::Interceptar;
    agentes::PercepcionInfectado debil{};
    debil.tactica = Tactica::Explorar;
    agentes::PercepcionInfectado noVeInterceptar{};
    noVeInterceptar.tactica = Tactica::Interceptar;
    const agentes::ContextoFsm nada{};
    auto t = agentes::decidirTransicion(E::Explorar, veInterceptar, nada, par);
    CHECK((t.hacia == E::Interceptar && t.motivo == M::EligeTactica));
    t = agentes::decidirTransicion(E::Perseguir, veInterceptar, nada, par);
    CHECK((t.hacia == E::Interceptar && t.motivo == M::EligeTactica));
    t = agentes::decidirTransicion(E::Perseguir, noVeInterceptar, nada, par);
    CHECK((t.hacia == E::Interceptar && t.motivo == M::EligeTactica));  // la pierde pero sabe adónde va
    t = agentes::decidirTransicion(E::Interceptar, veInterceptar, nada, par);
    CHECK(t.motivo == M::Ninguno);  // sigue cortándole el paso aunque la vea
    t = agentes::decidirTransicion(E::Interceptar, ve, nada, par);
    CHECK((t.hacia == E::Perseguir && t.motivo == M::VeAMara));
    t = agentes::decidirTransicion(E::Investigar, debil, nada, par);
    CHECK((t.hacia == E::Explorar && t.motivo == M::EvidenciaDebil));
    t = agentes::decidirTransicion(E::Perseguir, ve, nada, par);
    CHECK(t.motivo == M::Ninguno);  // con la táctica por defecto nada cambia
}

// ------------------------------------------------------------------------------- RF-053
TEST_CASE("CP-053 El punto de corte esta en la ruta prevista de Mara, 2 s o mas por delante", "[RF-053][HU-12]") {
    mundo::GrafoRejilla g(40, 20);
    for (int x = 0; x < 40; ++x) {  // calle este-oeste en y = 10 con un cruce norte-sur en x = 20
        for (int y = 0; y < 20; ++y)
            if (y != 10 && x != 20) g.bloquear(x, y);
    }
    const agentes::ParametrosTactica par{};
    const navegacion::Vec2 mara{5.5f, 10.5f};
    const navegacion::Vec2 vel{3.0f, 0.0f};
    // Perseguidor en el cruce, más cerca del cruce que de Mara.
    const navegacion::Vec2 agente{20.5f, 14.5f};
    const auto corte = agentes::puntoDeCorte(g, mara, vel, agente, 3.4f, par);
    REQUIRE(corte.has_value());
    CHECK(corte->celda.y == 10);  // sobre la calle por la que va Mara
    CHECK(corte->tiempoMara >= 2.0f);
    CHECK(corte->celda.x >= 5 + 6);  // al menos 2 s · 3 celdas/s por delante
    CHECK(corte->celda.x == 20);     // el más cercano al agente: el cruce
    CHECK(corte->distancia / 3.4f <= corte->tiempoMara);  // llega antes que ella

    // Quieta no tiene rumbo; con una pared delante la ruta prevista se corta.
    CHECK_FALSE(agentes::puntoDeCorte(g, mara, {0.0f, 0.0f}, agente, 3.4f, par).has_value());
    g.bloquear(8, 10);
    CHECK_FALSE(agentes::puntoDeCorte(g, mara, vel, agente, 3.4f, par).has_value());
}

TEST_CASE("CP-053 En la partida el infectado cerca del cruce elige interceptar y registra la tactica", "[RF-053][RF-054][RF-072][HU-12]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(40, 20);
    e.inicioMara = {5.5f, 10.5f};
    e.radio = {39, 19};
    e.infectados = {{16.5f, 15.5f}};
    juego::ConfigPartida cfg;
    cfg.visionAngulo = 360.0f;
    cfg.visionRadio = 12.0f;
    cfg.danioPorSegundo = 0.0f;
    juego::Partida p(std::move(e), cfg, 1);
    const juego::EntradaJugador este{1.0f, 0.0f, juego::ModoMovimiento::Caminar, false};
    bool intercepta = false;
    float maraAlDecidir = 0.0f;
    for (int i = 0; i < 2 * 60 && !intercepta; ++i) {
        p.paso(este);
        if (p.infectados()[0].estado == E::Interceptar) {
            intercepta = true;
            maraAlDecidir = p.mara().pos.x;
        }
    }
    REQUIRE(intercepta);
    const auto& z = p.infectados()[0];
    CHECK(z.tactica == Tactica::Interceptar);
    CHECK(z.objetivo == z.puntoCorte);
    CHECK(z.objetivo.y == 10);                                // en la calle de Mara
    CHECK(static_cast<float>(z.objetivo.x) + 0.5f >= maraAlDecidir + 2.0f * 3.0f - 1.0f);  // ~2 s por delante
    REQUIRE(p.totalCambiosPlan() >= 1);
    const auto& r = p.ultimoCambioPlan(0);
    CHECK(r.motivo == juego::MotivoPlan::TacticaInterceptar);
    CHECK(r.valorNuevo > r.valorAnterior);  // puntuación de Interceptar frente a la de Perseguir
    CHECK(juego::nombreMotivoPlan(r.motivo) == "tactica interceptar");
}
