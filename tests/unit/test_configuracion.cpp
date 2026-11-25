// Configuración local (HU-19): RF-092, RF-093, RNF-012, RNF-016.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "adaptadores/archivo/ArchivosLocales.hpp"
#include "dominio/config/Configuracion.hpp"
#include "dominio/juego/Partida.hpp"

using namespace refugio;
using config::Accion;
using Catch::Matchers::ContainsSubstring;

namespace {
std::string leerTodo(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
}  // namespace

TEST_CASE("Opciones y controles por defecto", "[config]") {
    const config::OpcionesJuego o;
    CHECK(o.volumen == 0.8f);
    CHECK(o.tutorial);
    const config::Controles c;
    CHECK(c.tecla(Accion::Arriba) == 'W');
    CHECK(c.tecla(Accion::Correr) == config::tecla::kShiftIzq);
    CHECK(c.tecla(Accion::GuardadoRapido) == config::tecla::kF1 + 4);
    CHECK(c.tecla(Accion::CargaRapida) == config::tecla::kF1 + 8);
    // Ninguna tecla repetida ni reservada: todas las acciones se pueden usar y reasignar (RNF-012).
    for (std::size_t i = 0; i < config::kNumAcciones; ++i) {
        const auto a = static_cast<Accion>(i);
        CHECK(config::teclaAsignable(c.tecla(a)));
        CHECK(c.accionDe(c.tecla(a)) == static_cast<int>(i));
        CHECK_FALSE(config::claveAccion(a).empty());
        CHECK_FALSE(config::textoAccion(a).empty());
    }
}

TEST_CASE("Nombres de tecla de ida y vuelta", "[config]") {
    int nombradas = 0;
    for (int k = 0; k < 400; ++k) {
        const auto n = config::nombreTecla(k);
        if (n.empty()) continue;
        ++nombradas;
        CHECK(config::teclaDesdeNombre(n) == k);
    }
    CHECK(nombradas == 26 + 10 + 12 + 9);
    CHECK(config::teclaDesdeNombre("left_shift") == config::tecla::kShiftIzq);
    CHECK(config::teclaDesdeNombre("f12") == config::tecla::kF1 + 11);
    CHECK(config::teclaDesdeNombre("F13") == config::tecla::kNinguna);
    CHECK(config::teclaDesdeNombre("ESCAPE") == config::tecla::kNinguna);  // reservada: sin nombre
    CHECK(config::teclaReservada(config::tecla::kEscape));
    CHECK(config::teclaReservada(config::tecla::kArriba));
    CHECK_FALSE(config::teclaAsignable(config::tecla::kEnter));
    CHECK(config::textoTecla(config::tecla::kCtrlIzq) == "Ctrl");
}

TEST_CASE("Reasignar una tecla usada intercambia las dos acciones", "[config][RNF-012]") {
    config::Controles c;
    REQUIRE(c.asignar(Accion::Senuelo, 'E'));  // E era de Interactuar
    CHECK(c.tecla(Accion::Senuelo) == 'E');
    CHECK(c.tecla(Accion::Interactuar) == 'Q');
    CHECK_FALSE(c.asignar(Accion::Senuelo, config::tecla::kEscape));
    CHECK(c.tecla(Accion::Senuelo) == 'E');
    CHECK(c.asignar(Accion::Arriba, 'I'));
    CHECK(c.tecla(Accion::Arriba) == 'I');
}

TEST_CASE("CP-092 cambiar volumen y una tecla se guarda en los JSON y se relee", "[config][CP-092]") {
    const auto dir = std::filesystem::path(REFUGIO_DIR_SALIDA) / "cp092";
    std::filesystem::remove_all(dir);
    // Sin archivos: valores por defecto y ningún error (faltar no es estar dañado).
    const auto inicial = adaptadores::leerConfigLocal(dir);
    CHECK(inicial.errores.empty());
    CHECK(inicial.controles == config::Controles{});

    config::OpcionesJuego o;
    o.volumen = 0.3f;
    o.tutorial = false;
    config::Controles c;
    c.asignar(Accion::Correr, config::tecla::kEspacio);
    REQUIRE(adaptadores::guardarConfigLocal(dir, o, c).empty());
    CHECK_THAT(leerTodo(dir / "juego.json"), ContainsSubstring("\"volumen\": 0.30"));
    CHECK_THAT(leerTodo(dir / "controles.json"), ContainsSubstring("\"correr\": \"SPACE\""));
    CHECK_FALSE(std::filesystem::exists(dir / "juego.json.tmp"));

    // «Reiniciar»: se vuelve a leer la carpeta.
    const auto leida = adaptadores::leerConfigLocal(dir);
    CHECK(leida.errores.empty());
    CHECK(leida.opciones.volumen == 0.3f);
    CHECK_FALSE(leida.opciones.tutorial);
    CHECK(leida.controles == c);
}

TEST_CASE("CP-093 JSON mal formado usa los valores por defecto con un mensaje", "[config][CP-093]") {
    const auto o = config::leerOpciones("{ \"volumen\": 0.5, ", "config/juego.json");
    REQUIRE(o.errores.size() == 1);
    CHECK_THAT(o.errores[0], ContainsSubstring("config/juego.json"));
    CHECK_THAT(o.errores[0], ContainsSubstring("linea 1"));
    CHECK(o.valor.volumen == 0.8f);

    const auto c = config::leerControles("[1, 2]");
    REQUIRE(c.errores.size() == 1);
    CHECK(c.valor == config::Controles{});

    const auto ia = config::leerParametrosIa("");
    REQUIRE(ia.errores.size() == 1);
    CHECK(ia.valor == config::ParametrosIa{});
}

TEST_CASE("CP-093 campos fuera de rango vuelven a su valor por defecto", "[config][CP-093]") {
    const auto o = config::leerOpciones(R"({"volumen": 2.5, "tutorial": "si"})");
    CHECK(o.errores.size() == 2);
    CHECK(o.valor.volumen == 0.8f);
    CHECK(o.valor.tutorial);

    const auto c = config::leerControles(R"({"arriba": "I", "abajo": "PATATA", "senuelo": "ESCAPE"})");
    CHECK(c.errores.size() == 2);
    CHECK(c.valor.tecla(Accion::Arriba) == 'I');
    CHECK(c.valor.tecla(Accion::Abajo) == 'S');
    CHECK(c.valor.tecla(Accion::Senuelo) == 'Q');

    // Dos acciones con la misma tecla: no se puede saber cuál querías, se vuelve a los por defecto.
    const auto d = config::leerControles(R"({"arriba": "E"})");
    REQUIRE(d.errores.size() == 1);
    CHECK_THAT(d.errores[0], ContainsSubstring("interactuar"));
    CHECK(d.valor == config::Controles{});
}

TEST_CASE("RF-092 config/ia.json del repositorio se lee sin errores", "[config][RF-092]") {
    const auto l = adaptadores::leerArchivo(std::filesystem::path(REFUGIO_DIR_FUENTES) / "config" / "ia.json");
    REQUIRE(l.ok);
    const auto ia = config::leerParametrosIa(l.datos);
    CHECK(ia.errores.empty());
    CHECK(ia.valor.velCorrer == 6.0f);
    CHECK(ia.valor.flowfieldUmbral == 8);
    CHECK(ia.valor.radioComunicacion == 5.0f);
    CHECK(ia.valor.profundidadMax == 3);
    // Los valores del archivo son los de ConfigPartida: aplicarlos no cambia la partida.
    juego::ConfigPartida cfg;
    const juego::ConfigPartida antes;
    config::aplicar(ia.valor, cfg);
    CHECK(cfg.velCorrer == antes.velCorrer);
    CHECK(cfg.senueloRuidos == antes.senueloRuidos);
    CHECK(cfg.tExtraccion == antes.tExtraccion);
    CHECK(cfg.alertas.atenuacion == antes.alertas.atenuacion);
    CHECK(cfg.presupuestoNodosTick == antes.presupuestoNodosTick);
}

TEST_CASE("RF-092 ia.json cambia los parametros y valida las reglas entre campos", "[config][RF-092]") {
    const auto ia = config::leerParametrosIa(R"({"movimiento": {"vel_correr": 5}, "senuelo": {"duracion_s": 6, "periodo_s": 2}})");
    CHECK(ia.errores.empty());
    juego::ConfigPartida cfg;
    config::aplicar(ia.valor, cfg);
    CHECK(cfg.velCorrer == 5.0f);
    CHECK(cfg.senueloRuidos == 3);
    CHECK(cfg.senueloPeriodo == 2.0f);

    const auto mal = config::leerParametrosIa(R"({"tick_sim_hz": 60, "tick_ia_hz": 7})");
    REQUIRE(mal.errores.size() == 1);
    CHECK_THAT(mal.errores[0], ContainsSubstring("tick_ia_hz"));
    CHECK(mal.valor == config::ParametrosIa{});

    const auto seccion = config::leerParametrosIa(R"({"ruido": 3})");
    CHECK(seccion.errores.size() == 1);
}

TEST_CASE("CP-116 corpus de configuraciones invalidas sin cierres", "[config][CP-116]") {
    const std::vector<std::string> corpus = {
        "", " ", "{", "}", "[", "null", "true", "42", "\"texto\"", "{\"volumen\":}", "{\"volumen\": 1e999}",
        "{\"volumen\": -1}", "{\"volumen\": \"alto\"}", "{\"tutorial\": 1}", "{\"arriba\": 5}", "{\"arriba\": \"\"}",
        "{\"arriba\": \"F0\"}", "{\"arriba\": \"F99\"}", "{\"arriba\": \"UP\"}", "{\"tick_sim_hz\": 0}",
        "{\"tick_sim_hz\": 60.5}", "{\"tick_ia_hz\": 0}", "{\"movimiento\": []}", "{\"movimiento\": {\"vel_correr\": 0}}",
        "{\"alertas\": {\"profundidad_max\": 99}}", "{\"alertas\": {\"atenuacion\": 0}}",
        "{\"senuelo\": {\"duracion_s\": 0.5, \"periodo_s\": 1}}", "{\"navegacion\": {\"presupuesto_nodos_tick\": 1}}",
        "{\"coordinacion\": {\"flowfield_umbral\": -3}}", "{\"a\": [1, 2,]}", "{\"a\": \"\\uZZZZ\"}", "\xff\xfe{}",
        std::string(1000, '['),
    };
    REQUIRE(corpus.size() >= 30);
    int conErrores = 0;
    for (const auto& t : corpus) {
        config::Lectura<config::OpcionesJuego> o;
        config::Lectura<config::Controles> c;
        config::Lectura<config::ParametrosIa> ia;
        REQUIRE_NOTHROW(o = config::leerOpciones(t));
        REQUIRE_NOTHROW(c = config::leerControles(t));
        REQUIRE_NOTHROW(ia = config::leerParametrosIa(t));
        if (!o.valida() || !c.valida() || !ia.valida()) ++conErrores;
        CHECK(config::validar(ia.valor).empty());  // lo que queda siempre es utilizable
    }
    CHECK(conErrores == static_cast<int>(corpus.size()));
}
