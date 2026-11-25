// Comparación Dijkstra / A* en vivo (HU-16): RF-020 a RF-024, RF-074.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

#include "contador_asignaciones.hpp"
#include "dominio/experimentos/GeneradorMapas.hpp"
#include "dominio/juego/Partida.hpp"
#include "dominio/navegacion/Comparacion.hpp"

using namespace refugio;
using juego::Partida;
using mundo::Celda;
using navegacion::Algoritmo;

namespace {
juego::Escenario abierto(int ancho, int alto, navegacion::Vec2 mara, std::vector<navegacion::Vec2> infectados) {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(ancho, alto);
    e.inicioMara = mara;
    e.radio = {ancho - 1, alto - 1};
    e.infectados = std::move(infectados);
    return e;
}

/// Muro con una abertura abajo: la ruta rodea y A* y Dijkstra expanden cantidades distintas.
juego::Escenario conMuro() {
    auto e = abierto(30, 14, {27.5f, 7.5f}, {{2.5f, 7.5f}, {2.5f, 2.5f}});
    for (int y = 0; y < 12; ++y) e.mapa.bloquear(14, y);
    return e;
}

juego::ConfigPartida configPrueba() {
    juego::ConfigPartida c;
    c.visionAngulo = 360.0f;
    c.danioPorSegundo = 0.0f;
    return c;
}
}  // namespace

TEST_CASE("CP-074 Misma consulta (3,3)>(60,50) en 64x64: mismo costo y A* expande menos", "[RF-074][RF-082][HU-16]") {
    nucleo::Pcg32 rng(42, 1u);
    auto g = experimentos::generarMapa({64, 64, 0.25, 0.10}, rng);
    g.fijarMultiplicador(3, 3, 1);
    g.fijarMultiplicador(60, 50, 1);
    const auto c = navegacion::compararAlgoritmos(g, g.nodo(3, 3), g.nodo(60, 50));
    REQUIRE(c.valida);
    REQUIRE(c.dijkstra.encontrada);
    REQUIRE(c.aEstrella.encontrada);
    CHECK(c.dijkstra.algoritmo == Algoritmo::Dijkstra);
    CHECK(c.aEstrella.algoritmo == Algoritmo::AEstrella);
    CHECK(std::abs(c.dijkstra.costo - c.aEstrella.costo) < 1e-6);
    CHECK(c.mismoCosto());
    CHECK(c.diferenciaCosto() < navegacion::kToleranciaCosto);
    CHECK(c.aEstrella.expandidos <= c.dijkstra.expandidos);
    CHECK(c.aEstrellaNoExpandeMas());
    CHECK(c.ahorroExpandidos() > 0.0);
    CHECK(c.ahorroExpandidos() < 1.0);
    // La tabla del panel tiene costo, expandidos, tiempo y memoria de cada uno (RF-074).
    for (const auto* r : {&c.dijkstra, &c.aEstrella}) {
        CHECK(r->expandidos > 0);
        CHECK(r->tiempoUs >= 0.0);
        CHECK(r->memoriaBytes > 0);
        CHECK(r->ruta.front() == g.nodo(3, 3));
        CHECK(r->ruta.back() == g.nodo(60, 50));
    }
}

TEST_CASE("CP-074 La comparacion detecta costos distintos y consultas sin ruta", "[RF-074][RF-082]") {
    navegacion::ComparacionBusqueda c;
    c.dijkstra.encontrada = c.aEstrella.encontrada = true;
    c.dijkstra.costo = 10.0;
    c.aEstrella.costo = 10.0 + 5e-7;
    CHECK(c.mismoCosto());
    c.aEstrella.costo = 11.0;
    CHECK_FALSE(c.mismoCosto());
    CHECK(c.diferenciaCosto() == Catch::Approx(1.0));
    c.aEstrella.encontrada = false;  // uno encuentra ruta y el otro no: error de optimalidad
    CHECK_FALSE(c.mismoCosto());
    c.dijkstra.encontrada = false;  // ninguno: coinciden
    CHECK(c.mismoCosto());
    c.dijkstra.expandidos = 100;
    c.aEstrella.expandidos = 25;
    CHECK(c.ahorroExpandidos() == Catch::Approx(0.75));
    c.aEstrella.expandidos = 120;
    CHECK(c.ahorroExpandidos() == 0.0);
    CHECK_FALSE(c.aEstrellaNoExpandeMas());
}

TEST_CASE("CP-074 F2 alterna el algoritmo de todos los agentes", "[RF-074][RF-024][HU-16]") {
    Partida p(conMuro(), configPrueba(), 3);
    CHECK(p.algoritmo() == Algoritmo::AEstrella);
    CHECK(p.alternarAlgoritmo() == Algoritmo::Dijkstra);
    CHECK(p.algoritmo() == Algoritmo::Dijkstra);
    CHECK(p.config().algoritmo == Algoritmo::Dijkstra);

    // Los dos agentes planifican ahora con Dijkstra y obtienen el costo óptimo de A*.
    p.enviarAlerta(0, {24, 7}, 0.9f);
    p.enviarAlerta(1, {24, 3}, 0.9f);
    p.paso({});
    for (std::size_t i = 0; i < 2; ++i) {
        const auto& m = p.infectados()[i].busqueda;
        REQUIRE(m.valida);
        CHECK(m.algoritmo == Algoritmo::Dijkstra);
        navegacion::BuscadorMejorPrimero a(Algoritmo::AEstrella);
        const auto ref = a.buscar(p.mapa(), p.mapa().nodo(m.origen.x, m.origen.y), p.mapa().nodo(m.destino.x, m.destino.y));
        CHECK(std::abs(m.costo - ref.costo) < 1e-6);
        CHECK(m.expandidos >= ref.expandidos);
    }

    // Segunda pulsación: vuelve a A*. La ConfigPartida también puede fijarlo desde el inicio.
    CHECK(p.alternarAlgoritmo() == Algoritmo::AEstrella);
    juego::ConfigPartida cfg = configPrueba();
    cfg.algoritmo = Algoritmo::Dijkstra;
    Partida q(conMuro(), cfg, 3);
    CHECK(q.algoritmo() == Algoritmo::Dijkstra);
}

TEST_CASE("CP-074 F3 compara la ultima consulta del agente seleccionado", "[RF-074][RF-023][HU-16]") {
    Partida p(conMuro(), configPrueba(), 3);
    CHECK_FALSE(p.compararSeleccionado());  // sin agente seleccionado
    CHECK_FALSE(p.comparacion().valida);
    p.seleccionar(0);
    CHECK_FALSE(p.compararSeleccionado());  // aún no ha planificado
    CHECK(p.agenteComparado() == -1);

    p.enviarAlerta(0, {24, 7}, 0.9f);
    p.paso({});
    const auto& m = p.infectados()[0].busqueda;
    REQUIRE(m.valida);
    REQUIRE(p.compararSeleccionado());
    const auto& c = p.comparacion();
    CHECK(c.valida);
    CHECK(p.agenteComparado() == 0);
    CHECK(p.tickComparacion() == p.tick());
    CHECK(p.mapa().celda(c.origen) == m.origen);
    CHECK(p.mapa().celda(c.destino) == m.destino);
    CHECK(c.mismoCosto());
    CHECK(c.aEstrella.costo == Catch::Approx(m.costo));
    CHECK(c.aEstrella.expandidos == m.expandidos);  // A* activo: la misma búsqueda que hizo el agente
    CHECK(c.aEstrella.expandidos < c.dijkstra.expandidos);
    // La comparación queda en el registro de IA (consola del panel y logs/ia_<semilla>.log).
    const std::string ultima(p.registroIa().ultima(0).vista());
    CHECK(ultima.find("comparar=(") != std::string::npos);
    CHECK(ultima.find("mismo_costo=si") != std::string::npos);

    p.descartarComparacion();
    CHECK_FALSE(p.comparacion().valida);
    CHECK(p.agenteComparado() == -1);
}

TEST_CASE("CP-074 Comparar con F3 no cambia la simulacion", "[RF-074][RNF-020]") {
    auto jugar = [](bool comparar) {
        Partida p(juego::crearEscenarioAleatorio(32, 22, 10, 21), {}, 21);
        for (int i = 0; i < 20 * 60; ++i) {
            if (comparar && i % 150 == 0) {
                p.seleccionar((i / 150) % 10);
                p.compararSeleccionado();
            }
            const float a = static_cast<float>(i) * 0.013f;
            p.paso({std::cos(a), std::sin(a), juego::ModoMovimiento::Correr, false});
        }
        return p;
    };
    const auto a = jugar(false);
    const auto b = jugar(true);
    for (std::size_t i = 0; i < a.infectados().size(); ++i) {
        CHECK(a.infectados()[i].pos.x == b.infectados()[i].pos.x);
        CHECK(a.infectados()[i].pos.y == b.infectados()[i].pos.y);
        CHECK(a.infectados()[i].estado == b.infectados()[i].estado);
    }
    CHECK(a.totalTransiciones() == b.totalTransiciones());
    CHECK(a.totalCambiosPlan() == b.totalCambiosPlan());
}

TEST_CASE("CP-106 La partida con Dijkstra es determinista y no asigna memoria", "[RNF-006][RNF-020][RF-024]") {
    auto crear = [] {
        juego::ConfigPartida cfg;
        cfg.algoritmo = Algoritmo::Dijkstra;
        return Partida(juego::crearEscenarioAleatorio(32, 22, 10, 5), cfg, 5);
    };
    auto entrada = [](int i) {
        const float a = static_cast<float>(i) * 0.017f;
        return juego::EntradaJugador{std::cos(a), std::sin(a), juego::ModoMovimiento::Correr, false};
    };
    Partida a = crear();
    Partida b = crear();
    a.seleccionar(1);
    for (int i = 0; i < 120; ++i) {
        a.paso(entrada(i));
        b.paso(entrada(i));
    }
    std::uint64_t asignaciones = 0;
    {
        pruebas::VentanaAsignaciones v;
        for (int i = 120; i < 120 + 30 * 60 && a.estado() == juego::EstadoPartida::EnCurso; ++i) a.paso(entrada(i));
        asignaciones = v.asignaciones();
    }
    for (int i = 120; i < 120 + 30 * 60 && b.estado() == juego::EstadoPartida::EnCurso; ++i) b.paso(entrada(i));
    CHECK(asignaciones == 0);
    CHECK(a.busquedasTotales() > 0);
    CHECK(a.tick() == b.tick());
    for (std::size_t i = 0; i < a.infectados().size(); ++i) {
        CHECK(a.infectados()[i].pos.x == b.infectados()[i].pos.x);
        CHECK(a.infectados()[i].pos.y == b.infectados()[i].pos.y);
        if (a.infectados()[i].busqueda.valida) CHECK(a.infectados()[i].busqueda.algoritmo == Algoritmo::Dijkstra);
    }
}
