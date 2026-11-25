#include "app/Cli.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include "dominio/experimentos/Benchmark.hpp"
#include "dominio/experimentos/EstresRecursion.hpp"
#include "dominio/experimentos/ExperimentoCoordinacion.hpp"

#ifndef REFUGIO_VERSION
#define REFUGIO_VERSION "0.1.0"
#endif
#ifndef REFUGIO_COMMIT
#define REFUGIO_COMMIT "desconocido"
#endif
#ifndef REFUGIO_OPCIONES
#define REFUGIO_OPCIONES ""
#endif

namespace refugio::app {
namespace {

std::string compilador() {
#if defined(__clang__)
    return "clang " __clang_version__;
#elif defined(__GNUC__)
    return "gcc " __VERSION__;
#elif defined(_MSC_VER)
    return "msvc " + std::to_string(_MSC_FULL_VER);
#else
    return "desconocido";
#endif
}

std::string sistemaOperativo() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    std::ifstream f("/etc/os-release");
    std::string linea;
    while (std::getline(f, linea))
        if (linea.rfind("PRETTY_NAME=", 0) == 0) return linea.substr(12);
    return "Linux";
#else
    return "desconocido";
#endif
}

std::string cpu() {
#if defined(_WIN32)
    char buf[256]{};
    DWORD tam = sizeof(buf);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString",
                     RRF_RT_REG_SZ, nullptr, buf, &tam) == ERROR_SUCCESS)
        return buf;
    return "desconocida";
#elif defined(__linux__)
    std::ifstream f("/proc/cpuinfo");
    std::string linea;
    while (std::getline(f, linea))
        if (linea.rfind("model name", 0) == 0) return linea.substr(linea.find(':') + 2);
    return "desconocida";
#else
    return "desconocida";
#endif
}

std::string ramMb() {
#if defined(_WIN32)
    MEMORYSTATUSEX m{};
    m.dwLength = sizeof(m);
    if (GlobalMemoryStatusEx(&m)) return std::to_string(m.ullTotalPhys / (1024ULL * 1024ULL));
    return "NA";
#elif defined(__linux__)
    std::ifstream f("/proc/meminfo");
    std::string clave;
    unsigned long long kb = 0;
    if (f >> clave >> kb) return std::to_string(kb / 1024ULL);
    return "NA";
#else
    return "NA";
#endif
}

std::string fechaUtc() {
    const std::time_t t = std::time(nullptr);
    char buf[32]{};
    std::tm tmUtc{};
#if defined(_WIN32)
    gmtime_s(&tmUtc, &t);
#else
    gmtime_r(&t, &tmUtc);
#endif
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmUtc);
    return buf;
}

/// Fecha local compacta para el nombre del archivo de resultados: 20261125-143000.
std::string fechaArchivo() {
    const std::time_t t = std::time(nullptr);
    char buf[32]{};
    std::tm tmLocal{};
#if defined(_WIN32)
    localtime_s(&tmLocal, &t);
#else
    localtime_r(&t, &tmLocal);
#endif
    std::strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", &tmLocal);
    return buf;
}

std::string tipoCompilacion() {
#if defined(NDEBUG)
    return "Release";
#else
    return "Debug";
#endif
}

std::string leerArchivo(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) throw std::runtime_error("No se pudo abrir la configuracion " + ruta);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

/// results/bench.csv -> results/bench_dinamico.csv
std::string rutaDinamico(const std::string& ruta) {
    const std::filesystem::path p(ruta);
    auto q = p;
    q.replace_filename(p.stem().string() + "_dinamico" + (p.has_extension() ? p.extension().string() : ".csv"));
    return q.string();
}

std::vector<int> listaEnteros(std::string_view texto) {
    std::vector<int> v;
    std::stringstream ss{std::string(texto)};
    std::string parte;
    while (std::getline(ss, parte, ',')) v.push_back(std::stoi(parte));
    return v;
}

void ayuda() {
    std::cout << "Uso:\n"
                 "  refugio --bench [config.json] [--tamanos 32,64,128,256] [--pares 20] [--reps 5]\n"
                 "                [--densidad 0.25] [--semilla 42] [--dinamico] [--salida results/bench.csv]\n"
                 "    Sin --salida escribe results/bench_<fecha>_<semilla>.csv; con escenario dinamico,\n"
                 "    tambien results/bench_<fecha>_<semilla>_dinamico.csv. Las opciones mandan sobre el JSON.\n"
                 "  refugio --coord-exp <semilla> [--reps 10] [--infectados 10] [--segundos 120]\n"
                 "                [--salida results/coord.csv]\n"
                 "    Horda aislada frente a coordinada (HU-18): reps partidas en cada modo con las mismas\n"
                 "    semillas y el mismo guion de Mara. Sin --salida escribe results/coord_<fecha>_<semilla>.csv.\n"
                 "  refugio_bench --estres-recursion [--agentes 50,100,...] [--profundidades 1,2,3]\n"
                 "                [--reps 5] [--semilla 42] [--salida results/estres.csv]\n";
}

std::ofstream abrirSalida(const std::string& ruta) {
    const std::filesystem::path p(ruta);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    std::ofstream f(p);
    if (!f) throw std::runtime_error("No se pudo crear " + ruta);
    return f;
}

}  // namespace

std::vector<std::string> metadatosEquipo() {
    return {"juego=ElUltimoRefugio version=" REFUGIO_VERSION " commit=" REFUGIO_COMMIT,
            "cpu=" + cpu(),
            "nucleos_logicos=" + std::to_string(std::thread::hardware_concurrency()),
            "ram_mb=" + ramMb(),
            "so=" + sistemaOperativo(),
            "compilador=" + compilador(),
            "opciones=" + tipoCompilacion() + " " REFUGIO_OPCIONES,
            "fecha_utc=" + fechaUtc()};
}

namespace {

void imprimirModo(std::string_view nombre, const experimentos::ResumenModo& r) {
    std::cout << "  " << nombre << ": " << r.simulaciones << " partidas, deteccion en " << r.detecciones
              << " (media " << r.mediaDeteccion << " s), contacto en " << r.contactos << " (media " << r.mediaContacto
              << " s), cobertura media " << r.mediaCobertura * 100.0 << " %, mensajes medios " << r.mediaMensajes
              << ", celdas sobrecargadas medias " << r.mediaSobrecargadas << ", max. infectados a 2 celdas "
              << r.mediaMaxCerca << ", duracion media " << r.mediaDuracion << " s, victorias " << r.victorias
              << ", derrotas " << r.derrotas << '\n';
}

/// --coord-exp <semilla> (RF-084). Devuelve el código de salida: 4 si algún par diverge antes de la
/// primera detección (RF-056 exige que el resto sea idéntico).
template <typename Valor>
int ejecutarCoordinacion(const Valor& valor, int codigoPrevio, bool hayOtroModo) {
    experimentos::ConfigCoordinacion cfg;
    if (auto v = valor("--coord-exp"); !v.empty() && !v.starts_with("--")) cfg.semilla = std::stoull(std::string(v));
    if (auto v = valor("--reps"); !v.empty()) cfg.repeticiones = std::stoi(std::string(v));
    if (auto v = valor("--infectados"); !v.empty()) cfg.infectados = std::stoi(std::string(v));
    if (auto v = valor("--segundos"); !v.empty()) cfg.segundos = std::stod(std::string(v));
    if (cfg.repeticiones < 1 || cfg.infectados < 1 || cfg.segundos <= 0.0)
        throw std::runtime_error("--reps, --infectados y --segundos deben ser positivos");
    std::string salida(valor("--salida"));
    if (salida.empty() || hayOtroModo)
        salida = "results/coord_" + fechaArchivo() + "_" + std::to_string(cfg.semilla) + ".csv";
    auto f = abrirSalida(salida);
    const auto r = experimentos::ejecutarExperimentoCoordinacion(cfg, metadatosEquipo(), f);
    std::cout << "Aislados frente a coordinados: " << r.simulaciones.size() << " simulaciones (semilla " << cfg.semilla
              << ", " << cfg.repeticiones << " por modo) -> " << salida << '\n';
    imprimirModo("aislado   ", r.aislado);
    imprimirModo("coordinado", r.coordinado);
    std::cout << "  pares en que la horda coordinada llego antes a Mara: " << r.contactoAntesCoordinado << " de "
              << cfg.repeticiones << "; pares distintos antes de la primera deteccion: " << r.paresDivergentes << '\n';
    if (r.paresDivergentes > 0) {
        std::cerr << "ERROR: " << r.paresDivergentes
                  << " pares difieren antes de la primera deteccion; el modo aislado debe cambiar solo la comunicacion\n";
        return 4;
    }
    return hayOtroModo ? codigoPrevio : 0;
}

}  // namespace

bool ejecutarModoSinVentana(int argc, char** argv, int& codigo) {
    std::vector<std::string_view> args(argv + 1, argv + argc);
    const bool bench = std::find(args.begin(), args.end(), "--bench") != args.end();
    const bool estres = std::find(args.begin(), args.end(), "--estres-recursion") != args.end();
    const bool coord = std::find(args.begin(), args.end(), "--coord-exp") != args.end();
    const bool help = std::find(args.begin(), args.end(), "--ayuda") != args.end() ||
                      std::find(args.begin(), args.end(), "--help") != args.end();
    if (help) {
        ayuda();
        codigo = 0;
        return true;
    }
    if (!bench && !estres && !coord) return false;

    auto valor = [&](std::string_view clave) -> std::string_view {
        for (std::size_t i = 0; i + 1 < args.size(); ++i)
            if (args[i] == clave) return args[i + 1];
        return {};
    };

    try {
        if (bench) {
            experimentos::ConfigBenchmark cfg;
            // --bench <config.json>: el argumento siguiente, si no es otra opción (RF-080).
            const auto it = std::find(args.begin(), args.end(), "--bench");
            if (it + 1 != args.end() && !(it + 1)->starts_with("--")) {
                const std::string ruta(*(it + 1));
                cfg = experimentos::leerConfigBenchmark(leerArchivo(ruta));
                cfg.archivoConfig = ruta;
            }
            if (auto v = valor("--tamanos"); !v.empty()) cfg.tamanos = listaEnteros(v);
            if (auto v = valor("--pares"); !v.empty()) cfg.pares = std::stoi(std::string(v));
            if (auto v = valor("--reps"); !v.empty()) cfg.repeticiones = std::stoi(std::string(v));
            if (auto v = valor("--densidad"); !v.empty()) cfg.densidad = std::stod(std::string(v));
            if (auto v = valor("--semilla"); !v.empty()) cfg.semilla = std::stoull(std::string(v));
            if (auto v = valor("--salida"); !v.empty()) cfg.salida = std::string(v);
            if (std::find(args.begin(), args.end(), "--dinamico") != args.end()) cfg.dinamico.activo = true;
            if (cfg.salida.empty())
                cfg.salida = "results/bench_" + fechaArchivo() + "_" + std::to_string(cfg.semilla) + ".csv";
            const auto meta = metadatosEquipo();
            auto f = abrirSalida(cfg.salida);
            const auto r = experimentos::ejecutarBenchmark(cfg, meta, f);
            std::cout << "Benchmark: " << r.mediciones << " mediciones, " << r.consultasSinRuta
                      << " consultas sin ruta, " << r.discrepanciasOptimalidad << " discrepancias de optimalidad, "
                      << r.aEstrellaExpandioMas << " consultas en que A* expandio mas -> " << cfg.salida << '\n';
            codigo = r.discrepanciasOptimalidad == 0 ? 0 : 2;  // RF-082: toda discrepancia es un error
            if (r.discrepanciasOptimalidad > 0)
                std::cerr << "ERROR: A* y Dijkstra difieren en costo en " << r.discrepanciasOptimalidad
                          << " consultas (tolerancia 1e-6)\n";
            if (cfg.dinamico.activo) {
                const std::string salidaDin = rutaDinamico(cfg.salida);
                auto fd = abrirSalida(salidaDin);
                const auto d = experimentos::ejecutarBenchmarkDinamico(cfg, meta, fd);
                std::cout << "Escenario dinamico: " << d.filas << " filas, replanificaciones Dijkstra "
                          << d.replanificaciones[0] << " / A* " << d.replanificaciones[1] << ", recorridos sin replanificar "
                          << d.sinReplanificar[0] << " / " << d.sinReplanificar[1] << ", sin llegar " << d.noLlegaron
                          << " -> " << salidaDin << '\n';
            }
        }
        if (estres) {
            experimentos::ConfigEstres cfg;
            if (auto v = valor("--agentes"); !v.empty()) cfg.agentes = listaEnteros(v);
            if (auto v = valor("--profundidades"); !v.empty()) cfg.profundidades = listaEnteros(v);
            if (auto v = valor("--reps"); !v.empty()) cfg.repeticiones = std::stoi(std::string(v));
            if (auto v = valor("--semilla"); !v.empty()) cfg.semilla = std::stoull(std::string(v));
            const std::string salida =
                valor("--salida").empty() ? "results/estres_recursion.csv" : std::string(valor("--salida"));
            auto f = abrirSalida(salida);
            for (const auto& m : metadatosEquipo()) f << "# " << m << '\n';
            const auto r = experimentos::ejecutarEstresRecursion(cfg, f);
            std::cout << "Estres de recursion: " << r.filas << " filas, pila maxima " << r.pilaMaximaBytes
                      << " bytes, profundidad quadtree (10000 puntos coincidentes) "
                      << r.profundidadQuadtreeCoincidentes << ", violaciones de limite " << r.violacionesLimite
                      << ", alertas duplicadas " << r.violacionesUnicidad << " -> " << salida << '\n';
            const bool ok = r.violacionesLimite == 0 && r.violacionesUnicidad == 0 && r.pilaMaximaBytes <= 64 * 1024;
            if (!ok) codigo = 3;
            else if (!bench) codigo = 0;
        }
        if (coord) codigo = ejecutarCoordinacion(valor, codigo, bench || estres);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        codigo = 1;
    }
    return true;
}

}  // namespace refugio::app
