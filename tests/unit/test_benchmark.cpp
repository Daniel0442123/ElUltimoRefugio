// Benchmark reproducible de búsqueda (HU-17): RF-080 a RF-083, RF-086.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "app/Cli.hpp"
#include "dominio/experimentos/Benchmark.hpp"
#include "dominio/nucleo/Json.hpp"

using namespace refugio;
using experimentos::ConfigBenchmark;
using navegacion::Algoritmo;

namespace {

struct Csv {
    std::vector<std::string> meta;
    std::vector<std::string> columnas;
    std::vector<std::map<std::string, std::string>> filas;
};

std::vector<std::string> separar(const std::string& linea) {
    std::vector<std::string> v;
    std::stringstream ss(linea);
    std::string campo;
    while (std::getline(ss, campo, ',')) v.push_back(campo);
    return v;
}

Csv leerCsv(std::istream& in) {
    Csv c;
    std::string linea;
    while (std::getline(in, linea)) {
        if (linea.empty()) continue;
        if (linea[0] == '#') {
            c.meta.push_back(linea);
        } else if (c.columnas.empty()) {
            c.columnas = separar(linea);
        } else {
            const auto campos = separar(linea);
            std::map<std::string, std::string> f;
            for (std::size_t i = 0; i < campos.size() && i < c.columnas.size(); ++i) f[c.columnas[i]] = campos[i];
            f["__campos"] = std::to_string(campos.size());
            c.filas.push_back(std::move(f));
        }
    }
    return c;
}

Csv leerCsv(const std::string& texto) {
    std::istringstream in(texto);
    return leerCsv(in);
}

ConfigBenchmark pequena() {
    ConfigBenchmark c;
    c.tamanos = {32, 48};
    c.pares = 6;
    c.repeticiones = 2;
    return c;
}

/// Buscador defectuoso para CP-082: delega en A* y suma 1 al costo.
class AEstrellaDefectuoso final : public navegacion::IBuscadorRuta {
public:
    void buscarEn(const mundo::GrafoRejilla& g, mundo::Nodo o, mundo::Nodo d, navegacion::ResultadoBusqueda& r,
                  bool registrar) override {
        real_.buscarEn(g, o, d, r, registrar);
        if (r.encontrada) r.costo += 1.0;
    }
    [[nodiscard]] Algoritmo algoritmo() const noexcept override { return Algoritmo::AEstrella; }

private:
    navegacion::BuscadorMejorPrimero real_{Algoritmo::AEstrella};
};

int ejecutarCli(std::vector<std::string> args) {
    args.insert(args.begin(), "refugio_bench");
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    int codigo = -1;
    REQUIRE(app::ejecutarModoSinVentana(static_cast<int>(argv.size()), argv.data(), codigo));
    return codigo;
}

std::string salida(const std::string& nombre) {
    std::filesystem::create_directories(REFUGIO_DIR_SALIDA);
    return std::string(REFUGIO_DIR_SALIDA) + "/" + nombre;
}

}  // namespace

TEST_CASE("CP-080 refugio --bench bench_mini.json escribe 800 filas con las columnas de RF-080", "[RF-080][RF-082][HU-17]") {
    const std::string csv = salida("cp080.csv");
    std::filesystem::remove(csv);
    const int codigo = ejecutarCli({"--bench", std::string(REFUGIO_DATOS_PRUEBA) + "/bench_mini.json", "--salida", csv});
    CHECK(codigo == 0);  // sin discrepancias de optimalidad
    std::ifstream f(csv);
    REQUIRE(f);
    const auto c = leerCsv(f);
    CHECK(c.filas.size() == 2u * 4u * 20u * 5u);
    for (const char* col : {"algoritmo", "mapa", "ancho", "alto", "densidad_obstaculos", "semilla", "repeticion",
                            "origen_x", "origen_y", "destino_x", "destino_y", "costo", "longitud", "nodos_expandidos",
                            "nodos_generados", "max_abierta", "tiempo_us", "memoria_pico_bytes", "replanificaciones"}) {
        CHECK(std::find(c.columnas.begin(), c.columnas.end(), col) != c.columnas.end());
    }
    std::map<std::string, int> porAlgoritmo;
    std::map<std::string, int> porTamano;
    for (const auto& r : c.filas) {
        REQUIRE(r.at("__campos") == std::to_string(c.columnas.size()));  // filas completas (CP-132)
        ++porAlgoritmo[r.at("algoritmo")];
        ++porTamano[r.at("ancho")];
        CHECK(r.at("ancho") == r.at("alto"));
        CHECK(r.at("mapa") == "ciudad_" + r.at("ancho"));
        CHECK(r.at("replanificaciones") == "0");
    }
    CHECK(porAlgoritmo["Dijkstra"] == 400);
    CHECK(porAlgoritmo["AEstrella"] == 400);
    for (const char* t : {"32", "64", "128", "256"}) CHECK(porTamano[t] == 200);
}

TEST_CASE("CP-081 La cabecera registra equipo, compilacion, version, commit, fecha, semilla y config", "[RF-081][HU-17]") {
    std::ostringstream out;
    auto cfg = pequena();
    cfg.archivoConfig = "bench/config.json";
    cfg.semilla = 7;
    (void)experimentos::ejecutarBenchmark(cfg, app::metadatosEquipo(), out);
    const auto c = leerCsv(out.str());
    std::string cabecera;
    for (const auto& m : c.meta) cabecera += m + "\n";
    for (const char* clave : {"version=", "commit=", "cpu=", "nucleos_logicos=", "ram_mb=", "so=", "compilador=",
                              "opciones=", "fecha_utc=", "semilla_maestra=7", "config=bench/config.json",
                              "escenario=estatico"}) {
        INFO(clave);
        CHECK(cabecera.find(clave) != std::string::npos);
    }
    CHECK(cabecera.find("version=" REFUGIO_VERSION) != std::string::npos);
}

TEST_CASE("CP-082 Un A* defectuoso que suma 1 al costo se reporta como discrepancia", "[RF-082][HU-17]") {
    std::ostringstream out;
    const auto cfg = pequena();
    const auto bueno = experimentos::ejecutarBenchmark(cfg, {}, out);
    CHECK(bueno.discrepanciasOptimalidad == 0);
    CHECK(bueno.aEstrellaExpandioMas == 0);  // heurística consistente: nunca expande más

    std::ostringstream out2;
    const auto malo = experimentos::ejecutarBenchmark(cfg, {}, out2, [](Algoritmo a) -> std::unique_ptr<navegacion::IBuscadorRuta> {
        if (a == Algoritmo::AEstrella) return std::make_unique<AEstrellaDefectuoso>();
        return navegacion::crearBuscador(a);
    });
    CHECK(malo.mediciones == bueno.mediciones);
    // Toda consulta con ruta es una discrepancia (las de sin ruta coinciden).
    CHECK(malo.discrepanciasOptimalidad == malo.mediciones / 2 - malo.consultasSinRuta);
    CHECK(malo.discrepanciasOptimalidad > 0);
}

TEST_CASE("CP-080 Misma configuracion y semilla 42 dan costos y nodos identicos", "[RF-080][RF-086][HU-17]") {
    auto correr = [](std::uint64_t semilla) {
        auto cfg = pequena();
        cfg.semilla = semilla;
        std::ostringstream out;
        (void)experimentos::ejecutarBenchmark(cfg, {}, out);
        return leerCsv(out.str());
    };
    const auto a = correr(42);
    const auto b = correr(42);
    const auto otra = correr(43);
    REQUIRE(a.filas.size() == b.filas.size());
    REQUIRE(a.filas.size() == 2u * 2u * 6u * 2u);
    bool distinta = false;
    for (std::size_t i = 0; i < a.filas.size(); ++i) {
        for (const char* col : {"algoritmo", "semilla", "origen_x", "origen_y", "destino_x", "destino_y", "costo",
                                "longitud", "nodos_expandidos", "nodos_generados", "max_abierta", "memoria_pico_bytes"}) {
            CHECK(a.filas[i].at(col) == b.filas[i].at(col));
        }
        if (a.filas[i].at("costo") != otra.filas[i].at("costo") || a.filas[i].at("origen_x") != otra.filas[i].at("origen_x"))
            distinta = true;
    }
    CHECK(distinta);  // otra semilla, otros mapas y pares
    // Las dos repeticiones de una misma consulta dan lo mismo salvo el tiempo.
    CHECK(a.filas[0].at("costo") == a.filas[2].at("costo"));
    CHECK(a.filas[0].at("nodos_expandidos") == a.filas[2].at("nodos_expandidos"));
}

TEST_CASE("CP-083 Barricada en el tick 30 sobre la ruta: ambos algoritmos replanifican", "[RF-083][RF-025][HU-17]") {
    auto cfg = pequena();
    cfg.dinamico.activo = true;
    cfg.dinamico.tamanos = {64};
    cfg.pares = 8;
    std::ostringstream out;
    const auto r = experimentos::ejecutarBenchmarkDinamico(cfg, {}, out);
    const auto c = leerCsv(out.str());
    REQUIRE(r.filas == c.filas.size());
    REQUIRE(r.filas == 2u * 8u * 2u);  // 8 pares x 2 repeticiones x 2 algoritmos
    CHECK(r.replanificaciones[0] >= 8);
    CHECK(r.replanificaciones[1] >= 8);
    CHECK(r.sinReplanificar[0] == 0);
    CHECK(r.sinReplanificar[1] == 0);
    CHECK(r.noLlegaron == 0);
    std::string cabecera;
    for (const auto& m : c.meta) cabecera += m + "\n";
    CHECK(cabecera.find("escenario=dinamico") != std::string::npos);
    CHECK(cabecera.find("evento tick=30 adelante=4 multiplicador=0") != std::string::npos);
    for (const auto& f : c.filas) {
        CHECK(f.at("mapa") == "ciudad_64_dinamico");
        CHECK(std::stoi(f.at("replanificaciones")) >= 1);
        CHECK(f.at("eventos_aplicados") == "1");
        CHECK(f.at("encontrada") == "1");
        // Rodear la barricada nunca sale más barato que la ruta planificada al principio.
        CHECK(std::stod(f.at("costo")) >= std::stod(f.at("costo_inicial")) - 1e-6);
        CHECK(std::stoi(f.at("longitud")) >= 30);
    }
}

TEST_CASE("CP-083 Un evento en una celda fija es el mismo para los dos algoritmos", "[RF-083]") {
    auto cfg = pequena();
    cfg.dinamico.activo = true;
    cfg.dinamico.tamanos = {48};
    cfg.dinamico.eventos = {{5, 40, 40, 4, 0}, {12, -1, -1, 2, 9}};  // barricada fija y terreno caro delante
    std::ostringstream out;
    const auto r = experimentos::ejecutarBenchmarkDinamico(cfg, {}, out);
    const auto c = leerCsv(out.str());
    REQUIRE(c.filas.size() == r.filas);
    REQUIRE_FALSE(c.filas.empty());
    for (std::size_t i = 0; i + 1 < c.filas.size(); i += 2) {
        // Las dos filas de una consulta son Dijkstra y A* en orden alterno, con el mismo origen y destino.
        CHECK(c.filas[i].at("algoritmo") != c.filas[i + 1].at("algoritmo"));
        CHECK(c.filas[i].at("origen_x") == c.filas[i + 1].at("origen_x"));
        CHECK(c.filas[i].at("destino_y") == c.filas[i + 1].at("destino_y"));
        CHECK(std::abs(std::stod(c.filas[i].at("costo_inicial")) - std::stod(c.filas[i + 1].at("costo_inicial"))) < 1e-6);
    }
}

TEST_CASE("CP-080 --dinamico escribe un segundo CSV junto al principal", "[RF-080][RF-083]") {
    const std::string csv = salida("cli_dinamico.csv");
    const std::string din = salida("cli_dinamico_dinamico.csv");
    std::filesystem::remove(csv);
    std::filesystem::remove(din);
    CHECK(ejecutarCli({"--bench", "--tamanos", "32", "--pares", "3", "--reps", "1", "--dinamico", "--salida", csv}) == 0);
    CHECK(std::filesystem::exists(csv));
    REQUIRE(std::filesystem::exists(din));
    std::ifstream f(din);
    const auto c = leerCsv(f);
    CHECK_FALSE(c.filas.empty());
    CHECK(c.filas.front().at("mapa") == "ciudad_64_dinamico");  // tamaños por defecto del escenario dinámico
}

TEST_CASE("bench/config.json se lee con sus valores y las claves ausentes toman el defecto", "[RF-080][HU-17]") {
    std::ifstream f(std::string(REFUGIO_DATOS_PRUEBA) + "/../../bench/config.json");
    REQUIRE(f);
    std::stringstream ss;
    ss << f.rdbuf();
    const auto cfg = experimentos::leerConfigBenchmark(ss.str());
    CHECK(cfg.tamanos == std::vector<int>{32, 64, 128, 256});
    CHECK(cfg.pares == 20);
    CHECK(cfg.repeticiones == 5);
    CHECK(cfg.semilla == 42);
    CHECK(cfg.densidad == Catch::Approx(0.25));
    CHECK(cfg.dinamico.activo);
    REQUIRE(cfg.dinamico.eventos.size() == 1);
    CHECK(cfg.dinamico.eventos[0].tick == 30);
    CHECK(cfg.dinamico.eventos[0].multiplicador == mundo::kBloqueado);

    const auto minima = experimentos::leerConfigBenchmark(R"({"pares": 3})");
    CHECK(minima.pares == 3);
    CHECK(minima.repeticiones == 5);
    CHECK(minima.tamanos == std::vector<int>{32, 64, 128, 256});
    CHECK_FALSE(minima.dinamico.activo);
}

TEST_CASE("Una configuracion invalida da un mensaje que dice que corregir", "[RF-080][RF-093]") {
    auto mensaje = [](const char* json) {
        try {
            (void)experimentos::leerConfigBenchmark(json);
        } catch (const nucleo::ErrorJson& e) {
            return std::string(e.what());
        }
        return std::string("sin error");
    };
    CHECK(mensaje(R"({"paress": 3})").find("clave desconocida \"paress\"") != std::string::npos);
    CHECK(mensaje(R"({"pares": "veinte"})").find("pares: se esperaba un numero") != std::string::npos);
    CHECK(mensaje(R"({"pares": 2.5})").find("pares: se esperaba un entero") != std::string::npos);
    CHECK(mensaje(R"({"tamanos": [32, 2]})").find("tamanos: debe estar entre 4 y 4096") != std::string::npos);
    CHECK(mensaje(R"({"densidad": 1.5})").find("densidad: debe estar entre") != std::string::npos);
    CHECK(mensaje(R"({"dinamico": {"eventos": [{"tick": 3, "x": 4}]}})").find("x e y van juntos") != std::string::npos);
    CHECK(mensaje("{\n  \"pares\": 3,\n  \"reps\" 5\n}").find("linea 3") != std::string::npos);
}

TEST_CASE("El lector JSON entiende objetos, arreglos, numeros, cadenas y literales", "[ADR-0013]") {
    const auto v = nucleo::leerJson(R"( {"a": [1, -2.5, 3e2], "b": {"c": "x\"y\\né"}, "d": true, "e": null} )");
    const auto& a = v.buscar("a")->comoArreglo();
    REQUIRE(a.size() == 3);
    CHECK(a[0].comoNumero() == 1.0);
    CHECK(a[1].comoNumero() == -2.5);
    CHECK(a[2].comoEntero("a[2]", 0, 1000) == 300);
    CHECK(v.buscar("b")->buscar("c")->comoCadena() == "x\"y\\n\xC3\xA9");
    CHECK(v.buscar("d")->comoBool());
    CHECK(v.buscar("e")->esNulo());
    CHECK(v.buscar("z") == nullptr);
    for (const char* malo : {"", "{", "[1,]", "{\"a\":1,}", "01x", "\"sin cerrar", "{\"a\":1} 2", "{\"a\":1,\"a\":2}", "nul"})
        CHECK_THROWS_AS(nucleo::leerJson(malo), nucleo::ErrorJson);
    std::string profundo(100, '[');
    profundo += std::string(100, ']');
    CHECK_THROWS_AS(nucleo::leerJson(profundo), nucleo::ErrorJson);  // límite de anidamiento
}
