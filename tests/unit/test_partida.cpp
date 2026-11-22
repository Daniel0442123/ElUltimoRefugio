#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

#include "contador_asignaciones.hpp"
#include "dominio/juego/Partida.hpp"

using namespace refugio;
using juego::EntradaJugador;
using juego::EstadoPartida;
using juego::ModoMovimiento;
using juego::Partida;
using E = agentes::EstadoInfectado;

namespace {
juego::Escenario abierto(int ancho, int alto, navegacion::Vec2 mara) {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(ancho, alto);
    e.inicioMara = mara;
    e.radio = {ancho - 1, alto - 1};
    return e;
}

bool transicionRegistrada(const Partida& p, E desde, E hacia) {
    for (std::size_t i = 0; i < p.numTransiciones(); ++i) {
        const auto& t = p.ultimaTransicion(i);
        if (t.desde == desde && t.hacia == hacia) return true;
    }
    return false;
}
}  // namespace

TEST_CASE("CP-002 Velocidad de Mara por modo", "[RF-002][HU-01]") {
    const struct {
        ModoMovimiento modo;
        float esperado;
    } casos[] = {{ModoMovimiento::Sigilo, 1.5f}, {ModoMovimiento::Caminar, 3.0f}, {ModoMovimiento::Correr, 6.0f}};
    for (const auto& c : casos) {
        Partida p(abierto(30, 5, {2.5f, 2.5f}));
        for (int i = 0; i < 60; ++i) p.paso({1.0f, 0.0f, c.modo, false});
        CHECK(p.mara().pos.x - 2.5f == Catch::Approx(c.esperado).margin(0.1));
    }
}

TEST_CASE("CP-003 Ruido de Mara: 2 eventos por segundo con radio segun el modo", "[RF-003][HU-01]") {
    const struct {
        ModoMovimiento modo;
        float radio;
    } casos[] = {{ModoMovimiento::Sigilo, 1.0f}, {ModoMovimiento::Caminar, 3.0f}, {ModoMovimiento::Correr, 6.0f}};
    for (const auto& c : casos) {
        Partida p(abierto(30, 5, {2.5f, 2.5f}));
        int eventos = 0;
        for (int i = 0; i < 60; ++i) {
            const bool tickIa = p.tick() % 6 == 0;
            p.paso({1.0f, 0.0f, c.modo, false});
            if (tickIa && p.huboRuidoUltimoTickIa()) {
                ++eventos;
                CHECK(p.radioUltimoRuido() == c.radio);
            }
        }
        CHECK(eventos == 2);
    }
}

TEST_CASE("CP-004 Mara no atraviesa paredes y se desliza a lo largo de ellas", "[RF-004]") {
    auto e = abierto(12, 8, {3.5f, 3.5f});
    for (int y = 0; y < 8; ++y) e.mapa.bloquear(6, y);
    Partida p(std::move(e));
    for (int i = 0; i < 120; ++i) p.paso({1.0f, 0.3f, ModoMovimiento::Correr, false});
    CHECK(p.mara().pos.x + p.config().radioCuerpo <= 6.0f);
    CHECK(p.mara().pos.y > 3.5f);  // la componente vertical sigue avanzando
}

TEST_CASE("CP-005 Recoger un objeto solo a distancia 1", "[RF-005][HU-02]") {
    auto e = abierto(12, 5, {2.5f, 2.5f});
    e.objetos.push_back({juego::TipoObjeto::Medicamento, {3, 2}, false});  // a 1 celda
    e.objetos.push_back({juego::TipoObjeto::Bateria, {5, 2}, false});      // a 3 celdas
    Partida p(std::move(e));
    p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
    p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
    CHECK(p.objetos()[0].recogido);
    CHECK(p.mara().inventario[0]);
    CHECK_FALSE(p.objetos()[1].recogido);
    CHECK_FALSE(p.mara().inventario[2]);
}

TEST_CASE("CP-007 Activar la radio con los tres objetos manteniendo interactuar", "[RF-007][RF-010][HU-03]") {
    auto e = abierto(12, 5, {2.5f, 2.5f});
    e.radio = {3, 2};
    e.objetos = {{juego::TipoObjeto::Medicamento, {2, 1}, false},
                 {juego::TipoObjeto::Combustible, {2, 3}, false},
                 {juego::TipoObjeto::Bateria, {1, 2}, false}};
    Partida p(std::move(e));
    for (int i = 0; i < 3; ++i) p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
    REQUIRE(p.tieneTodosLosObjetos());
    for (int i = 0; i < 4 * 60; ++i) p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
    p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, false});  // soltar reinicia (CP-008)
    CHECK(p.mara().progresoInstalacion == 0.0f);
    for (int i = 0; i < 8 * 60 + 1; ++i) p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
    CHECK(p.estado() == EstadoPartida::Victoria);
}

TEST_CASE("CP-044 Un infectado ve a Mara, la persigue con A* y la ataca", "[RF-044][RF-040][HU-08]") {
    auto e = abierto(24, 10, {14.5f, 5.5f});
    e.infectados.push_back({6.5f, 5.5f});
    // Unos escombros en medio: la ruta de A* los rodea.
    for (int y = 3; y <= 7; ++y) e.mapa.fijarMultiplicador(10, y, 8);
    juego::Partida p(std::move(e));
    // La orientación inicial es aleatoria: si al principio no mira hacia Mara, explora hasta
    // verla o hasta oírla caminar.
    bool persiguio = false;
    bool ataco = false;
    for (int i = 0; i < 20 * 60 && p.estado() == EstadoPartida::EnCurso; ++i) {
        // Mara camina despacio arriba y abajo, casi sin desplazarse, y hace ruido de radio 3.
        p.paso({0.0f, i % 120 < 60 ? 0.2f : -0.2f, ModoMovimiento::Caminar, false});
        persiguio = persiguio || p.infectados()[0].estado == E::Perseguir;
        ataco = ataco || p.infectados()[0].estado == E::Atacar;
    }
    CHECK(persiguio);
    CHECK(ataco);
    CHECK(transicionRegistrada(p, E::Perseguir, E::Atacar));
    CHECK(p.mara().salud < p.config().saludMax);
}

TEST_CASE("CP-011 Derrota cuando la salud de Mara llega a 0", "[RF-011][HU-08]") {
    auto e = abierto(10, 5, {5.3f, 2.5f});
    e.infectados.push_back({4.6f, 2.5f});  // pegado a Mara: la nota aunque no la mire
    Partida p(std::move(e));
    for (int i = 0; i < 15 * 60; ++i) p.paso({});
    CHECK(p.estado() == EstadoPartida::Derrota);
    CHECK(p.mara().salud == 0.0f);
    const auto t = p.tick();
    p.paso({});
    CHECK(p.tick() == t);  // la partida terminada no avanza
}

TEST_CASE("CP-045 Al explorar, el infectado recorre el mapa sin salirse de las celdas libres", "[RF-045][RF-004]") {
    auto esc = juego::crearEscenarioAleatorio(32, 22, 6, 7);
    Partida p(std::move(esc));
    std::vector<navegacion::Vec2> inicio;
    for (const auto& z : p.infectados()) inicio.push_back(z.pos);
    for (int i = 0; i < 30 * 60; ++i) {
        p.paso({});
        for (const auto& z : p.infectados()) {
            const auto c = agentes::celdaDe(z.pos);
            REQUIRE(p.mapa().transitable(c.x, c.y));
        }
    }
    int seMovieron = 0;
    for (std::size_t i = 0; i < inicio.size(); ++i)
        if (std::hypot(p.infectados()[i].pos.x - inicio[i].x, p.infectados()[i].pos.y - inicio[i].y) > 2.0f) ++seMovieron;
    CHECK(seMovieron >= 4);
}

TEST_CASE("CP-086 La partida es determinista con la misma semilla y la misma entrada", "[RNF-020][RF-086]") {
    auto jugar = [](std::uint64_t semilla) {
        Partida p(juego::crearEscenarioAleatorio(32, 22, 8, semilla), {}, semilla);
        for (int i = 0; i < 20 * 60; ++i) {
            const float a = static_cast<float>(i) * 0.01f;
            p.paso({std::cos(a), std::sin(a), ModoMovimiento::Correr, false});
        }
        return p;
    };
    const auto a = jugar(11);
    const auto b = jugar(11);
    REQUIRE(a.infectados().size() == b.infectados().size());
    for (std::size_t i = 0; i < a.infectados().size(); ++i) {
        CHECK(a.infectados()[i].pos.x == b.infectados()[i].pos.x);
        CHECK(a.infectados()[i].pos.y == b.infectados()[i].pos.y);
        CHECK(a.infectados()[i].estado == b.infectados()[i].estado);
    }
    CHECK(a.totalTransiciones() == b.totalTransiciones());
    CHECK(a.mara().salud == b.mara().salud);
}

TEST_CASE("CP-106 El paso de simulacion no asigna memoria tras la carga", "[RNF-006]") {
    Partida p(juego::crearEscenarioAleatorio(32, 22, 10, 3), {}, 3);
    auto entrada = [](int i) {
        const float a = static_cast<float>(i) * 0.02f;
        return EntradaJugador{std::cos(a), std::sin(a), i % 300 < 150 ? ModoMovimiento::Correr : ModoMovimiento::Caminar,
                              (i % 50) == 0};
    };
    // Calentamiento: el quadtree y los vectores alcanzan su capacidad de trabajo.
    for (int i = 0; i < 120; ++i) p.paso(entrada(i));
    std::uint64_t asignaciones = 0;
    std::uint64_t busquedas = 0;
    {
        refugio::pruebas::VentanaAsignaciones ventana;
        for (int i = 120; i < 120 + 60 * 60 && p.estado() == EstadoPartida::EnCurso; ++i) {
            p.paso(entrada(i));
            busquedas += p.busquedasUltimoTickIa();
        }
        asignaciones = ventana.asignaciones();
    }
    CHECK(busquedas > 0);  // hubo planificación con A* dentro de la ventana
    CHECK(asignaciones == 0);
}
