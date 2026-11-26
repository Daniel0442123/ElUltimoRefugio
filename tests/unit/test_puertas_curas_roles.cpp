// Puerta metálica (RF-006), medicamento para curarse (RF-015) y roles de la horda (RF-048).
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "dominio/config/Configuracion.hpp"
#include "dominio/juego/Partida.hpp"

using namespace refugio;
using juego::EntradaJugador;
using juego::ModoMovimiento;
using juego::Partida;
using juego::RolInfectado;
using E = agentes::EstadoInfectado;

namespace {
juego::Escenario abierto(int ancho, int alto, navegacion::Vec2 mara) {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(ancho, alto);
    e.inicioMara = mara;
    e.radio = {ancho - 1, alto - 1};
    return e;
}

/// Muro vertical en x = @p x con una puerta metálica en (x, @p yPuerta).
juego::Escenario conPuerta(int x, int yPuerta) {
    auto e = abierto(16, 7, {static_cast<float>(x) - 0.5f, static_cast<float>(yPuerta) + 0.5f});
    for (int y = 0; y < 7; ++y) e.mapa.bloquear(x, y);
    e.puertas.push_back({x, yPuerta});
    return e;
}

EntradaJugador quieta(bool interactuar = false) {
    EntradaJugador e;
    e.interactuar = interactuar;
    return e;
}

std::string leerTexto(const std::filesystem::path& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
}  // namespace

// ----------------------------------------------------------------------------- RF-006
TEST_CASE("CP-006 Abrir una puerta metalica publica un Ruido de radio RUIDO_PUERTA_METALICA", "[RF-006][HU-02]") {
    Partida p(conPuerta(6, 3));
    REQUIRE(p.puertas().size() == 1);
    CHECK_FALSE(p.mapa().transitable(6, 3));  // cerrada bloquea su celda
    const auto cambios = p.cambiosMapa();

    p.paso(quieta(true));  // Mara está pegada a la puerta: interactuar la abre
    CHECK(p.puertas()[0].abierta);
    CHECK(p.puertasAbiertas() == 1);
    CHECK(p.mapa().transitable(6, 3));
    CHECK(p.cambiosMapa() == cambios + 1);  // CambioDeMapa (RF-025)

    // El ruido entra en el siguiente ciclo de IA (cada 6 pasos), con origen en la puerta.
    bool oido = false;
    for (int i = 0; i < 6 && !oido; ++i) {
        p.paso(quieta());
        for (const auto& r : p.ruidosUltimoTickIa())
            if (r.radio == p.config().ruidoPuertaMetalica && r.origen.x == 6.5f && r.origen.y == 3.5f && !r.senuelo) oido = true;
    }
    CHECK(oido);
    CHECK(p.config().ruidoPuertaMetalica == 10.0f);

    // Abierta se queda abierta y no vuelve a sonar; Mara pasa al otro lado.
    for (int i = 0; i < 60; ++i) p.paso({1.0f, 0.0f, ModoMovimiento::Sigilo, true});
    CHECK(p.puertasAbiertas() == 1);
    CHECK(p.mara().pos.x > 7.0f);
}

TEST_CASE("CP-006 La puerta solo se abre de cerca y los infectados la oyen", "[RF-006][RF-042]") {
    // Lejos (3 celdas) no se abre.
    auto lejos = conPuerta(6, 3);
    lejos.inicioMara = {3.5f, 3.5f};
    Partida p0(std::move(lejos));
    for (int i = 0; i < 30; ++i) p0.paso(quieta(true));
    CHECK_FALSE(p0.puertas()[0].abierta);
    CHECK_FALSE(p0.mapa().transitable(6, 3));

    // Un infectado a 8 celdas de la puerta, al otro lado del muro y de espaldas, la oye y va a investigar.
    auto e = conPuerta(6, 3);
    e.infectados.push_back({14.5f, 3.5f});
    Partida p(std::move(e));
    p.paso(quieta(true));
    REQUIRE(p.puertas()[0].abierta);
    bool investiga = false;
    for (int i = 0; i < 30 && !investiga; ++i) {
        p.paso(quieta());
        investiga = p.infectados()[0].estado == E::Investigar;
    }
    CHECK(investiga);
}

TEST_CASE("CP-006 La ciudad generada pone puertas metalicas cerradas en huecos de muro", "[RF-006]") {
    int conPuertas = 0;
    for (std::uint64_t semilla = 1; semilla <= 12; ++semilla) {
        const auto esc = juego::crearEscenarioAleatorio(32, 22, 10, semilla);
        CHECK(esc.puertas.size() <= 2);
        if (!esc.puertas.empty()) ++conPuertas;
        Partida p(esc, {}, semilla);
        REQUIRE(p.puertas().size() == esc.puertas.size());
        for (const auto& pu : p.puertas()) {
            CHECK_FALSE(pu.abierta);
            CHECK_FALSE(p.mapa().transitable(pu.celda.x, pu.celda.y));
            // Hueco de una celda: transitable a los dos lados en un eje y muro en el otro.
            const auto& g = p.mapa();
            const int x = pu.celda.x;
            const int y = pu.celda.y;
            const bool h = g.transitable(x - 1, y) && g.transitable(x + 1, y) && !g.transitable(x, y - 1) && !g.transitable(x, y + 1);
            const bool v = g.transitable(x, y - 1) && g.transitable(x, y + 1) && !g.transitable(x - 1, y) && !g.transitable(x + 1, y);
            CHECK((h || v));
        }
    }
    CHECK(conPuertas >= 6);  // la mayoría de las ciudades tiene alguna
}

// ----------------------------------------------------------------------------- RF-015
namespace {
/// Mara con @p medicamentos al alcance y un infectado pegado que la hiere a 30 puntos/s.
Partida conHerida(int medicamentos) {
    auto e = abierto(12, 5, {2.5f, 2.5f});
    for (int i = 0; i < medicamentos; ++i) e.objetos.push_back({juego::TipoObjeto::Medicamento, {2 + (i % 2), 2}, false});
    e.infectados.push_back({3.2f, 2.5f});
    juego::ConfigPartida cfg;
    cfg.danioPorSegundo = 30.0f;
    cfg.visionAngulo = 360.0f;
    return Partida(std::move(e), cfg);
}

void herirHasta(Partida& p, float salud) {
    for (int i = 0; i < 1200 && p.mara().salud > salud; ++i) p.paso(quieta(true));
}
}  // namespace

TEST_CASE("CP-015 Usar un medicamento suma CURA_MEDICAMENTO sin pasar de SALUD_MAX", "[RF-015][HU-02]") {
    auto p = conHerida(3);
    herirHasta(p, 50.0f);
    REQUIRE(p.mara().medicamentos == 3);
    REQUIRE(p.medicamentosUsables() == 2);
    const float antes = p.mara().salud;
    EntradaJugador usar = quieta();
    usar.usarMedicamento = true;
    p.paso(usar);
    // Cura (35) y en el mismo paso recibe el daño de 1/60 s.
    CHECK(p.mara().salud == Catch::Approx(antes + 35.0f - 30.0f / 60.0f).margin(0.01));
    CHECK(p.mara().medicamentos == 2);
    CHECK(p.medicamentosUsados() == 1);

    // Con unos 84 de salud, 84 + 35 se recorta a SALUD_MAX (y después llega el daño del paso).
    REQUIRE(p.mara().salud > p.config().saludMax - 35.0f);
    p.paso(usar);
    CHECK(p.mara().salud == Catch::Approx(p.config().saludMax - 30.0f / 60.0f).margin(0.01));
    CHECK(p.mara().medicamentos == 1);
}

TEST_CASE("CP-015 El medicamento reservado para la radio no se gasta", "[RF-015][RF-007]") {
    auto p = conHerida(1);
    herirHasta(p, 60.0f);
    REQUIRE(p.mara().medicamentos == 1);
    CHECK(p.medicamentosUsables() == 0);
    EntradaJugador usar = quieta();
    usar.usarMedicamento = true;
    const float antes = p.mara().salud;
    p.paso(usar);
    CHECK(p.mara().salud < antes);  // no se curó: solo recibió daño
    CHECK(p.mara().medicamentos == 1);
    CHECK(p.mara().inventario[static_cast<std::size_t>(juego::TipoObjeto::Medicamento)]);

    // Con la salud llena tampoco se gasta ninguno.
    auto e = abierto(12, 5, {2.5f, 2.5f});
    e.objetos.push_back({juego::TipoObjeto::Medicamento, {2, 2}, false});
    e.objetos.push_back({juego::TipoObjeto::Medicamento, {3, 2}, false});
    Partida llena(std::move(e));
    for (int i = 0; i < 4; ++i) llena.paso(quieta(true));
    REQUIRE(llena.mara().medicamentos == 2);
    llena.paso(usar);
    CHECK(llena.mara().medicamentos == 2);
    CHECK(llena.mara().salud == llena.config().saludMax);
}

TEST_CASE("CP-015 La ciudad generada tiene medicamentos de mas para curarse", "[RF-015]") {
    for (std::uint64_t semilla = 1; semilla <= 8; ++semilla) {
        const auto esc = juego::crearEscenarioAleatorio(32, 22, 10, semilla);
        const auto n = std::count_if(esc.objetos.begin(), esc.objetos.end(),
                                     [](const juego::ObjetoMision& o) { return o.tipo == juego::TipoObjeto::Medicamento; });
        CHECK(n == 3);  // el de la radio y dos para curarse
        CHECK(esc.objetos.size() == 5);
    }
}

// ----------------------------------------------------------------------------- RF-048
TEST_CASE("CP-048 Los prototipos de config/ia.json configuran los roles", "[RF-048][RF-092]") {
    const auto repo = config::leerParametrosIa(leerTexto(std::filesystem::path(REFUGIO_DIR_FUENTES) / "config" / "ia.json"));
    REQUIRE(repo.valida());
    const auto& p = repo.valor;
    CHECK(p.exploradorVisionRadio == 7.0f);
    CHECK(p.exploradorVisionAngulo == 110.0f);
    CHECK(p.exploradorVelocidad == 2.0f);
    CHECK(p.perseguidorVisionAngulo == 90.0f);
    // La sección «pesos» (RF-085) reemplaza el radio y la velocidad del perseguidor.
    CHECK(p.pesos.visionRadio == 6.0f);
    CHECK(p.pesos.velPerseguir == 3.4f);
    CHECK(p.curaMedicamento == 35.0f);
    CHECK(p.ruidoPuertaMetalica == 10.0f);

    // Sin «pesos», el perseguidor es el del prototipo (visión estrecha, 4 celdas/s).
    const auto solo = config::leerParametrosIa(R"({"prototipos": {
        "explorador": {"vision_radio": 7, "vision_angulo": 120, "velocidad": 2.0},
        "perseguidor": {"vision_radio": 5, "vision_angulo": 90, "velocidad": 4.0, "danio_por_segundo": 12}}})");
    REQUIRE(solo.valida());
    juego::ConfigPartida cfg;
    config::aplicar(solo.valor, cfg);
    CHECK(cfg.explorador.visionRadio == 7.0f);
    CHECK(cfg.explorador.visionAngulo == 120.0f);
    CHECK(cfg.explorador.velocidad == 2.0f);
    CHECK(cfg.visionRadio == 5.0f);
    CHECK(cfg.visionAngulo == 90.0f);
    CHECK(cfg.velPerseguir == 4.0f);
    CHECK(cfg.danioPorSegundo == 12.0f);

    // Un valor fuera de rango vuelve al de por defecto y se avisa.
    const auto mal = config::leerParametrosIa(R"({"prototipos": {"explorador": {"velocidad": -1}}})");
    CHECK_FALSE(mal.valida());
    CHECK(mal.valor.exploradorVelocidad == 2.0f);
}

TEST_CASE("CP-048 El explorador ve mas lejos y el perseguidor corre mas", "[RF-048][RF-041]") {
    // Mara quieta a 6,5 celdas de los dos (visión en 360° para comparar solo el radio).
    auto e = abierto(40, 5, {20.5f, 2.5f});
    e.infectados = {{14.0f, 2.5f}, {27.0f, 2.5f}};
    e.roles = {RolInfectado::Explorador, RolInfectado::Perseguidor};
    juego::ConfigPartida cfg;
    cfg.visionRadio = 5.0f;
    cfg.visionAngulo = 360.0f;
    cfg.explorador.visionAngulo = 360.0f;
    cfg.velPerseguir = 4.0f;
    cfg.coordinado = false;  // sin alertas: cada uno con lo que ve
    Partida p(std::move(e), cfg);
    REQUIRE(p.infectados()[0].rol == RolInfectado::Explorador);
    REQUIRE(p.infectados()[1].rol == RolInfectado::Perseguidor);
    p.paso(quieta());
    CHECK(p.infectados()[0].percibido.veAMara);
    CHECK_FALSE(p.infectados()[1].percibido.veAMara);
    CHECK(p.visionRadio(p.infectados()[0]) == 7.0f);
    CHECK(p.velPerseguir(p.infectados()[0]) == 2.0f);
    CHECK(p.velPerseguir(p.infectados()[1]) == 4.0f);

    // Velocidad al perseguir: los dos ven a Mara (radio 30) y se mide 1 s ya en marcha.
    auto e2 = abierto(60, 5, {30.5f, 2.5f});
    e2.infectados = {{5.5f, 1.5f}, {55.5f, 3.5f}};
    e2.roles = {RolInfectado::Explorador, RolInfectado::Perseguidor};
    cfg.visionRadio = 30.0f;
    cfg.explorador.visionRadio = 30.0f;
    Partida q(std::move(e2), cfg);
    for (int i = 0; i < 60; ++i) q.paso(quieta());
    REQUIRE(q.infectados()[0].estado == E::Perseguir);
    REQUIRE(q.infectados()[1].estado == E::Perseguir);
    const float x0 = q.infectados()[0].pos.x;
    const float x1 = q.infectados()[1].pos.x;
    for (int i = 0; i < 60; ++i) q.paso(quieta());
    CHECK(std::fabs(q.infectados()[0].pos.x - x0) == Catch::Approx(2.0f).margin(0.3));
    CHECK(std::fabs(q.infectados()[1].pos.x - x1) == Catch::Approx(4.0f).margin(0.4));
}

TEST_CASE("CP-048 La horda generada mezcla exploradores y perseguidores", "[RF-048][HU-08]") {
    const auto esc = juego::crearEscenarioAleatorio(32, 22, 10, 42);
    REQUIRE(esc.roles.size() == 10);
    Partida p(esc, {}, 42);
    int exploradores = 0;
    for (std::size_t i = 0; i < p.infectados().size(); ++i) {
        CHECK(p.infectados()[i].rol == esc.roles[i]);
        exploradores += p.infectados()[i].rol == RolInfectado::Explorador ? 1 : 0;
    }
    CHECK(exploradores == 4);  // los infectados 0, 3, 6 y 9
    CHECK(juego::nombreRol(RolInfectado::Explorador) == "explorador");
    CHECK(juego::nombreRol(RolInfectado::Perseguidor) == "perseguidor");

    // Sin roles en el escenario, todos son perseguidores (escenarios de las pruebas anteriores).
    auto e = abierto(10, 5, {1.5f, 1.5f});
    e.infectados = {{8.5f, 3.5f}};
    Partida sinRoles(std::move(e));
    CHECK(sinRoles.infectados()[0].rol == RolInfectado::Perseguidor);
}
