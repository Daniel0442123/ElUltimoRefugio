// Modo --medir-rendimiento del juego (RNF-001, RNF-002, RNF-004, RNF-005, RNF-008): estadísticas,
// guion fijo de la jugadora, archivos de resultados y lectura de CPU y memoria del proceso.
// Las cifras de rendimiento se miden con la ventana en el equipo real (tools/rendimiento.py); aquí
// solo se prueba que la herramienta mide y escribe bien.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "adaptadores/sistema/RecursosProceso.hpp"
#include "app/MedicionRendimiento.hpp"
#include "dominio/nucleo/Json.hpp"

using Catch::Approx;
using namespace refugio;

namespace {
std::string leer(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream o;
    o << f.rdbuf();
    return o.str();
}
}  // namespace

TEST_CASE("Los percentiles de la medicion usan el rango mas cercano", "[medicion]") {
    std::vector<double> v;
    for (int i = 100; i >= 1; --i) v.push_back(i);  // desordenados a propósito
    const auto e = app::estadistica(v);
    CHECK(e.n == 100);
    CHECK(e.media == Approx(50.5));
    CHECK(e.min == 1.0);
    CHECK(e.p5 == 5.0);
    CHECK(e.p50 == 50.0);
    CHECK(e.p95 == 95.0);
    CHECK(e.p99 == 99.0);
    CHECK(e.max == 100.0);
    CHECK(app::estadistica({}).n == 0);
    CHECK(app::estadistica({7.0}).p5 == 7.0);
}

TEST_CASE("El guion de la medicion recorre las 8 direcciones y no depende del reloj", "[medicion]") {
    std::set<std::pair<float, float>> direcciones;
    bool corre = false, sigilo = false;
    for (std::uint64_t tick = 0; tick < 60 * 2 * 16; tick += 60) {
        const auto e = app::entradaGuion(tick, 60.0);
        direcciones.insert({e.ejeX, e.ejeY});
        corre = corre || e.modo == juego::ModoMovimiento::Correr;
        sigilo = sigilo || e.modo == juego::ModoMovimiento::Sigilo;
        const auto otra = app::entradaGuion(tick, 60.0);
        CHECK(otra.ejeX == e.ejeX);
        CHECK(otra.ejeY == e.ejeY);
    }
    CHECK(direcciones.size() == 8);
    CHECK(corre);
    CHECK(sigilo);
    // 2 s por tramo: el tick 119 sigue en el primer tramo y el 120 cambia de dirección.
    CHECK(app::entradaGuion(119, 60.0).ejeY == 0.0f);
    CHECK(app::entradaGuion(120, 60.0).ejeY == 1.0f);
}

TEST_CASE("Las opciones de medir-rendimiento se leen y se validan", "[medicion]") {
    app::ConfigMedicion cfg;
    const char* sin[] = {"refugio", "--captura", "x.png"};
    CHECK_FALSE(app::leerConfigMedicion(3, const_cast<char**>(sin), cfg));

    const char* con[] = {"refugio", "--medir-rendimiento", "--segundos", "60", "--calentamiento", "5",
                         "--infectados", "100", "--escenario", "horda100", "--salida", "results/r/h"};
    REQUIRE(app::leerConfigMedicion(12, const_cast<char**>(con), cfg));
    CHECK(cfg.medicionSeg == 60.0);
    CHECK(cfg.calentamientoSeg == 5.0);
    CHECK(cfg.infectados == 100);
    CHECK(cfg.escenario == "horda100");
    CHECK(cfg.salida == "results/r/h");

    const char* mal[] = {"refugio", "--medir-rendimiento", "--segundos", "uno"};
    CHECK_THROWS(app::leerConfigMedicion(4, const_cast<char**>(mal), cfg));
    const char* cero[] = {"refugio", "--medir-rendimiento", "--infectados", "0"};
    CHECK_THROWS(app::leerConfigMedicion(4, const_cast<char**>(cero), cfg));
}

TEST_CASE("La medicion descarta el calentamiento y escribe JSON y CSV completos", "[medicion]") {
    app::ConfigMedicion cfg;
    cfg.escenario = "prueba";
    cfg.calentamientoSeg = 1.0;
    cfg.medicionSeg = 3.0;
    cfg.salida = (std::filesystem::path(REFUGIO_DIR_SALIDA) / "rendimiento" / "prueba").string();
    app::MedicionRendimiento m(cfg);
    m.fase("ventana", 120.0);
    m.escena(15.0);
    m.primerFrame(300.0, 340.0);
    m.entorno("ventana", "1280x720");

    // 60 frames por segundo durante 5 s; uno de 40 ms en el calentamiento no debe contar. La CPU
    // sube 0,25 s por segundo de reloj (25 % de un núcleo).
    double t = 0.0;
    for (int i = 1; i <= 300; ++i) {
        t = i / 60.0;
        app::MuestraFrame f;
        f.msFrame = i == 30 ? 40.0 : (i % 10 == 0 ? 20.0 : 16.0);
        f.msActualizar = 1.0;
        f.msDibujar = 2.0;
        f.pasos = 1;
        f.msPasoMax = 0.5;
        f.reinicio = i == 200;
        m.frame(t, f);
        if (m.tocaMuestra(t)) m.muestra(t, 0.25 * t, 50u * 1024u * 1024u, 60u * 1024u * 1024u);
        if (m.terminado(t)) break;
    }
    CHECK(t == Approx(4.0).margin(0.02));
    CHECK(m.framesMedidos() == 180);  // de 1 s a 4 s
    CHECK(m.cpuMediaPct() == Approx(25.0).margin(0.5));
    CHECK(m.memoriaPicoMb() == Approx(60.0));
    CHECK(m.fpsP5() == Approx(50.0));  // el 10 % de los frames dura 20 ms

    m.escribir({"juego=ElUltimoRefugio version=prueba", "so=\"Sistema de prueba\""});
    const auto json = nucleo::leerJson(leer(cfg.salida + ".json"));
    CHECK(json.buscar("escenario")->comoCadena() == "prueba");
    CHECK(json.buscar("frames")->comoNumero() == 180.0);
    CHECK(json.buscar("reinicios")->comoNumero() == 1.0);
    CHECK(json.buscar("frames_mas_de_25ms")->comoNumero() == 0.0);
    CHECK(json.buscar("fps")->buscar("p5")->comoNumero() == Approx(50.0));
    CHECK(json.buscar("paso_ms")->buscar("p99")->comoNumero() == Approx(0.5));
    CHECK(json.buscar("cpu_media_pct")->comoNumero() == Approx(25.0).margin(0.5));
    CHECK(json.buscar("memoria_pico_mb")->comoNumero() == Approx(60.0));
    const auto* arranque = json.buscar("arranque");
    REQUIRE(arranque != nullptr);
    CHECK(arranque->buscar("primer_frame_desde_proceso_ms")->comoNumero() == Approx(340.0));
    CHECK(arranque->buscar("fases_ms")->buscar("ventana")->comoNumero() == Approx(120.0));
    CHECK(arranque->buscar("escenas_ms")->comoArreglo().size() == 1);
    CHECK(json.buscar("entorno")->buscar("ventana")->comoCadena() == "1280x720");
    CHECK(json.buscar("meta")->comoArreglo()[1].comoCadena() == "so=\"Sistema de prueba\"");

    const auto frames = leer(cfg.salida + "_frames.csv");
    CHECK(frames.find("# escenario=prueba infectados=10 ventana=1280x720") != std::string::npos);
    CHECK(frames.find("t_s,ms_frame,ms_actualizar,ms_dibujar,pasos,ms_paso_max,us_busqueda_ia,reinicio\n") != std::string::npos);
    const auto segundos = leer(cfg.salida + "_segundos.csv");
    CHECK(segundos.find("t_s,cpu_pct,memoria_mb\n") != std::string::npos);
}

TEST_CASE("Solo arranque: sin segundos medidos termina en el primer frame", "[medicion]") {
    app::ConfigMedicion cfg;
    cfg.medicionSeg = 0.0;
    cfg.calentamientoSeg = 0.0;
    const app::MedicionRendimiento m(cfg);
    CHECK(m.terminado(0.0));
    CHECK(m.resumenTexto().find("Arranque") != std::string::npos);
}

TEST_CASE("La CPU y la memoria del proceso se leen del sistema", "[medicion]") {
    const auto antes = adaptadores::leerRecursosProceso();
    const auto t0 = std::chrono::steady_clock::now();
    volatile double x = 0.0;
    while (std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(150)) x = x + 1.0;
    const auto despues = adaptadores::leerRecursosProceso();
    CHECK(despues.cpuSegundos > antes.cpuSegundos);
    CHECK(despues.cpuSegundos - antes.cpuSegundos < 5.0);
#if defined(_WIN32) || defined(__linux__)
    CHECK(despues.memoriaBytes > 0);
    CHECK(despues.memoriaPicoBytes >= despues.memoriaBytes / 2);
    const double edad = adaptadores::segundosDesdeCreacionProceso();
    CHECK(edad >= 0.0);
    CHECK(edad < 24.0 * 3600.0);
#endif
}
