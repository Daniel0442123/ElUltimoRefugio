cmake_minimum_required(VERSION 3.24)
project(ElUltimoRefugio VERSION 0.3.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(REFUGIO_BUILD_GAME   "Compilar el juego con raylib (requiere OpenGL)" ON)
option(REFUGIO_BUILD_TESTS  "Compilar las pruebas con Catch2" ON)
option(REFUGIO_WARNINGS_AS_ERRORS "Tratar advertencias como errores" ON)
option(REFUGIO_SANITIZERS   "Compilar con AddressSanitizer y UBSan (GCC/Clang)" OFF)
option(REFUGIO_COVERAGE     "Instrumentar para cobertura (GCC/Clang)" OFF)

add_compile_definitions(REFUGIO_VERSION="${PROJECT_VERSION}")

include(cmake/Opciones.cmake)
include(cmake/Dependencias.cmake)

# ---------------------------------------------------------------------------
# Dominio puro: sin raylib, sin E/S de ventana (ADR-0003, RNF-023)
# ---------------------------------------------------------------------------
add_library(refugio_dominio STATIC
    src/dominio/mundo/Grafo.cpp
    src/dominio/navegacion/BuscadorRuta.cpp
    src/dominio/navegacion/Quadtree.cpp
    src/dominio/navegacion/CampoFlujo.cpp
    src/dominio/coordinacion/PropagadorAlertas.cpp
    src/dominio/experimentos/Benchmark.cpp
    src/dominio/experimentos/EstresRecursion.cpp
    src/dominio/experimentos/GeneradorMapas.cpp
    src/dominio/agentes/Percepcion.cpp
    src/dominio/agentes/MaquinaEstados.cpp
    src/dominio/juego/Partida.cpp
)
target_include_directories(refugio_dominio PUBLIC src)
refugio_configurar_objetivo(refugio_dominio)

# ---------------------------------------------------------------------------
# Herramienta sin ventana: benchmark y pruebas de estrés (RF-080, RNF-027)
# ---------------------------------------------------------------------------
add_executable(refugio_bench src/app/main_bench.cpp src/app/Cli.cpp)
target_link_libraries(refugio_bench PRIVATE refugio_dominio)
refugio_configurar_objetivo(refugio_bench)

# ---------------------------------------------------------------------------
# Juego (adaptadores raylib)
# ---------------------------------------------------------------------------
if(REFUGIO_BUILD_GAME)
    add_library(refugio_adaptadores STATIC src/adaptadores/raylib/VistaDepuracion.cpp
                                           src/adaptadores/raylib/VistaPartida.cpp)
    target_link_libraries(refugio_adaptadores PUBLIC refugio_dominio raylib)
    refugio_configurar_objetivo(refugio_adaptadores)

    add_executable(refugio src/app/main_juego.cpp src/app/Cli.cpp)
    target_link_libraries(refugio PRIVATE refugio_adaptadores)
    refugio_configurar_objetivo(refugio)
endif()

# ---------------------------------------------------------------------------
# Pruebas
# ---------------------------------------------------------------------------
if(REFUGIO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
