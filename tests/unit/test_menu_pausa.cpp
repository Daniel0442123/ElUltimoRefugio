// Menú de pausa (HU-19): RF-090 a RF-092, RNF-012, RNF-014.
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>

#include "app/MenuPausa.hpp"

using namespace refugio;
using app::MenuPausa;
using app::PantallaMenu;
using Tipo = app::PeticionMenu::Tipo;
namespace tecla = config::tecla;

namespace {
const std::array<std::string, MenuPausa::kRanuras> kRanuras{"vacía", "semilla 42, 30 s de juego", "dañada"};

MenuPausa abierto() {
    MenuPausa m(config::OpcionesJuego{}, config::Controles{});
    m.abrir(kRanuras);
    return m;
}
}  // namespace

TEST_CASE("Menu: Esc abre y cierra, guardar pide la ranura", "[menu][CP-090]") {
    auto m = abierto();
    CHECK(m.pantalla() == PantallaMenu::Principal);
    CHECK(m.lineas().size() == 5);
    m.pulsar(tecla::kAbajo, false);
    m.pulsar(tecla::kEnter, false);
    REQUIRE(m.pantalla() == PantallaMenu::Guardar);
    CHECK(m.lineas()[1] == "Ranura 2: semilla 42, 30 s de juego");
    m.pulsar(tecla::kAbajo, false);
    const auto p = m.pulsar(tecla::kEnter, true);  // guardar nunca pide confirmación
    CHECK(p.tipo == Tipo::Guardar);
    CHECK(p.ranura == 2);
    m.pulsar(tecla::kEscape, false);
    CHECK(m.pantalla() == PantallaMenu::Principal);
    CHECK(m.pulsar(tecla::kEscape, false).tipo == Tipo::Continuar);
    CHECK_FALSE(m.abierto());
}

TEST_CASE("CP-114 cargar y salir con progreso sin guardar piden confirmacion", "[menu][CP-114]") {
    auto m = abierto();
    m.pulsar(tecla::kAbajo, false);
    m.pulsar(tecla::kAbajo, false);
    m.pulsar(tecla::kEnter, false);
    REQUIRE(m.pantalla() == PantallaMenu::Cargar);
    // Sin progreso pendiente, carga directamente.
    CHECK(m.pulsar(tecla::kEnter, false).tipo == Tipo::Cargar);
    // Con progreso pendiente, pregunta; «No» (por defecto) vuelve a la lista.
    CHECK(m.pulsar(tecla::kEnter, true).tipo == Tipo::Ninguna);
    REQUIRE(m.pantalla() == PantallaMenu::Confirmar);
    CHECK(m.cursor() == 1);
    CHECK(m.pulsar(tecla::kEnter, true).tipo == Tipo::Ninguna);
    CHECK(m.pantalla() == PantallaMenu::Cargar);
    // «Sí» carga.
    m.pulsar(tecla::kEnter, true);
    m.pulsar(tecla::kIzquierda, true);
    const auto p = m.pulsar(tecla::kEnter, true);
    CHECK(p.tipo == Tipo::Cargar);
    CHECK(p.ranura == 1);

    auto s = abierto();
    for (int i = 0; i < 4; ++i) s.pulsar(tecla::kAbajo, true);
    CHECK(s.pulsar(tecla::kEnter, true).tipo == Tipo::Ninguna);
    CHECK(s.titulo() == "Hay progreso sin guardar. ¿Salir del juego?");
    s.pulsar(tecla::kArriba, true);
    CHECK(s.pulsar(tecla::kEnter, true).tipo == Tipo::Salir);

    // F9 con progreso: la confirmación se abre sola y «No» vuelve al juego.
    MenuPausa f(config::OpcionesJuego{}, config::Controles{});
    f.confirmar({Tipo::Cargar, 1}, kRanuras);
    CHECK(f.abierto());
    CHECK(f.pulsar(tecla::kEnter, true).tipo == Tipo::Ninguna);
    CHECK_FALSE(f.abierto());
}

TEST_CASE("CP-092 opciones: volumen y tutorial desde el menu", "[menu][CP-092]") {
    auto m = abierto();
    for (int i = 0; i < 3; ++i) m.pulsar(tecla::kAbajo, false);
    m.pulsar(tecla::kEnter, false);
    REQUIRE(m.pantalla() == PantallaMenu::Opciones);
    CHECK(m.lineas()[0] == "Volumen: < 80 % >");
    CHECK(m.pulsar(tecla::kIzquierda, false).tipo == Tipo::OpcionesCambiadas);
    CHECK(m.opciones().volumen == 0.7f);
    for (int i = 0; i < 20; ++i) m.pulsar(tecla::kDerecha, false);
    CHECK(m.opciones().volumen == 1.0f);
    m.pulsar(tecla::kAbajo, false);
    CHECK(m.pulsar(tecla::kEnter, false).tipo == Tipo::OpcionesCambiadas);
    CHECK_FALSE(m.opciones().tutorial);
    CHECK(m.lineas()[1] == "Tutorial: < desactivado >");
}

TEST_CASE("CP-112 todas las acciones se reasignan desde el menu", "[menu][CP-112][RNF-012]") {
    auto m = abierto();
    for (int i = 0; i < 3; ++i) m.pulsar(tecla::kAbajo, false);
    m.pulsar(tecla::kEnter, false);  // Opciones
    m.pulsar(tecla::kAbajo, false);
    m.pulsar(tecla::kAbajo, false);
    m.pulsar(tecla::kEnter, false);  // Controles
    REQUIRE(m.pantalla() == PantallaMenu::Controles);
    REQUIRE(m.lineas().size() == config::kNumAcciones + 2);
    // Se asigna a cada acción una tecla F o un número distinto: 100 % reasignables.
    const int nuevas[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'Z', 'X',
                          'C', 'V', 'G', 'H', 'J', 'K', 'U', 'Y', 'I', 'M', 'F', 'O'};
    static_assert(std::size(nuevas) == config::kNumAcciones);
    for (std::size_t i = 0; i < config::kNumAcciones; ++i) {
        REQUIRE(m.cursor() == static_cast<int>(i));
        m.pulsar(tecla::kEnter, false);
        REQUIRE(m.pantalla() == PantallaMenu::EsperandoTecla);
        CHECK(m.pulsar(nuevas[i], false).tipo == Tipo::OpcionesCambiadas);
        CHECK(m.controles().tecla(static_cast<config::Accion>(i)) == nuevas[i]);
        m.pulsar(tecla::kAbajo, false);
    }
    // Tecla reservada: no cambia y avisa. Esc cancela.
    REQUIRE(m.cursor() == static_cast<int>(config::kNumAcciones));  // «Restaurar»
    m.pulsar(tecla::kArriba, false);  // última acción (MasRapido)
    m.pulsar(tecla::kEnter, false);
    CHECK(m.pulsar(tecla::kArriba, false).tipo == Tipo::Ninguna);
    CHECK(m.mensajeEsError());
    CHECK(m.controles().tecla(config::Accion::MasRapido) == 'O');
    m.pulsar(tecla::kEnter, false);
    CHECK(m.pulsar(tecla::kEscape, false).tipo == Tipo::Ninguna);
    CHECK(m.controles().tecla(config::Accion::MasRapido) == 'O');
    // Restaurar por defecto.
    m.pulsar(tecla::kAbajo, false);
    CHECK(m.pulsar(tecla::kEnter, false).tipo == Tipo::OpcionesCambiadas);
    CHECK(m.controles() == config::Controles{});
}
