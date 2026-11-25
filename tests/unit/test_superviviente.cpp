#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "contador_asignaciones.hpp"
#include "dominio/juego/Partida.hpp"
#include "dominio/superviviente/ArbolComportamiento.hpp"
#include "dominio/superviviente/Goap.hpp"
#include "dominio/superviviente/Superviviente.hpp"

using namespace refugio;
using juego::Actividad;
using juego::ModoMovimiento;
using juego::MotivoReplan;
using juego::Partida;
using superviviente::Accion;
using superviviente::EstadoNodo;
using superviviente::Meta;
namespace h = superviviente::hecho;

namespace {
std::vector<Accion> accionesDe(const superviviente::PlanGoap& p) {
    std::vector<Accion> v;
    for (std::size_t i = 0; i < p.longitud; ++i) v.push_back(static_cast<Accion>(p.acciones[i]));
    return v;
}

/// Costos de CP-061: ir al almacén o a la puerta 3, recoger 1, arrancar 8, reparar 2.
superviviente::CostosAcciones costosCp061() {
    superviviente::CostosAcciones c;
    c.irAlmacen = 3.0f;
    c.recoger = 1.0f;
    c.arrancar = 8.0f;
    c.irPuerta = 3.0f;
    c.reparar = 2.0f;
    return c;
}

/// Menor costo por fuerza bruta hacia delante (todas las secuencias de hasta @p prof acciones).
float costoMinimoFuerzaBruta(superviviente::Hechos estado, superviviente::Hechos meta,
                             const std::array<superviviente::AccionGoap, superviviente::kNumAcciones>& acc, int prof) {
    if ((estado & meta) == meta) return 0.0f;
    if (prof == 0) return 1e9f;
    float mejor = 1e9f;
    for (const auto& a : acc) {
        if (!a.habilitada || (estado & a.pre) != a.pre) continue;
        const superviviente::Hechos sig = (estado & ~a.borra) | a.anade;
        mejor = std::min(mejor, a.costo + costoMinimoFuerzaBruta(sig, meta, acc, prof - 1));
    }
    return mejor;
}

/// Hojas simuladas: cada clave devuelve lo que diga la tabla y se cuentan las visitas.
struct HojasFalsas final : superviviente::EjecutorHojas {
    std::array<bool, 8> cond{};
    std::array<EstadoNodo, 8> acc{};
    std::array<int, 8> visitas{};
    bool condicion(std::uint16_t c) override { ++visitas[c]; return cond[c]; }
    EstadoNodo accion(std::uint16_t c) override { ++visitas[c]; return acc[c]; }
};

/// Pasillo de 24 x 12 con el refugio (radio) a la izquierda y un almacén cerrado a la derecha,
/// con una sola entrada en (14, 5). El superviviente empieza en la puerta del refugio.
juego::Escenario escenarioSuperviviente() {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(24, 12);
    for (int x = 14; x <= 18; ++x) {
        e.mapa.bloquear(x, 3);
        e.mapa.bloquear(x, 7);
    }
    for (int y = 3; y <= 7; ++y) {
        if (y != 5) e.mapa.bloquear(14, y);
        e.mapa.bloquear(18, y);
    }
    e.radio = {3, 5};
    e.inicioSuperviviente = {3.5f, 5.5f};
    e.almacen = {16, 5};
    e.inicioMara = {1.5f, 10.5f};
    e.extraccion = {22, 10};
    return e;
}

juego::ConfigPartida conIntegridad(float integridad) {
    juego::ConfigPartida cfg;
    cfg.integridadInicial = integridad;
    return cfg;
}
}  // namespace

// --------------------------------------------------------------------------------- RF-060
TEST_CASE("CP-060 El superviviente elige la meta por utilidad con histeresis", "[RF-060][HU-13]") {
    using superviviente::elegirMeta;
    using superviviente::puntuarMetas;
    SECTION("Integridad al 30 % y ningun infectado a menos de 8 celdas: RefugioSeguro") {
        const auto u = puntuarMetas({0.3f, 1e9f, false});
        CHECK(u[static_cast<std::size_t>(Meta::RefugioSeguro)] == Catch::Approx(0.7f).margin(1e-3f));
        CHECK(elegirMeta(u, Meta::Vigilar, 0.1f) == Meta::RefugioSeguro);
        // Con el infectado a 9 celdas (fuera de vision) sigue igual.
        CHECK(elegirMeta(puntuarMetas({0.3f, 9.0f, false}), Meta::Vigilar, 0.1f) == Meta::RefugioSeguro);
    }
    SECTION("Un infectado a 2 celdas: PonerseASalvo") {
        const auto u = puntuarMetas({0.3f, 2.0f, false});
        CHECK(elegirMeta(u, Meta::Vigilar, 0.1f) == Meta::PonerseASalvo);
        CHECK(elegirMeta(u, Meta::RefugioSeguro, 0.1f) == Meta::PonerseASalvo);
    }
    SECTION("Todas las utilidades estan en [0, 1]") {
        for (float integ : {0.0f, 0.5f, 1.0f})
            for (float d : {0.0f, 2.0f, 5.0f, 1e9f})
                for (bool inst : {false, true})
                    for (float v : puntuarMetas({integ, d, inst})) {
                        CHECK(v >= 0.0f);
                        CHECK(v <= 1.0f);
                    }
    }
    SECTION("Cerca del empate no cambia de meta por debajo de la histeresis") {
        // Vigilar vale 0,2: RefugioSeguro con integridad 75 % vale 0,25 (no basta) y con 65 %, 0,35.
        CHECK(elegirMeta(puntuarMetas({0.75f, 1e9f, false}), Meta::Vigilar, 0.1f) == Meta::Vigilar);
        CHECK(elegirMeta(puntuarMetas({0.65f, 1e9f, false}), Meta::Vigilar, 0.1f) == Meta::RefugioSeguro);
        // Y al reves: estando en RefugioSeguro, una integridad del 85 % (0,15 < 0,2 + 0,1) tampoco lo saca.
        CHECK(elegirMeta(puntuarMetas({0.85f, 1e9f, false}), Meta::RefugioSeguro, 0.1f) == Meta::RefugioSeguro);
    }
    SECTION("Ayudar con la instalacion cuando Mara instala") {
        CHECK(elegirMeta(puntuarMetas({1.0f, 1e9f, true}), Meta::Vigilar, 0.1f) == Meta::AyudarInstalacion);
    }
}

// --------------------------------------------------------------------------------- RF-061
TEST_CASE("CP-061 GOAP: sin tablas y con meta RefugioSeguro el plan es ir, recoger, volver y reparar", "[RF-061][HU-13]") {
    superviviente::PlanificadorGoap goap;
    const auto acc = superviviente::catalogoAcciones(costosCp061());
    const auto plan = goap.planificar(0, h::PuertaReparada, acc);
    REQUIRE(plan.encontrado);
    CHECK(accionesDe(plan) == std::vector<Accion>{Accion::IrAlmacen, Accion::RecogerTablas, Accion::IrPuerta, Accion::RepararPuerta});
    CHECK(plan.costo == Catch::Approx(9.0f));
    CHECK(plan.longitud <= superviviente::kProfundidadMaxGoap);
    // Costo minimo: coincide con la busqueda exhaustiva hacia delante.
    CHECK(plan.costo == Catch::Approx(costoMinimoFuerzaBruta(0, h::PuertaReparada, acc, 6)));

    SECTION("Con el almacen inaccesible arranca tablas") {
        auto c = costosCp061();
        c.almacenAccesible = false;
        const auto p = goap.planificar(0, h::PuertaReparada, superviviente::catalogoAcciones(c));
        REQUIRE(p.encontrado);
        CHECK(accionesDe(p) == std::vector<Accion>{Accion::ArrancarTablas, Accion::IrPuerta, Accion::RepararPuerta});
    }
    SECTION("En la puerta y con tablas basta con reparar") {
        const auto p = goap.planificar(h::EnPuerta | h::TieneTablas, h::PuertaReparada, acc);
        CHECK(accionesDe(p) == std::vector<Accion>{Accion::RepararPuerta});
    }
    SECTION("Meta ya cumplida: plan vacio") {
        const auto p = goap.planificar(h::ASalvo, h::ASalvo, acc);
        CHECK(p.encontrado);
        CHECK(p.longitud == 0);
    }
    SECTION("Las otras metas") {
        CHECK(accionesDe(goap.planificar(0, h::Vigilando, acc)) == std::vector<Accion>{Accion::IrPuerta, Accion::Vigilar});
        CHECK(accionesDe(goap.planificar(h::EnPuerta, h::Ayudando, acc)) == std::vector<Accion>{Accion::AyudarInstalacion});
        CHECK(accionesDe(goap.planificar(h::EnPuerta, h::ASalvo, acc)) == std::vector<Accion>{Accion::Huir});
    }
}

TEST_CASE("CP-127 GOAP con 20 acciones no pasa de 6 acciones de profundidad", "[RF-061][RNF-027]") {
    // Cadena f0 -> f1 -> ... -> f7 (una accion por eslabon) mas 13 acciones de relleno.
    std::array<superviviente::AccionGoap, 20> acc{};
    for (std::uint32_t i = 0; i < 7; ++i) acc[i] = {1u << i, 1u << (i + 1), 0, 1.0f, true};
    for (std::uint32_t i = 7; i < 20; ++i) acc[i] = {1u << 20, 1u << (i + 1), 0, 1.0f, true};  // inalcanzables
    superviviente::PlanificadorGoap goap;
    const auto seis = goap.planificar(1u << 0, 1u << 6, acc);
    REQUIRE(seis.encontrado);
    CHECK(seis.longitud == 6);
    for (std::uint8_t i = 0; i < 6; ++i) CHECK(seis.acciones[i] == i);
    const auto siete = goap.planificar(1u << 0, 1u << 7, acc);  // necesitaria 7
    CHECK_FALSE(siete.encontrado);
    CHECK(siete.expandidos <= superviviente::kMaxNodosGoap);
    // Planificar no asigna memoria (RNF-006).
    pruebas::VentanaAsignaciones v;
    (void)goap.planificar(1u << 0, 1u << 6, acc);
    CHECK(v.asignaciones() == 0);
}

// --------------------------------------------------------------------------------- RF-062
TEST_CASE("CP-062 Arbol de comportamiento: secuencia, selector y decoradores propagan los estados", "[RF-062][HU-13]") {
    superviviente::ArbolComportamiento a;
    HojasFalsas f;
    const auto c0 = a.condicion(0);
    const auto x1 = a.accion(1);
    const auto x2 = a.accion(2);
    const auto seq = a.secuencia({c0, x1, x2});
    const auto sel = a.selector({c0, x1, x2});

    SECTION("Secuencia") {
        f.cond[0] = true;
        f.acc[1] = EstadoNodo::Exito;
        f.acc[2] = EstadoNodo::Exito;
        CHECK(a.tick(seq, f) == EstadoNodo::Exito);
        f.acc[1] = EstadoNodo::EnCurso;
        f.visitas = {};
        CHECK(a.tick(seq, f) == EstadoNodo::EnCurso);
        CHECK(f.visitas[2] == 0);  // no pasa al siguiente mientras uno sigue en curso
        f.cond[0] = false;          // precondicion rota: falla sin tocar las acciones
        f.visitas = {};
        CHECK(a.tick(seq, f) == EstadoNodo::Fallo);
        CHECK(f.visitas[1] == 0);
    }
    SECTION("Selector") {
        f.cond[0] = true;
        CHECK(a.tick(sel, f) == EstadoNodo::Exito);
        CHECK(f.visitas[1] == 0);
        f.cond[0] = false;
        f.acc[1] = EstadoNodo::EnCurso;
        CHECK(a.tick(sel, f) == EstadoNodo::EnCurso);
        f.acc[1] = EstadoNodo::Fallo;
        f.acc[2] = EstadoNodo::Fallo;
        CHECK(a.tick(sel, f) == EstadoNodo::Fallo);
        f.acc[2] = EstadoNodo::Exito;
        CHECK(a.tick(sel, f) == EstadoNodo::Exito);
    }
    SECTION("Decoradores") {
        const auto inv = a.inversor(x1);
        const auto forzar = a.forzarExito(x1);
        f.acc[1] = EstadoNodo::Exito;
        CHECK(a.tick(inv, f) == EstadoNodo::Fallo);
        f.acc[1] = EstadoNodo::Fallo;
        CHECK(a.tick(inv, f) == EstadoNodo::Exito);
        CHECK(a.tick(forzar, f) == EstadoNodo::Exito);
        f.acc[1] = EstadoNodo::EnCurso;
        CHECK(a.tick(inv, f) == EstadoNodo::EnCurso);
        CHECK(a.tick(forzar, f) == EstadoNodo::EnCurso);
    }
    SECTION("Profundidad maxima declarada (RNF-027)") {
        f.cond[0] = true;
        auto n = c0;
        for (int i = 0; i < 9; ++i) n = a.inversor(a.inversor(n));  // 19 niveles
        CHECK(a.profundidad(n) == 19);
        CHECK(a.tick(n, f) == EstadoNodo::Fallo);  // se corta en el nivel 10
        auto m = c0;
        for (int i = 0; i < 4; ++i) m = a.inversor(a.inversor(m));  // 9 niveles: se evalua entero
        CHECK(a.tick(m, f) == EstadoNodo::Exito);
    }
}

TEST_CASE("CP-062 Los subarboles del superviviente caben en la profundidad maxima", "[RF-062][RNF-027]") {
    Partida p(escenarioSuperviviente());
    CHECK(p.arbolSuperviviente().numNodos() > 0);
    for (std::size_t n = 0; n < p.arbolSuperviviente().numNodos(); ++n)
        CHECK(p.arbolSuperviviente().profundidad(static_cast<superviviente::ArbolComportamiento::Id>(n)) <= superviviente::kProfundidadMaxArbol);
}

// ---------------------------------------------------------------------------- Integración
TEST_CASE("HU-13 Con el refugio al 30 % el superviviente va a por tablas y repara", "[RF-060][RF-061][RF-063][HU-13]") {
    Partida p(escenarioSuperviviente(), conIntegridad(30.0f));
    p.paso({});
    const auto& s = p.superviviente();
    CHECK(s.meta == Meta::RefugioSeguro);
    REQUIRE(p.numRegistrosSuperviviente() == 1);
    CHECK(accionesDe(s.plan) == std::vector<Accion>{Accion::IrAlmacen, Accion::RecogerTablas, Accion::IrPuerta, Accion::RepararPuerta});
    CHECK(p.ultimoRegistroSuperviviente(0).motivo == MotivoReplan::Inicio);
    bool recogio = false;
    for (int i = 0; i < 30 * 60 && s.actividad != Actividad::Reparar; ++i) {
        p.paso({});
        recogio = recogio || s.tablas > 0.0f;
    }
    CHECK(recogio);
    CHECK(s.actividad == Actividad::Reparar);
    CHECK(std::hypot(s.pos.x - 3.5f, s.pos.y - 5.5f) < 0.8f);
    for (int i = 0; i < 10 * 60; ++i) p.paso({});
    CHECK(p.integridad() == Catch::Approx(70.0f).margin(0.5f));  // 30 + 40 de una carga de tablas
}

TEST_CASE("HU-13 Ante un infectado a 2 celdas el superviviente se pone a salvo", "[RF-060][RF-062][HU-13]") {
    auto e = escenarioSuperviviente();
    e.infectados.push_back({5.5f, 5.5f});
    Partida p(std::move(e), conIntegridad(30.0f));
    p.paso({});
    const auto& s = p.superviviente();
    CHECK(s.distanciaAmenaza == Catch::Approx(2.0f).margin(0.05f));
    CHECK(s.meta == Meta::PonerseASalvo);
    CHECK(accionesDe(s.plan) == std::vector<Accion>{Accion::Huir});
    CHECK(s.actividad == Actividad::Huir);
    for (int i = 0; i < 30; ++i) p.paso({});
    CHECK(std::hypot(s.pos.x - p.infectados()[0].pos.x, s.pos.y - p.infectados()[0].pos.y) > 2.0f);
}

TEST_CASE("CP-063 Reparando 2 s el superviviente suma 16 puntos con tope en el maximo", "[RF-063][HU-13]") {
    SECTION("+16 en 2 s") {
        Partida p(escenarioSuperviviente(), conIntegridad(30.0f));
        for (int i = 0; i < 30 * 60 && p.superviviente().actividad != Actividad::Reparar; ++i) p.paso({});
        REQUIRE(p.superviviente().actividad == Actividad::Reparar);
        const float antes = p.integridad();
        for (int i = 0; i < 2 * 60; ++i) p.paso({});
        CHECK(p.integridad() - antes == Catch::Approx(16.0f).margin(0.01f));
    }
    SECTION("Tope en INTEGRIDAD_REFUGIO_MAX") {
        // Sin la utilidad de vigilar repara hasta el final; nunca pasa del máximo.
        auto cfg = conIntegridad(70.0f);
        cfg.utilidad.vigilar = 0.0f;
        Partida p(escenarioSuperviviente(), cfg);
        for (int i = 0; i < 30 * 60 && p.superviviente().actividad != Actividad::Reparar; ++i) p.paso({});
        REQUIRE(p.superviviente().actividad == Actividad::Reparar);
        float maximo = 0.0f;
        for (int i = 0; i < 10 * 60; ++i) {
            p.paso({});
            maximo = std::max(maximo, p.integridad());
        }
        CHECK(maximo <= 100.0f);
        CHECK(p.integridad() == Catch::Approx(100.0f));
        CHECK(p.superviviente().tablas == Catch::Approx(10.0f).margin(0.01f));  // le sobran 10 de 40
    }
    SECTION("Con la histeresis deja de reparar al pasar de 90 y vigila") {
        Partida p(escenarioSuperviviente(), conIntegridad(55.0f));
        for (int i = 0; i < 40 * 60; ++i) p.paso({});
        CHECK(p.integridad() > 90.0f);
        CHECK(p.integridad() < 95.0f);
        CHECK(p.superviviente().meta == Meta::Vigilar);
    }
}

TEST_CASE("CP-064 Si el almacen queda bloqueado el superviviente replanifica en el siguiente tick", "[RF-064][HU-13]") {
    Partida p(escenarioSuperviviente(), conIntegridad(30.0f));
    std::uint64_t asignaciones = 0;  // RNF-006: decidir, planificar y replanificar no asigna memoria
    {
        pruebas::VentanaAsignaciones v;
        for (int i = 0; i < 30; ++i) p.paso({});
        asignaciones += v.asignaciones();
    }
    const auto& s = p.superviviente();
    REQUIRE(s.plan.longitud == 4);
    REQUIRE(static_cast<Accion>(s.plan.acciones[s.paso]) == Accion::IrAlmacen);
    REQUIRE(s.actividad == Actividad::Mover);
    REQUIRE(p.colocarBarricadaEn({14, 5}));  // la unica entrada del almacen
    const auto planesAntes = p.totalRegistrosSuperviviente();
    {
        pruebas::VentanaAsignaciones v;
        for (int i = 0; i < 30 && p.totalRegistrosSuperviviente() == planesAntes; ++i) p.paso({});
        asignaciones += v.asignaciones();
    }
    REQUIRE(p.totalRegistrosSuperviviente() == planesAntes + 1);
    const auto& r = p.ultimoRegistroSuperviviente(0);
    CHECK(r.motivo == MotivoReplan::AccionFallida);
    CHECK(r.accionFallida == Accion::IrAlmacen);
    CHECK(r.tick - r.tickFallo == static_cast<std::uint64_t>(p.config().tickSimHz / p.config().tickIaHz));  // siguiente tick de IA
    const auto nuevo = accionesDe(s.plan);
    CHECK(std::find(nuevo.begin(), nuevo.end(), Accion::IrAlmacen) == nuevo.end());
    CHECK(nuevo.front() == Accion::ArrancarTablas);
    CHECK(nuevo.back() == Accion::RepararPuerta);
    // Con el nuevo plan termina reparando.
    {
        pruebas::VentanaAsignaciones v;
        for (int i = 0; i < 25 * 60; ++i) p.paso({});
        asignaciones += v.asignaciones();
    }
    CHECK(p.integridad() > 30.0f);
    CHECK(asignaciones == 0);
}

TEST_CASE("HU-13 Si Mara instala la radio el superviviente ayuda y tarda menos", "[RF-060][RF-007][HU-13]") {
    auto e = escenarioSuperviviente();
    e.inicioMara = {3.5f, 5.5f};
    e.objetos = {{juego::TipoObjeto::Medicamento, {3, 4}, false},
                 {juego::TipoObjeto::Combustible, {3, 6}, false},
                 {juego::TipoObjeto::Bateria, {2, 5}, false}};
    Partida p(std::move(e));
    for (int i = 0; i < 3; ++i) p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
    REQUIRE(p.tieneTodosLosObjetos());
    int pasos = 0;
    bool ayudo = false;
    for (; pasos < 8 * 60 && p.activacionesRadio() == 0; ++pasos) {
        p.paso({0.0f, 0.0f, ModoMovimiento::Caminar, true});
        ayudo = ayudo || p.superviviente().ayudando;
    }
    CHECK(ayudo);
    CHECK(p.activacionesRadio() == 1);
    CHECK(pasos < static_cast<int>(6.0f * 60.0f));  // 8 s / 1,5 = 5,3 s más el primer tick sin ayuda
}

TEST_CASE("HU-13 El escenario aleatorio trae superviviente y almacen", "[HU-13]") {
    const auto e = juego::crearEscenarioAleatorio(48, 32, 5, 7);
    CHECK(e.inicioSuperviviente.x >= 0.0f);
    REQUIRE(e.almacen.x >= 0);
    CHECK(e.mapa.transitable(e.almacen.x, e.almacen.y));
    Partida p(e);
    CHECK(p.superviviente().activo);
    for (int i = 0; i < 60; ++i) p.paso({});
    CHECK(p.superviviente().meta == Meta::Vigilar);  // refugio intacto y sin amenazas
    CHECK(p.totalRegistrosSuperviviente() >= 1);
}
