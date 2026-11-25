// Comparar agentes aislados y coordinados (HU-18): RF-056, RF-084.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "app/Cli.hpp"
#include "dominio/experimentos/ExperimentoCoordinacion.hpp"
#include "dominio/juego/Partida.hpp"

using namespace refugio;
using experimentos::ConfigCoordinacion;
using experimentos::GuionJugadora;
using experimentos::ModoHorda;
using juego::Partida;

namespace {

std::vector<std::string> separar(const std::string& linea) {
    std::vector<std::string> v;
    std::stringstream ss(linea);
    std::string campo;
    while (std::getline(ss, campo, ',')) v.push_back(campo);
    return v;
}

struct Csv {
    std::vector<std::string> columnas;
    std::vector<std::map<std::string, std::string>> filas;
    std::string datos;  ///< Todo lo que no es cabecera de comentarios.
};

Csv leer(std::istream& in) {
    Csv c;
    std::string linea;
    while (std::getline(in, linea)) {
        if (linea.empty() || linea[0] == '#') continue;
        c.datos += linea + '\n';
        if (c.columnas.empty()) {
            c.columnas = separar(linea);
            continue;
        }
        const auto campos = separar(linea);
        std::map<std::string, std::string> f;
        for (std::size_t i = 0; i < campos.size() && i < c.columnas.size(); ++i) f[c.columnas[i]] = campos[i];
        f["__campos"] = std::to_string(campos.size());
        c.filas.push_back(std::move(f));
    }
    return c;
}

Csv ejecutar(const ConfigCoordinacion& cfg, experimentos::ResumenCoordinacion* resumen = nullptr) {
    std::stringstream ss;
    auto r = experimentos::ejecutarExperimentoCoordinacion(cfg, {"prueba=1"}, ss);
    if (resumen) *resumen = r;
    return leer(ss);
}

Partida crear(std::uint64_t semilla, bool coordinado, int infectados = 10) {
    juego::ConfigPartida cp;
    cp.coordinado = coordinado;
    return Partida(juego::crearEscenarioAleatorio(32, 22, infectados, semilla), cp, semilla);
}

bool algunoVe(const Partida& p) {
    return std::any_of(p.infectados().begin(), p.infectados().end(),
                       [](const juego::Infectado& z) { return z.percibido.veAMara; });
}

int codigoCli(std::vector<std::string> args) {
    args.insert(args.begin(), "refugio_bench");
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    int codigo = -1;
    REQUIRE(app::ejecutarModoSinVentana(static_cast<int>(argv.size()), argv.data(), codigo));
    return codigo;
}

}  // namespace

// ------------------------------------------------------------------------------- RF-056
TEST_CASE("CP-056 Modo aislado sin alertas ni Contract Net e identico hasta la primera deteccion", "[RF-056][HU-18]") {
    for (std::uint64_t semilla : {7ULL, 8ULL, 13ULL}) {
        Partida aislada = crear(semilla, false);
        Partida coordinada = crear(semilla, true);
        GuionJugadora ga(aislada);
        GuionJugadora gc(coordinada);
        bool detectado = false;
        std::uint64_t ticksIguales = 0;
        for (int t = 0; t < 60 * 60 && aislada.estado() == juego::EstadoPartida::EnCurso &&
                        coordinada.estado() == juego::EstadoPartida::EnCurso;
             ++t) {
            aislada.paso(ga.entrada(aislada));
            coordinada.paso(gc.entrada(coordinada));
            if (!detectado) {
                // La percepción no depende de los mensajes: los dos detectan en el mismo tick.
                REQUIRE(algunoVe(aislada) == algunoVe(coordinada));
                detectado = algunoVe(aislada);
                // Hasta que alguien ve a Mara no hay nada que comunicar: el estado es el mismo. En el
                // tick de la detección ya puede diferir, porque la alerta llega en ese mismo ciclo de IA
                // a los infectados que aún no han decidido.
                if (!detectado) {
                    REQUIRE(experimentos::hashEstado(aislada) == experimentos::hashEstado(coordinada));
                    ++ticksIguales;
                }
            }
        }
        INFO("semilla " << semilla);
        CHECK(detectado);
        CHECK(ticksIguales > 60);  // al menos un segundo de exploración antes de la detección
        CHECK(aislada.alertasEmitidas() == 0);
        CHECK(aislada.mensajesAlerta() == 0);
        CHECK(aislada.subastas() == 0);
        CHECK(aislada.mensajesContractNet() == 0);
        CHECK(aislada.totalMensajes() == 0);
        CHECK(coordinada.alertasEmitidas() > 0);
        CHECK(coordinada.totalMensajes() > 0);
    }
}

TEST_CASE("CP-056 F4 en vivo corta la comunicacion a partir de ese momento", "[RF-056][HU-18]") {
    Partida p = crear(7, true);
    GuionJugadora g(p);
    while (p.alertasEmitidas() == 0 && p.tick() < 60 * 60) p.paso(g.entrada(p));
    REQUIRE(p.alertasEmitidas() > 0);
    p.fijarCoordinado(false);  // lo que hace F4 en la partida
    CHECK_FALSE(p.config().coordinado);
    const auto alertas = p.alertasEmitidas();
    const auto mensajes = p.totalMensajes();
    const auto subastas = p.subastas();
    for (int t = 0; t < 600 && p.estado() == juego::EstadoPartida::EnCurso; ++t) p.paso(g.entrada(p));
    CHECK(p.alertasEmitidas() == alertas);
    CHECK(p.totalMensajes() == mensajes);
    CHECK(p.subastas() == subastas);
    p.fijarCoordinado(true);
    CHECK(p.config().coordinado);
}

// ------------------------------------------------------------------------------- RF-084
TEST_CASE("CP-084 Experimento con semilla 7 y 10 repeticiones da 20 simulaciones pareadas", "[RF-084][HU-18]") {
    ConfigCoordinacion cfg;
    cfg.semilla = 7;
    cfg.repeticiones = 10;
    experimentos::ResumenCoordinacion r;
    const Csv csv = ejecutar(cfg, &r);
    const auto& cols = experimentos::columnasCoordinacion();
    REQUIRE(csv.columnas.size() == cols.size());
    for (std::size_t i = 0; i < cols.size(); ++i) CHECK(csv.columnas[i] == cols[i]);
    for (const char* c : {"t_primera_deteccion_s", "t_contacto_s", "cobertura", "mensajes", "celdas_sobrecargadas"})
        CHECK(std::find(csv.columnas.begin(), csv.columnas.end(), c) != csv.columnas.end());
    REQUIRE(csv.filas.size() == 20);
    REQUIRE(r.simulaciones.size() == 20);
    CHECK(r.aislado.simulaciones == 10);
    CHECK(r.coordinado.simulaciones == 10);
    CHECK(r.paresDivergentes == 0);
    for (std::size_t i = 0; i < csv.filas.size(); i += 2) {
        const auto& a = csv.filas[i];
        const auto& c = csv.filas[i + 1];
        CHECK(a.at("__campos") == std::to_string(cols.size()));
        CHECK(a.at("modo") == "aislado");
        CHECK(c.at("modo") == "coordinado");
        CHECK(a.at("semilla") == std::to_string(7 + i / 2));  // misma semilla en el par
        CHECK(c.at("semilla") == a.at("semilla"));
        CHECK(a.at("t_primera_deteccion_s") == c.at("t_primera_deteccion_s"));
        CHECK(a.at("hash_previo_deteccion") == c.at("hash_previo_deteccion"));
        CHECK(a.at("mensajes") == "0");
        CHECK(a.at("alertas") == "0");
        const double cob = std::stod(a.at("cobertura"));
        CHECK(cob > 0.0);
        CHECK(cob <= 1.0);
    }
    // La horda coordinada se comunica y llega a rodear a Mara con más infectados.
    CHECK(r.coordinado.mediaMensajes > 0.0);
    CHECK(r.aislado.mediaMensajes == 0.0);
    CHECK(r.coordinado.mediaMaxCerca > r.aislado.mediaMaxCerca);
}

TEST_CASE("CP-084 El experimento es reproducible con la misma semilla", "[RF-084][RNF-020][HU-18]") {
    ConfigCoordinacion cfg;
    cfg.semilla = 21;
    cfg.repeticiones = 3;
    cfg.segundos = 40.0;
    const Csv a = ejecutar(cfg);
    const Csv b = ejecutar(cfg);
    CHECK(a.datos == b.datos);
    cfg.semilla = 22;
    const Csv c = ejecutar(cfg);
    CHECK(c.datos != a.datos);
}

TEST_CASE("CP-084 El tope de segundos corta cada simulacion", "[RF-084][HU-18]") {
    ConfigCoordinacion cfg;
    cfg.repeticiones = 1;
    cfg.segundos = 2.0;
    const auto m = experimentos::simularHorda(cfg, ModoHorda::Coordinado, 0);
    CHECK(m.ticks <= 120);
    CHECK(m.duracion <= 2.0 + 1e-4);
}

TEST_CASE("CP-084 El guion de la jugadora recoge los objetos y activa la radio sin infectados", "[RF-084][HU-18]") {
    Partida p = crear(7, true, 0);
    GuionJugadora g(p);
    while (p.estado() == juego::EstadoPartida::EnCurso && p.tick() < 60 * 180) p.paso(g.entrada(p));
    CHECK(p.tieneTodosLosObjetos());
    CHECK(p.activacionesRadio() > 0);
    CHECK(p.estado() == juego::EstadoPartida::Victoria);
    CHECK(g.meta() == p.objetos().size() + 1);
}

TEST_CASE("CP-084 La huella de estado cambia con la simulacion y coincide con la misma semilla", "[RF-084][RNF-020]") {
    Partida a = crear(9, true);
    Partida b = crear(9, true);
    CHECK(experimentos::hashEstado(a) == experimentos::hashEstado(b));
    const auto inicial = experimentos::hashEstado(a);
    a.paso({});
    CHECK(experimentos::hashEstado(a) != inicial);
    b.paso({});
    CHECK(experimentos::hashEstado(a) == experimentos::hashEstado(b));
}

TEST_CASE("CP-084 refugio --coord-exp escribe el CSV y termina con codigo 0", "[RF-084][HU-18]") {
    std::filesystem::create_directories(REFUGIO_DIR_SALIDA);
    const std::string ruta = std::string(REFUGIO_DIR_SALIDA) + "/coord_cli.csv";
    std::filesystem::remove(ruta);
    CHECK(codigoCli({"--coord-exp", "7", "--reps", "2", "--segundos", "30", "--salida", ruta}) == 0);
    std::ifstream f(ruta);
    REQUIRE(f);
    std::stringstream todo;
    todo << f.rdbuf();
    const std::string texto = todo.str();
    CHECK(texto.find("# juego=ElUltimoRefugio") != std::string::npos);
    CHECK(texto.find("semilla_maestra=7 repeticiones=2") != std::string::npos);
    std::istringstream in(texto);
    const Csv csv = leer(in);
    CHECK(csv.filas.size() == 4);
    CHECK(codigoCli({"--coord-exp", "7", "--reps", "0", "--salida", ruta}) == 1);  // valores no válidos
}
