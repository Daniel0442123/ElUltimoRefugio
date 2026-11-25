// Optimización offline de los pesos de la IA (HU-21): RF-085, RF-086, RNF-020, CP-085.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "app/Cli.hpp"
#include "dominio/config/Configuracion.hpp"
#include "dominio/experimentos/Optimizacion.hpp"
#include "dominio/juego/Partida.hpp"
#include "dominio/nucleo/Json.hpp"
#include "dominio/persistencia/Crc32.hpp"
#include "dominio/persistencia/PartidaGuardada.hpp"

using namespace refugio;
using Catch::Approx;
using experimentos::Evaluador;

namespace {

/// Función sintética con máximo 0 en x = (0,3, 0,3, …): rápida y con óptimo conocido.
double esfera(const std::vector<double>& x) {
    double s = 0.0;
    for (const double v : x) s -= (v - 0.3) * (v - 0.3);
    return s;
}

Evaluador evaluadorEsfera(int* llamadas = nullptr) {
    return [llamadas](const std::vector<std::vector<double>>& lote) {
        if (llamadas) *llamadas += static_cast<int>(lote.size());
        std::vector<double> f;
        for (const auto& x : lote) f.push_back(esfera(x));
        return f;
    };
}

std::string leerTexto(const std::filesystem::path& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::filesystem::path carpetaPrueba(const std::string& nombre) {
    const auto d = std::filesystem::path(REFUGIO_DIR_SALIDA) / nombre;
    std::filesystem::remove_all(d);
    std::filesystem::create_directories(d);
    return d;
}

int codigoCli(std::vector<std::string> args) {
    args.insert(args.begin(), "refugio_bench");
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    int codigo = -1;
    REQUIRE(app::ejecutarModoSinVentana(static_cast<int>(argv.size()), argv.data(), codigo));
    return codigo;
}

/// Evaluación mínima con simulaciones reales para que las pruebas tarden poco.
experimentos::ConfigEvaluacion evaluacionCorta(int hilos = 1) {
    experimentos::ConfigEvaluacion e;
    e.simulaciones = 2;
    e.segundos = 15.0;
    e.hilos = hilos;
    return e;
}

config::PesosIa pesosDistintos() {
    config::PesosIa p;
    p.steeringSeparacion = 20.0f;
    p.utilidadExplorar = 0.3f;
    p.visionRadio = 4.5f;
    p.velPerseguir = 2.8f;
    p.danioPorSegundo = 7.0f;
    return p;
}

}  // namespace

// ----------------------------------------------------------------------------- Genes
TEST_CASE("Genes: uno por peso de ia.json y los valores por defecto dentro del rango", "[optimizacion][RF-085]") {
    const auto& gs = experimentos::genes();
    REQUIRE(gs.size() == config::kNumPesos);
    const config::PesosIa def;
    for (std::size_t i = 0; i < gs.size(); ++i) {
        INFO(experimentos::nombreGen(gs[i]));
        CHECK(gs[i].campo == &config::camposPesos()[i]);
        CHECK(gs[i].minimo < gs[i].maximo);
        // El rango de búsqueda cabe en el rango válido de ia.json y contiene el valor actual.
        CHECK(gs[i].minimo >= gs[i].campo->minimo);
        CHECK(gs[i].maximo <= gs[i].campo->maximo);
        CHECK(def.*(gs[i].campo->campo) >= gs[i].minimo);
        CHECK(def.*(gs[i].campo->campo) <= gs[i].maximo);
    }
    CHECK(experimentos::nombreGen(gs[0]) == "steering.seguir");
    CHECK(experimentos::nombreGen(gs.back()) == "infectados.danio_por_segundo");
}

TEST_CASE("Codificar y decodificar los pesos ida y vuelta, recortando a [0, 1]", "[optimizacion][RF-085]") {
    const auto p = pesosDistintos();
    const auto x = experimentos::codificar(p);
    for (const double v : x) {
        CHECK(v >= 0.0);
        CHECK(v <= 1.0);
    }
    const auto q = experimentos::decodificar(x);
    for (const auto& c : config::camposPesos()) CHECK(q.*c.campo == Approx(p.*c.campo).epsilon(1e-5));

    std::vector<double> fuera(x.size(), 2.0);
    fuera[0] = -1.0;
    const auto r = experimentos::decodificar(fuera);
    CHECK(r.steeringSeguir == Approx(experimentos::genes()[0].minimo));
    CHECK(r.danioPorSegundo == Approx(experimentos::genes().back().maximo));
}

// ---------------------------------------------------------------------------- CP-085 GA
TEST_CASE("CP-085 GA: el mejor nunca empeora y se acerca al optimo", "[optimizacion][CP-085]") {
    experimentos::ParametrosGa p;
    p.poblacion = 20;
    p.generaciones = 30;
    int llamadas = 0;
    const auto r = experimentos::ejecutarGa(p, 6, 11, evaluadorEsfera(&llamadas));
    REQUIRE(r.generaciones.size() == 30);
    CHECK(r.generaciones.front().generacion == 0);
    CHECK(r.generaciones.back().generacion == 29);
    // Elitismo de 2: los dos mejores pasan sin volver a evaluarse.
    CHECK(r.evaluaciones == 20 + 29 * 18);
    CHECK(llamadas == r.evaluaciones);
    for (std::size_t g = 1; g < r.generaciones.size(); ++g) {
        CHECK(r.generaciones[g].mejor >= r.generaciones[g - 1].mejor);
        CHECK(r.generaciones[g].media <= r.generaciones[g].mejor);
        CHECK(r.generaciones[g].peor <= r.generaciones[g].media);
    }
    CHECK(r.aptitudMejor == r.generaciones.back().mejor);
    CHECK(esfera(r.mejor) == r.aptitudMejor);
    CHECK(r.aptitudMejor > r.generaciones.front().mejor);
    CHECK(r.aptitudMejor > -0.02);  // cerca del óptimo 0
}

TEST_CASE("CP-085 GA: misma semilla mismo resultado; otra semilla otro", "[optimizacion][CP-085][RF-086]") {
    experimentos::ParametrosGa p;
    p.poblacion = 10;
    p.generaciones = 8;
    const auto a = experimentos::ejecutarGa(p, 5, 11, evaluadorEsfera());
    const auto b = experimentos::ejecutarGa(p, 5, 11, evaluadorEsfera());
    const auto c = experimentos::ejecutarGa(p, 5, 12, evaluadorEsfera());
    CHECK(a.mejor == b.mejor);
    CHECK(a.aptitudMejor == b.aptitudMejor);
    for (std::size_t g = 0; g < a.generaciones.size(); ++g) CHECK(a.generaciones[g].media == b.generaciones[g].media);
    CHECK(a.mejor != c.mejor);
}

TEST_CASE("GA: el individuo inicial entra en la poblacion y valida sus datos", "[optimizacion][RF-085]") {
    experimentos::ParametrosGa p;
    p.poblacion = 6;
    p.generaciones = 1;
    const std::vector<double> optimo(4, 0.3);
    const auto r = experimentos::ejecutarGa(p, 4, 5, evaluadorEsfera(), optimo);
    CHECK(r.aptitudMejor == 0.0);
    CHECK(r.mejor == optimo);
    CHECK(r.evaluaciones == 6);
    CHECK_THROWS(experimentos::ejecutarGa(p, 4, 5, evaluadorEsfera(), std::vector<double>(3, 0.5)));
    p.poblacion = 1;
    CHECK_THROWS(experimentos::ejecutarGa(p, 4, 5, evaluadorEsfera()));
}

// ---------------------------------------------------------------------------- CP-085 SA
TEST_CASE("CP-085 SA: registra temperatura, solucion actual y mejor por iteracion", "[optimizacion][CP-085]") {
    experimentos::ParametrosSa p;
    p.iteraciones = 1500;
    p.temperaturaInicial = 0.05;
    p.alfa = 0.995;
    const std::vector<double> inicio(6, 0.9);
    const auto r = experimentos::ejecutarSa(p, 6, 11, evaluadorEsfera(), inicio);
    REQUIRE(r.iteraciones.size() == 1501);
    CHECK(r.evaluaciones == 1501);
    CHECK(r.iteraciones[0].actual == inicio);
    CHECK(r.iteraciones[0].aptitudActual == esfera(inicio));
    double t = 0.05;
    for (std::size_t i = 1; i < r.iteraciones.size(); ++i) {
        const auto& it = r.iteraciones[i];
        CHECK(it.iteracion == static_cast<int>(i));
        CHECK(it.temperatura == Approx(t).epsilon(1e-9));  // enfriamiento geométrico
        t *= 0.995;
        CHECK(it.aptitudActual == esfera(it.actual));
        CHECK(it.aptitudMejor >= r.iteraciones[i - 1].aptitudMejor);
        CHECK(it.aptitudMejor >= it.aptitudActual);
        if (!it.aceptada) CHECK(it.actual == r.iteraciones[i - 1].actual);
    }
    CHECK(r.aptitudMejor == r.iteraciones.back().aptitudMejor);
    CHECK(esfera(r.mejor) == r.aptitudMejor);
    CHECK(r.aptitudMejor > -0.02);
    CHECK(r.aceptadas > 0);
}

TEST_CASE("SA: acepta empeoramientos con calor y es reproducible", "[optimizacion][CP-085][RF-086]") {
    experimentos::ParametrosSa caliente;
    caliente.iteraciones = 200;
    caliente.temperaturaInicial = 100.0;
    caliente.alfa = 1.0;
    const auto c = experimentos::ejecutarSa(caliente, 4, 3, evaluadorEsfera());
    CHECK(c.empeoramientosAceptados > 50);

    experimentos::ParametrosSa frio = caliente;
    frio.temperaturaInicial = 1e-9;
    const auto f = experimentos::ejecutarSa(frio, 4, 3, evaluadorEsfera());
    CHECK(f.empeoramientosAceptados == 0);
    for (std::size_t i = 1; i < f.iteraciones.size(); ++i)
        CHECK(f.iteraciones[i].aptitudActual >= f.iteraciones[i - 1].aptitudActual);

    const auto otra = experimentos::ejecutarSa(caliente, 4, 3, evaluadorEsfera());
    CHECK(otra.mejor == c.mejor);
    CHECK(otra.aceptadas == c.aceptadas);
    caliente.alfa = 1.5;
    CHECK_THROWS(experimentos::ejecutarSa(caliente, 4, 3, evaluadorEsfera()));
}

// ------------------------------------------------------------------------------ Aptitud
TEST_CASE("Aptitud con simulaciones: determinista e independiente de los hilos", "[optimizacion][RNF-020]") {
    const experimentos::PesosAptitud w;
    const auto a = experimentos::evaluarPesos(evaluacionCorta(1), w, 11, config::PesosIa{});
    const auto b = experimentos::evaluarPesos(evaluacionCorta(3), w, 11, config::PesosIa{});
    CHECK(a.simulaciones == 2);
    CHECK(a.aptitud == b.aptitud);
    CHECK(a.cobertura == b.cobertura);
    CHECK(a.tContactoMedio == b.tContactoMedio);
    CHECK(a.cobertura > 0.0);
    CHECK(a.cobertura <= 1.0);
    // Fórmula de ADR-0015 con los términos medidos.
    const double esperada = w.contacto * a.presion + w.cobertura * a.cobertura - w.sobrecarga * a.sobrecargadasPorTick -
                            w.dificultad * (std::max(0.0, w.victoriasMinimas - a.tasaVictorias) +
                                            std::max(0.0, a.tasaVictorias - w.victoriasMaximas));
    CHECK(a.aptitud == Approx(esperada));

    // El evaluador por lotes da lo mismo que la evaluación suelta, en cualquier orden del lote.
    const auto ev = experimentos::evaluadorSimulacion(evaluacionCorta(2), w, 11);
    const auto x0 = experimentos::codificar(config::PesosIa{});
    const auto x1 = experimentos::codificar(pesosDistintos());
    const auto f = ev({x1, x0});
    CHECK(f[1] == experimentos::evaluarPesos(evaluacionCorta(1), w, 11, experimentos::decodificar(x0)).aptitud);
    CHECK(f[0] == experimentos::evaluarPesos(evaluacionCorta(1), w, 11, experimentos::decodificar(x1)).aptitud);
}

TEST_CASE("Aptitud penaliza una horda demasiado dificil o demasiado facil", "[optimizacion][RF-085]") {
    experimentos::PesosAptitud w;
    w.contacto = w.cobertura = w.sobrecarga = 0.0;
    w.dificultad = 1.0;
    auto e = evaluacionCorta();
    e.segundos = 90.0;
    e.simulaciones = 3;
    // Por defecto el guion de Mara pierde siempre: 0 % de victorias, penalización 0,2.
    const auto dura = experimentos::evaluarPesos(e, w, 11, config::PesosIa{});
    CHECK(dura.victorias == 0);
    CHECK(dura.aptitud == Approx(-0.2));
    // Sin margen (mínimo = máximo = tasa medida) no hay penalización.
    w.victoriasMinimas = w.victoriasMaximas = 0.0;
    CHECK(experimentos::evaluarPesos(e, w, 11, config::PesosIa{}).aptitud == 0.0);
}

// ------------------------------------------------------------------------- config/opt
TEST_CASE("CP-085 lee opt/ga.json y opt/sa.json del repositorio", "[optimizacion][CP-085]") {
    const std::filesystem::path raiz(REFUGIO_DIR_FUENTES);
    const auto ga = experimentos::leerConfigOptimizacion(leerTexto(raiz / "opt" / "ga.json"));
    CHECK(ga.algoritmo == experimentos::AlgoritmoOptimizacion::Genetico);
    CHECK(ga.semilla == 11);
    CHECK(ga.ga.poblacion == 30);
    CHECK(ga.ga.generaciones == 40);
    CHECK(ga.ga.torneo == 3);
    CHECK(ga.ga.sigma == 0.1);
    CHECK(ga.ga.elites == 2);
    const auto sa = experimentos::leerConfigOptimizacion(leerTexto(raiz / "opt" / "sa.json"));
    CHECK(sa.algoritmo == experimentos::AlgoritmoOptimizacion::Recocido);
    CHECK(sa.semilla == 11);
    CHECK(sa.sa.iteraciones == 2000);
    CHECK(sa.sa.alfa == 0.995);
}

TEST_CASE("Configuracion de optimizacion invalida da un error claro", "[optimizacion][RF-085]") {
    using experimentos::leerConfigOptimizacion;
    CHECK_THROWS_AS(leerConfigOptimizacion(R"({"poblacion": 30})"), nucleo::ErrorJson);
    CHECK_THROWS_AS(leerConfigOptimizacion(R"({"algoritmo": "pso"})"), nucleo::ErrorJson);
    CHECK_THROWS_AS(leerConfigOptimizacion(R"({"ga": {"poblacion": 5, "torneo": 6}})"), nucleo::ErrorJson);
    CHECK_THROWS_AS(leerConfigOptimizacion(R"({"ga": {"poblacion": 4, "elites": 4}})"), nucleo::ErrorJson);
    CHECK_THROWS_AS(leerConfigOptimizacion(R"({"sa": {"alfa": 0}})"), nucleo::ErrorJson);
    CHECK_THROWS_AS(leerConfigOptimizacion(R"({"evaluacion": {"segundos": -1}})"), nucleo::ErrorJson);
    CHECK_THROWS_AS(leerConfigOptimizacion(R"({"aptitud": {"victorias_minimas": 0.5, "victorias_maximas": 0.4}})"),
                    nucleo::ErrorJson);
    CHECK_THROWS_AS(leerConfigOptimizacion("{"), nucleo::ErrorJson);
    CHECK(leerConfigOptimizacion("{}").ga.poblacion == 30);
}

// --------------------------------------------------------------------- JSON resultado
TEST_CASE("CP-085 GA con simulaciones escribe el JSON con el mejor y la evolucion", "[optimizacion][CP-085]") {
    experimentos::ConfigOptimizacion cfg;
    cfg.ga.poblacion = 4;
    cfg.ga.generaciones = 3;
    cfg.evaluacion = evaluacionCorta(2);
    std::ostringstream out;
    const auto r = experimentos::ejecutarOptimizacion(cfg, {"juego=prueba \"comillas\""}, out);
    CHECK(r.mejorNoDecreciente);
    CHECK(r.evaluaciones == 4 + 2 * 2);
    CHECK(r.simulaciones == (r.evaluaciones + 2) * 2);
    CHECK(r.mejor.aptitud >= r.base.aptitud);  // los pesos iniciales están en la población

    const auto doc = nucleo::leerJson(out.str());
    CHECK(doc.buscar("algoritmo")->comoCadena() == "ga");
    CHECK(doc.buscar("semilla")->comoNumero() == 11.0);
    CHECK(doc.buscar("metadatos")->comoArreglo()[0].comoCadena() == "juego=prueba \"comillas\"");
    CHECK(doc.buscar("genes")->comoArreglo().size() == config::kNumPesos);
    const auto& gens = doc.buscar("generaciones")->comoArreglo();
    REQUIRE(gens.size() == 3);
    for (std::size_t g = 1; g < gens.size(); ++g)
        CHECK(gens[g].buscar("mejor")->comoNumero() >= gens[g - 1].buscar("mejor")->comoNumero());
    CHECK(gens[0].buscar("media") != nullptr);
    const auto* mejor = doc.buscar("mejor");
    REQUIRE(mejor != nullptr);
    CHECK(mejor->buscar("aptitud")->comoNumero() == Approx(r.mejor.aptitud));

    // La sección «pesos» se puede pegar tal cual en config/ia.json.
    std::ostringstream ia;
    ia << "{\"pesos\": " << config::escribirPesos(r.pesosMejor) << "}";
    const auto leida = config::leerParametrosIa(ia.str());
    CHECK(leida.valida());
    for (const auto& c : config::camposPesos())
        CHECK(leida.valor.pesos.*c.campo == Approx(r.pesosMejor.*c.campo).epsilon(1e-5));
}

TEST_CASE("CP-085 SA con simulaciones escribe temperatura y soluciones por iteracion", "[optimizacion][CP-085]") {
    experimentos::ConfigOptimizacion cfg;
    cfg.algoritmo = experimentos::AlgoritmoOptimizacion::Recocido;
    cfg.sa.iteraciones = 4;
    cfg.evaluacion = evaluacionCorta(2);
    std::ostringstream out;
    const auto r = experimentos::ejecutarOptimizacion(cfg, {}, out);
    CHECK(r.evaluaciones == 5);
    const auto doc = nucleo::leerJson(out.str());
    CHECK(doc.buscar("algoritmo")->comoCadena() == "sa");
    const auto& its = doc.buscar("iteraciones")->comoArreglo();
    REQUIRE(its.size() == 5);
    for (const auto& it : its) {
        CHECK(it.buscar("temperatura")->comoNumero() > 0.0);
        CHECK(it.buscar("solucion_actual")->comoArreglo().size() == config::kNumPesos);
        CHECK(it.buscar("mejor")->comoNumero() >= it.buscar("actual")->comoNumero());
    }
    CHECK(doc.buscar("mejor")->buscar("pesos")->buscar("infectados") != nullptr);
}

TEST_CASE("CP-085 refugio --optimize escribe results/opt_<alg>_<semilla>.json", "[optimizacion][CP-085]") {
    const auto dir = carpetaPrueba("optimizacion");
    const auto config = dir / "sa_corto.json";
    {
        std::ofstream f(config);
        f << R"({"algoritmo": "ga", "semilla": 5, "sa": {"iteraciones": 2},
                 "evaluacion": {"simulaciones": 1, "segundos": 10, "hilos": 1}})";
    }
    const auto salida = dir / "opt.json";
    // --optimize manda sobre el «algoritmo» del archivo.
    CHECK(codigoCli({"--optimize", "sa", "--config", config.string(), "--salida", salida.string()}) == 0);
    const auto doc = nucleo::leerJson(leerTexto(salida));
    CHECK(doc.buscar("algoritmo")->comoCadena() == "sa");
    CHECK(doc.buscar("semilla")->comoNumero() == 5.0);
    CHECK(doc.buscar("iteraciones")->comoArreglo().size() == 3);

    CHECK(codigoCli({"--optimize", "pso"}) == 1);
    CHECK(codigoCli({"--optimize", "ga", "--config", (dir / "no_existe.json").string()}) == 1);
}

// ------------------------------------------------------------------------- ia.json
TEST_CASE("ia.json: la seccion pesos se lee, se valida y se aplica", "[optimizacion][configuracion][RF-092][RF-093]") {
    // Los pesos por defecto son los de ConfigPartida: aplicarlos no cambia nada.
    const juego::ConfigPartida def;
    juego::ConfigPartida cfg;
    config::aplicar(config::ParametrosIa{}, cfg);
    CHECK(cfg.steering.pesoSeguir == def.steering.pesoSeguir);
    CHECK(cfg.steering.pesoSeparacion == def.steering.pesoSeparacion);
    CHECK(cfg.steering.pesoAlineacion == def.steering.pesoAlineacion);
    CHECK(cfg.steering.pesoCohesion == def.steering.pesoCohesion);
    CHECK(cfg.steering.pesoAnticipacion == def.steering.pesoAnticipacion);
    CHECK(cfg.tactica.utilidadExplorar == def.tactica.utilidadExplorar);
    CHECK(cfg.tactica.pesoInterceptar == def.tactica.pesoInterceptar);
    CHECK(cfg.histeresisTactica == def.histeresisTactica);
    CHECK(cfg.visionRadio == def.visionRadio);
    CHECK(cfg.velPerseguir == def.velPerseguir);
    CHECK(cfg.danioPorSegundo == def.danioPorSegundo);

    const auto l = config::leerParametrosIa(
        R"({"pesos": {"steering": {"separacion": 20}, "infectados": {"vel_perseguir": 2.8, "vision_radio": 99}}})");
    REQUIRE(l.errores.size() == 1);  // vision_radio fuera de rango
    CHECK(l.errores[0].find("pesos.infectados.vision_radio") != std::string::npos);
    CHECK(l.valor.pesos.steeringSeparacion == 20.0f);
    CHECK(l.valor.pesos.velPerseguir == 2.8f);
    CHECK(l.valor.pesos.visionRadio == config::PesosIa{}.visionRadio);
    config::aplicar(l.valor, cfg);
    CHECK(cfg.steering.pesoSeparacion == 20.0f);
    CHECK(cfg.velPerseguir == 2.8f);

    const auto mal = config::leerParametrosIa(R"({"pesos": {"tactica": 3}})");
    REQUIRE(mal.errores.size() == 1);
    CHECK(mal.errores[0].find("pesos.tactica") != std::string::npos);
    CHECK(mal.valor == config::ParametrosIa{});

    // El config/ia.json del repositorio se lee sin errores y trae los pesos por defecto.
    const auto repo = config::leerParametrosIa(leerTexto(std::filesystem::path(REFUGIO_DIR_FUENTES) / "config" / "ia.json"));
    CHECK(repo.valida());
    CHECK(repo.valor.pesos == config::PesosIa{});
}

// ----------------------------------------------------------------------- Persistencia
TEST_CASE("Partida guardada conserva los pesos y lee el formato 1", "[optimizacion][persistencia][RF-091]") {
    persistencia::InicioPartida in;
    in.semilla = 3;
    in.ia.pesos = pesosDistintos();
    auto p = persistencia::crearPartida(in);
    persistencia::GrabadorPartida g(in);
    for (int i = 0; i < 240; ++i) {
        juego::EntradaJugador e;
        e.ejeX = i < 120 ? 1.0f : 0.0f;
        e.ejeY = i < 120 ? 0.0f : 1.0f;
        p->paso(g.registrar(e, *p));
    }
    CHECK(p->config().velPerseguir == 2.8f);
    const auto bytes = persistencia::codificar(persistencia::instantanea(g, *p));
    CHECK((bytes[4] | (bytes[5] << 8)) == 2);
    const auto r = persistencia::cargar({bytes.data(), bytes.size()});
    REQUIRE(r.valida());
    CHECK(r.partida->config().velPerseguir == 2.8f);
    CHECK(persistencia::huellaPartida(*r.partida) == persistencia::huellaPartida(*p));

    // Formato 1 (0.12.0): igual pero sin los 11 pesos; se carga con los pesos por defecto.
    persistencia::InicioPartida in1 = in;
    in1.ia.pesos = {};
    auto p1 = persistencia::crearPartida(in1);
    persistencia::GrabadorPartida g1(in1);
    for (int i = 0; i < 60; ++i) p1->paso(g1.registrar(juego::EntradaJugador{}, *p1));
    auto v2 = persistencia::codificar(persistencia::instantanea(g1, *p1));
    const std::size_t lv = v2[persistencia::kTamCabecera];
    // carga: versión del juego, semilla, 4 u16, 2 u8, 2 u16 + 18 f32 + 4 u32 de ia; luego los pesos.
    const std::size_t inicioPesos = persistencia::kTamCabecera + 1 + lv + 8 + 8 + 2 + 4 + 18 * 4 + 4 * 4;
    std::vector<std::uint8_t> v1(v2.begin(), v2.begin() + static_cast<std::ptrdiff_t>(inicioPesos));
    v1.insert(v1.end(), v2.begin() + static_cast<std::ptrdiff_t>(inicioPesos + config::kNumPesos * 4), v2.end());
    v1[4] = 1;
    v1[5] = 0;
    const auto largo = static_cast<std::uint32_t>(v1.size() - persistencia::kTamCabecera);
    for (int i = 0; i < 4; ++i) v1[8 + i] = static_cast<std::uint8_t>(largo >> (8 * i));
    const auto crc = persistencia::crc32({v1.data() + persistencia::kTamCabecera, v1.size() - persistencia::kTamCabecera});
    for (int i = 0; i < 4; ++i) v1[12 + i] = static_cast<std::uint8_t>(crc >> (8 * i));
    const auto d = persistencia::decodificar({v1.data(), v1.size()});
    REQUIRE(d.valida());
    CHECK(d.datos.inicio.ia.pesos == config::PesosIa{});
    const auto c1 = persistencia::cargar({v1.data(), v1.size()});
    REQUIRE(c1.valida());
    CHECK(persistencia::huellaPartida(*c1.partida) == persistencia::huellaPartida(*p1));
}
