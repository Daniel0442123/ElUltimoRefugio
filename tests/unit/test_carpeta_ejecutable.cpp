// Versión portable (RNF-028): el juego busca config/, maps/, saves/ y logs/ junto al ejecutable
// cuando se abre desde otra carpeta, y no cambia nada al lanzarlo desde la raíz del repositorio.
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "adaptadores/sistema/CarpetaEjecutable.hpp"

namespace fs = std::filesystem;
using refugio::adaptadores::carpetaDatos;
using refugio::adaptadores::kMarcaCarpetaDatos;

namespace {
void marcar(const fs::path& carpeta) {
    fs::create_directories(carpeta / "config");
    std::ofstream(carpeta / kMarcaCarpetaDatos) << "{}";
}

/// Carpeta temporal limpia con portable/ (ejecutable y datos) y otra/ (directorio de trabajo).
fs::path preparar(const char* nombre) {
    const fs::path base = fs::path(REFUGIO_DIR_SALIDA) / nombre;
    fs::remove_all(base);
    fs::create_directories(base / "portable");
    fs::create_directories(base / "otra");
    return base;
}
}  // namespace

TEST_CASE("Version portable: abierta desde otra carpeta usa la del ejecutable", "[RNF-028][portable]") {
    const fs::path base = preparar("portable_otra");
    marcar(base / "portable");
    const auto r = carpetaDatos(base / "otra", base / "portable" / "refugio.exe");
    REQUIRE(r.has_value());
    CHECK(fs::equivalent(*r, base / "portable"));
}

TEST_CASE("Version portable: si la carpeta de trabajo tiene sus datos no se cambia", "[RNF-028][portable]") {
    const fs::path base = preparar("portable_repo");
    marcar(base / "portable");
    marcar(base / "otra");  // p. ej. la raíz del repositorio con build/Release/refugio.exe
    CHECK_FALSE(carpetaDatos(base / "otra", base / "portable" / "refugio.exe").has_value());
}

TEST_CASE("Version portable: sin datos junto al ejecutable no se cambia", "[RNF-028][portable]") {
    const fs::path base = preparar("portable_sin_datos");
    CHECK_FALSE(carpetaDatos(base / "otra", base / "portable" / "refugio.exe").has_value());
    CHECK_FALSE(carpetaDatos(base / "otra", fs::path{}).has_value());
}

TEST_CASE("Version portable: abierta en su propia carpeta no se cambia", "[RNF-028][portable]") {
    const fs::path base = preparar("portable_misma");
    marcar(base / "portable");
    CHECK_FALSE(carpetaDatos(base / "portable", base / "portable" / "refugio.exe").has_value());
}

TEST_CASE("Version portable: la ruta del ejecutable en marcha existe", "[RNF-028][portable]") {
    const fs::path exe = refugio::adaptadores::rutaEjecutable();
    REQUIRE_FALSE(exe.empty());
    CHECK(fs::exists(exe));
}
