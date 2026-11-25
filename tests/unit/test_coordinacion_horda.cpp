#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "contador_asignaciones.hpp"
#include "dominio/experimentos/GeneradorMapas.hpp"
#include "dominio/juego/Partida.hpp"
#include "dominio/navegacion/CampoFlujo.hpp"

using namespace refugio;
using juego::Partida;
using E = agentes::EstadoInfectado;
using M = agentes::MotivoTransicion;

namespace {
/// Infectados que ven en todas direcciones y no hacen daño: así la prueba controla quién ve a
/// Mara solo con paredes y distancias.
juego::ConfigPartida configPrueba() {
    juego::ConfigPartida c;
    c.visionAngulo = 360.0f;
    c.danioPorSegundo = 0.0f;
    return c;
}

bool recibioAlerta(const Partida& p, std::uint16_t agente) {
    for (std::size_t i = 0; i < p.numTransiciones(); ++i) {
        const auto& t = p.ultimaTransicion(i);
        if (t.agente == agente && t.hacia == E::Investigar && t.motivo == M::RecibeAlerta) return true;
    }
    return false;
}

/// Mara al este de un muro con una sola abertura abajo; A la ve, B y C están tras el muro en cadena.
juego::Escenario escenarioMuro() {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(30, 12);
    for (int y = 0; y < 11; ++y) e.mapa.bloquear(15, y);
    e.inicioMara = {20.5f, 5.5f};
    e.radio = {29, 11};
    e.infectados = {{17.5f, 5.5f}, {13.5f, 5.5f}, {9.5f, 5.5f}};  // A, B (a 4 de A), C (a 4 de B)
    return e;
}
}  // namespace

TEST_CASE("CP-028 El campo de flujo da el costo optimo desde cada celda", "[RF-028][RF-021]") {
    nucleo::Pcg32 rng(9, 1);
    const auto g = experimentos::generarMapa({32, 32, 0.25, 0.15}, rng);
    const auto pares = experimentos::generarPares(g, 3, rng);
    auto dijkstra = navegacion::crearBuscador(navegacion::Algoritmo::Dijkstra);
    navegacion::CampoFlujo campo;
    navegacion::ResultadoBusqueda r;
    for (const auto& par : pares) {
        const mundo::Nodo destino = par.second;
        REQUIRE(campo.calcular(g, destino));
        for (mundo::Nodo v = 0; v < g.numCeldas(); ++v) {
            if (!g.transitable(v)) continue;
            dijkstra->buscarEn(g, v, destino, r);
            REQUIRE(campo.alcanzable(v) == r.encontrada);
            if (!r.encontrada) continue;
            REQUIRE(campo.costo(v) == Catch::Approx(r.costo).margin(1e-9));
            // Seguir «siguiente» llega al destino pagando exactamente ese costo.
            double suma = 0.0;
            int pasos = 0;
            for (mundo::Nodo n = v; n != destino && pasos < 4096; ++pasos) {
                const mundo::Nodo s = campo.siguiente(n);
                double w = -1.0;
                g.paraCadaVecino(n, [&](mundo::Nodo u, double peso) {
                    if (u == s) w = peso;
                });
                REQUIRE(w > 0.0);  // siempre una arista real del grafo
                suma += w;
                n = s;
            }
            REQUIRE(suma == Catch::Approx(campo.costo(v)).margin(1e-9));
        }
    }
}

TEST_CASE("CP-028 Horda con el mismo destino: un campo de flujo y ningun A* individual", "[RF-028][HU-23]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(40, 20);
    // Escombros (no tapan la vista): una franja cara con dos pasos que el campo debe aprovechar.
    for (int y = 2; y < 18; ++y) e.mapa.fijarMultiplicador(20, y, 6);
    e.inicioMara = {33.5f, 8.5f};
    e.radio = {39, 19};
    for (int k = 0; k < 12; ++k) e.infectados.push_back({2.5f + static_cast<float>(k % 4) * 2.0f, 3.5f + static_cast<float>(k / 4) * 5.0f});
    auto cfg = configPrueba();
    cfg.visionRadio = 60.0f;  // todos la ven desde el inicio
    cfg.flowfieldUmbral = 8;
    cfg.coordinado = true;
    Partida p(std::move(e), cfg, 5);

    // Primeros 2 s: nadie ha llegado todavía.
    for (int i = 0; i < 2 * 60; ++i) p.paso({});
    CHECK(p.camposCalculados() >= 1);
    CHECK(p.agentesEnCampo() >= 8);
    CHECK(p.busquedasTotales() == 0);

    for (int i = 0; i < 30 * 60; ++i) p.paso({});
    int cerca = 0;
    for (const auto& z : p.infectados())
        if (std::hypot(z.pos.x - p.mara().pos.x, z.pos.y - p.mara().pos.y) <= 2.5f) ++cerca;
    CHECK(cerca == 12);  // todos llegan
}

TEST_CASE("CP-047 Quien ve a Mara alerta en cadena a los que estan tras el muro", "[RF-047][RF-046][HU-09]") {
    auto cfg = configPrueba();
    cfg.contractNet = false;  // solo la alerta; el reparto de tareas se prueba en CP-049
    Partida p(escenarioMuro(), cfg, 1);
    for (int i = 0; i < 12; ++i) p.paso({});  // dos ciclos de IA
    REQUIRE(p.alertasEmitidas() >= 1);
    const auto& prop = p.ultimaPropagacion();
    CHECK(prop.saltoPorAgente[0] == 0);
    CHECK(prop.saltoPorAgente[1] == 1);
    CHECK(prop.saltoPorAgente[2] == 2);
    CHECK(prop.mensajes.size() == 2);
    CHECK(prop.mensajes[1].confianza == Catch::Approx(0.49f));  // 1 · 0,7²
    CHECK(recibioAlerta(p, 1));
    CHECK(recibioAlerta(p, 2));
    CHECK(p.infectados()[1].estado == E::Investigar);
    CHECK(p.infectados()[1].objetivo == agentes::celdaDe(p.mara().pos));  // va hacia donde la vieron

    // B y C llegan rodeando el muro por la abertura (A* sobre el grafo).
    for (int i = 0; i < 15 * 60; ++i) p.paso({});
    for (std::size_t k = 1; k < 3; ++k) {
        const auto& z = p.infectados()[k];
        CHECK(std::hypot(z.pos.x - p.mara().pos.x, z.pos.y - p.mara().pos.y) <= 2.5f);
    }
}

TEST_CASE("CP-084 Sin modo coordinado nadie avisa y los de detras del muro siguen explorando", "[RF-047][RF-084]") {
    auto cfg = configPrueba();
    cfg.coordinado = false;
    Partida p(escenarioMuro(), cfg, 1);
    for (int i = 0; i < 60; ++i) p.paso({});
    CHECK(p.alertasEmitidas() == 0);
    CHECK_FALSE(recibioAlerta(p, 1));
    CHECK_FALSE(recibioAlerta(p, 2));
    CHECK(p.infectados()[0].estado != E::Explorar);  // A sí la ve
    CHECK(p.infectados()[2].estado == E::Explorar);
}

TEST_CASE("CP-106 Alertas y campo de flujo tampoco asignan memoria en el bucle", "[RNF-006][RF-028][RF-047]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(40, 20);
    for (int y = 0; y < 14; ++y) e.mapa.bloquear(26, y);
    e.inicioMara = {33.5f, 8.5f};
    e.radio = {39, 19};
    for (int k = 0; k < 12; ++k) e.infectados.push_back({2.5f + static_cast<float>(k % 4) * 2.0f, 3.5f + static_cast<float>(k / 4) * 5.0f});
    auto cfg = configPrueba();
    cfg.visionRadio = 60.0f;
    Partida p(std::move(e), cfg, 5);
    auto entrada = [](int i) {
        const float a = static_cast<float>(i) * 0.03f;
        return juego::EntradaJugador{std::cos(a), std::sin(a), juego::ModoMovimiento::Correr, false};
    };
    for (int i = 0; i < 30; ++i) p.paso(entrada(i));
    const auto camposAntes = p.camposCalculados();
    const auto alertasAntes = p.alertasEmitidas();
    std::uint64_t asignaciones = 0;
    {
        refugio::pruebas::VentanaAsignaciones ventana;
        for (int i = 30; i < 30 + 20 * 60; ++i) p.paso(entrada(i));
        asignaciones = ventana.asignaciones();
    }
    CHECK(p.camposCalculados() > camposAntes);  // Mara se mueve: el campo se recalcula
    CHECK(p.alertasEmitidas() > alertasAntes);
    CHECK(asignaciones == 0);
}
