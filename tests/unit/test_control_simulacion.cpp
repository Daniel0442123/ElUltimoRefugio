// Control de la simulación (HU-22): RF-075, RNF-020, ADR-0005.
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <vector>

#include "dominio/config/Configuracion.hpp"
#include "dominio/juego/ControlSimulacion.hpp"
#include "dominio/persistencia/PartidaGuardada.hpp"

using namespace refugio;
using juego::ControlSimulacion;

namespace {

constexpr double kFrame = 1.0 / 60.0;  // un monitor de 60 Hz

persistencia::InicioPartida inicioPrueba() {
    persistencia::InicioPartida in;
    in.semilla = 21;
    in.ancho = 32;
    in.alto = 22;
    in.infectados = 10;
    in.flowfieldUmbral = 5;
    return in;
}

/// Entrada determinista que depende solo del tick de la partida (no del frame ni de la
/// velocidad): Mara camina, corre, se detiene, recoge, lanza un señuelo y pone una barricada.
juego::EntradaJugador entradaDelTick(const juego::Partida& p) {
    const auto t = p.tick();
    juego::EntradaJugador e;
    const int fase = static_cast<int>((t / 40) % 6);
    e.ejeX = fase == 0 || fase == 1 ? 1.0f : fase == 3 ? -1.0f : 0.0f;
    e.ejeY = fase == 2 ? 1.0f : fase == 4 ? -1.0f : 0.0f;
    e.modo = fase == 1 ? juego::ModoMovimiento::Correr : juego::ModoMovimiento::Caminar;
    e.interactuar = fase == 5;
    e.lanzarSenuelo = t == 150;
    e.colocarBarricada = t == 333;
    e.apuntar = {p.mara().pos.x + 4.0f, p.mara().pos.y};
    return e;
}

/// Juega frames de 1/60 s con @p control hasta llegar a @p ticks pasos de simulación.
std::uint64_t jugarHasta(ControlSimulacion& control, juego::Partida& p, std::uint64_t ticks) {
    std::uint64_t frames = 0;
    while (p.tick() < ticks) {
        const int n = control.pasosParaFrame(kFrame);
        for (int k = 0; k < n && p.tick() < ticks; ++k) p.paso(entradaDelTick(p));
        if (control.pausada()) control.pedirPaso();  // en pausa: un tick por frame, como pulsar F6
        ++frames;
    }
    return frames;
}

int pasosEnUnSegundo(ControlSimulacion& c) {
    int total = 0;
    for (int f = 0; f < 60; ++f) total += c.pasosParaFrame(kFrame);
    return total;
}

}  // namespace

TEST_CASE("CP-075 En pausa cada paso pedido ejecuta exactamente un tick", "[RF-075][HU-22]") {
    auto p = persistencia::crearPartida(inicioPrueba());
    ControlSimulacion c(static_cast<double>(p->dt()));
    for (int f = 0; f < 30; ++f)  // medio segundo a 1x
        for (int k = c.pasosParaFrame(kFrame); k > 0; --k) p->paso(entradaDelTick(*p));
    REQUIRE(p->tick() == 30);

    c.alternarPausa();
    REQUIRE(c.pausada());
    // Dos segundos en pausa: no avanza nada.
    for (int f = 0; f < 120; ++f) CHECK(c.pasosParaFrame(kFrame) == 0);
    CHECK(p->tick() == 30);

    // N x 10, con frames vacíos entre pulsaciones: exactamente 10 ticks.
    const auto registro = p->totalCambiosPlan();
    for (int i = 0; i < 10; ++i) {
        c.pedirPaso();
        for (int k = c.pasosParaFrame(kFrame); k > 0; --k) p->paso(entradaDelTick(*p));
        for (int f = 0; f < 5; ++f) CHECK(c.pasosParaFrame(kFrame) == 0);
    }
    CHECK(p->tick() == 40);
    CHECK(p->totalCambiosPlan() >= registro);  // el panel lee el estado nuevo de la partida

    // Varias pulsaciones dentro del mismo frame cuentan todas; un frame larguísimo no añade nada.
    c.pedirPaso();
    c.pedirPaso();
    c.pedirPaso();
    CHECK(c.pasosParaFrame(5.0) == 3);
    CHECK(c.pasosParaFrame(5.0) == 0);

    // Sin pausa, pedir paso no hace nada y la simulación sigue a su ritmo.
    c.alternarPausa();
    c.pedirPaso();
    CHECK(pasosEnUnSegundo(c) == 60);
}

TEST_CASE("CP-075 Velocidades 0.25x a 4x dan 15 a 240 ticks por segundo real", "[RF-075][HU-22]") {
    ControlSimulacion c;
    CHECK(c.velocidad() == 1.0);
    CHECK(pasosEnUnSegundo(c) == 60);

    c.fijarIndiceVelocidad(0);
    CHECK(c.velocidad() == 0.25);
    CHECK(pasosEnUnSegundo(c) == 15);
    // 0,25x reparte un tick cada 4 frames, sin acumular deriva.
    std::vector<int> patron;
    for (int f = 0; f < 8; ++f) patron.push_back(c.pasosParaFrame(kFrame));
    CHECK(patron == std::vector<int>{0, 0, 0, 1, 0, 0, 0, 1});

    CHECK(c.masRapido() == 0.5);
    CHECK(pasosEnUnSegundo(c) == 30);
    CHECK(c.masRapido() == 1.0);
    CHECK(c.masRapido() == 2.0);
    CHECK(pasosEnUnSegundo(c) == 120);
    CHECK(c.masRapido() == 4.0);
    CHECK(pasosEnUnSegundo(c) == 240);
    CHECK(c.masRapido() == 4.0);  // tope
    for (int i = 0; i < 10; ++i) c.masLento();
    CHECK(c.velocidad() == 0.25);  // suelo

    // Con frames irregulares (30 Hz y 144 Hz) el total por segundo es el mismo.
    ControlSimulacion d;
    int total = 0;
    for (int f = 0; f < 30; ++f) total += d.pasosParaFrame(1.0 / 30.0);
    CHECK(total == 60);
    total = 0;
    for (int f = 0; f < 144; ++f) total += d.pasosParaFrame(1.0 / 144.0);
    CHECK((total == 60 || total == 59));  // el resto queda en el acumulador para el frame siguiente
}

TEST_CASE("CP-075 Un tiron largo se recorta a 0.25 s y la pausa no acumula tiempo", "[RF-075][ADR-0005]") {
    ControlSimulacion c;
    CHECK(c.pasosParaFrame(3.0) == 15);  // 0,25 s a 60 Hz, no 180 pasos de golpe
    c.fijarIndiceVelocidad(4);
    CHECK(c.pasosParaFrame(3.0) == 60);  // 0,25 s a 4x
    CHECK(c.pasosParaFrame(-1.0) == 0);

    c.fijarIndiceVelocidad(ControlSimulacion::kIndiceNormal);
    CHECK(c.pasosParaFrame(kFrame * 0.9) == 0);  // casi un tick acumulado
    c.alternarPausa();
    for (int f = 0; f < 600; ++f) CHECK(c.pasosParaFrame(kFrame) == 0);
    c.alternarPausa();
    // Al salir de la pausa no hay ráfaga: el acumulador empezó de cero.
    CHECK(c.pasosParaFrame(kFrame * 0.5) == 0);
    CHECK(c.pasosParaFrame(kFrame * 0.5) == 1);
}

TEST_CASE("CP-075 Mismo estado final tras 600 ticks a cualquier velocidad", "[RF-075][RNF-020][HU-22]") {
    struct Caso {
        std::size_t velocidad;
        bool pausa;
        std::uint64_t framesEsperados;
    };
    // 600 ticks: 2400 frames a 0,25x, 600 a 1x, 150 a 4x y 600 pulsando F6 en pausa.
    const Caso casos[] = {{0, false, 2400}, {1, false, 1200}, {2, false, 600}, {3, false, 300}, {4, false, 150},
                          {2, true, 601}};
    std::vector<std::uint64_t> huellas;
    for (const auto& caso : casos) {
        auto p = persistencia::crearPartida(inicioPrueba());
        ControlSimulacion c(static_cast<double>(p->dt()));
        c.fijarIndiceVelocidad(caso.velocidad);
        if (caso.pausa) c.alternarPausa();
        const auto frames = jugarHasta(c, *p, 600);
        CHECK(frames == caso.framesEsperados);
        REQUIRE(p->tick() == 600);
        huellas.push_back(persistencia::huellaPartida(*p));
    }
    for (const auto h : huellas) CHECK(h == huellas.front());

    // Cambiar de velocidad y pausar a mitad de partida tampoco cambia el resultado.
    auto p = persistencia::crearPartida(inicioPrueba());
    ControlSimulacion c(static_cast<double>(p->dt()));
    c.fijarIndiceVelocidad(0);
    jugarHasta(c, *p, 123);
    c.fijarIndiceVelocidad(4);
    jugarHasta(c, *p, 300);
    c.alternarPausa();
    jugarHasta(c, *p, 310);
    c.alternarPausa();
    c.masLento();
    jugarHasta(c, *p, 600);
    CHECK(persistencia::huellaPartida(*p) == huellas.front());
}

TEST_CASE("RF-075 Las teclas de control de la simulacion se pueden reasignar", "[RF-075][RNF-012][config]") {
    const config::Controles porDefecto;
    CHECK(porDefecto.tecla(config::Accion::PausaSimulacion) == 'P');
    CHECK(config::nombreTecla(porDefecto.tecla(config::Accion::PasoSimulacion)) == "F6");
    CHECK(config::nombreTecla(porDefecto.tecla(config::Accion::MasLento)) == "F7");
    CHECK(config::nombreTecla(porDefecto.tecla(config::Accion::MasRapido)) == "F8");

    // Ida y vuelta por controles.json.
    config::Controles k;
    REQUIRE(k.asignar(config::Accion::PasoSimulacion, 'M'));
    const auto l = config::leerControles(config::escribirControles(k));
    CHECK(l.errores.empty());
    CHECK(l.valor == k);

    // Un controles.json de la 0.13.0 (sin las acciones nuevas) que usa P y F6 para otras acciones
    // se respeta: las acciones nuevas toman teclas libres en vez de perder toda la configuración.
    const auto vieja = config::leerControles(R"({"nodos": "P", "consola": "F6"})");
    CHECK(vieja.errores.empty());
    CHECK(vieja.valor.tecla(config::Accion::Nodos) == 'P');
    CHECK(config::nombreTecla(vieja.valor.tecla(config::Accion::Consola)) == "F6");
    const int pausa = vieja.valor.tecla(config::Accion::PausaSimulacion);
    const int paso = vieja.valor.tecla(config::Accion::PasoSimulacion);
    CHECK(pausa != 'P');
    CHECK(paso != config::tecla::kF1 + 5);
    CHECK(pausa != paso);
    CHECK(config::teclaAsignable(pausa));
    CHECK(config::teclaAsignable(paso));
    for (std::size_t i = 0; i < config::kNumAcciones; ++i)
        for (std::size_t j = i + 1; j < config::kNumAcciones; ++j)
            CHECK(vieja.valor.tecla(static_cast<config::Accion>(i)) != vieja.valor.tecla(static_cast<config::Accion>(j)));
}
