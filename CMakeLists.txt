cmake_minimum_required(VERSION 3.24)
project(ElUltimoRefugio VERSION 0.14.0 LANGUAGES CXX C)

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

# RF-081: commit y opciones de compilación para la cabecera de los resultados del benchmark. El
# commit se lee al configurar; CMake vuelve a configurar cuando cambia la rama o su último commit.
set(REFUGIO_COMMIT "desconocido")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE REFUGIO_COMMIT
                    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND "${GIT_EXECUTABLE}" symbolic-ref -q HEAD
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE _refugio_ref
                    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/.git/HEAD")
    if(_refugio_ref AND EXISTS "${CMAKE_SOURCE_DIR}/.git/${_refugio_ref}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/.git/${_refugio_ref}")
    endif()
    if(REFUGIO_COMMIT STREQUAL "")
        set(REFUGIO_COMMIT "desconocido")
    endif()
endif()
set(REFUGIO_OPCIONES "${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION} C++${CMAKE_CXX_STANDARD}")
if(REFUGIO_SANITIZERS)
    string(APPEND REFUGIO_OPCIONES " asan+ubsan")
endif()

include(cmake/Opciones.cmake)
include(cmake/Dependencias.cmake)

# ---------------------------------------------------------------------------
# Dominio puro: sin raylib, sin E/S de ventana (ADR-0003, RNF-023)
# ---------------------------------------------------------------------------
add_library(refugio_dominio STATIC
    src/dominio/nucleo/Json.cpp
    src/dominio/mundo/Grafo.cpp
    src/dominio/navegacion/BuscadorRuta.cpp
    src/dominio/navegacion/Comparacion.cpp
    src/dominio/navegacion/Quadtree.cpp
    src/dominio/navegacion/CampoFlujo.cpp
    src/dominio/navegacion/Steering.cpp
    src/dominio/coordinacion/ContractNet.cpp
    src/dominio/coordinacion/AsignadorAccesos.cpp
    src/dominio/coordinacion/PropagadorAlertas.cpp
    src/dominio/experimentos/Benchmark.cpp
    src/dominio/experimentos/ExperimentoCoordinacion.cpp
    src/dominio/experimentos/EstresRecursion.cpp
    src/dominio/experimentos/GeneradorMapas.cpp
    src/dominio/experimentos/Optimizacion.cpp
    src/dominio/agentes/Percepcion.cpp
    src/dominio/agentes/MaquinaEstados.cpp
    src/dominio/agentes/Memoria.cpp
    src/dominio/agentes/Tactica.cpp
    src/dominio/superviviente/Goap.cpp
    src/dominio/superviviente/ArbolComportamiento.cpp
    src/dominio/superviviente/Superviviente.cpp
    src/dominio/juego/Partida.cpp
    src/dominio/juego/PartidaSuperviviente.cpp
    src/dominio/juego/RegistroIa.cpp
    src/dominio/juego/Tutorial.cpp
    src/dominio/config/Configuracion.cpp
    src/dominio/persistencia/Crc32.cpp
    src/dominio/persistencia/PartidaGuardada.cpp
)
target_include_directories(refugio_dominio PUBLIC src)
# La optimización (HU-21) reparte las simulaciones entre hilos.
find_package(Threads REQUIRED)
target_link_libraries(refugio_dominio PUBLIC Threads::Threads)
refugio_configurar_objetivo(refugio_dominio)

# ---------------------------------------------------------------------------
# Adaptadores de archivo sin raylib: registro de IA en logs/ (RF-072, RF-073), partidas en
# saves/ y configuración en config/ (RF-090 a RF-093)
# ---------------------------------------------------------------------------
add_library(refugio_archivo STATIC src/adaptadores/archivo/RegistroArchivo.cpp
                                   src/adaptadores/archivo/ArchivosLocales.cpp)
target_link_libraries(refugio_archivo PUBLIC refugio_dominio)
refugio_configurar_objetivo(refugio_archivo)

# ---------------------------------------------------------------------------
# Herramienta sin ventana: benchmark y pruebas de estrés (RF-080, RNF-027)
# ---------------------------------------------------------------------------
add_library(refugio_cli STATIC src/app/Cli.cpp)
target_link_libraries(refugio_cli PUBLIC refugio_dominio)
target_compile_definitions(refugio_cli PRIVATE REFUGIO_COMMIT="${REFUGIO_COMMIT}" REFUGIO_OPCIONES="${REFUGIO_OPCIONES}")
refugio_configurar_objetivo(refugio_cli)

# Menú de pausa (HU-19): lógica sin raylib para poder probarla.
add_library(refugio_menu STATIC src/app/MenuPausa.cpp)
target_link_libraries(refugio_menu PUBLIC refugio_dominio)
refugio_configurar_objetivo(refugio_menu)

add_executable(refugio_bench src/app/main_bench.cpp)
target_link_libraries(refugio_bench PRIVATE refugio_cli)
refugio_configurar_objetivo(refugio_bench)

# ---------------------------------------------------------------------------
# Juego (adaptadores raylib)
# ---------------------------------------------------------------------------
if(REFUGIO_BUILD_GAME)
    add_library(refugio_adaptadores STATIC src/adaptadores/raylib/VistaDepuracion.cpp
                                           src/adaptadores/raylib/VistaPartida.cpp
                                           src/adaptadores/raylib/Sonidos.cpp)
    target_link_libraries(refugio_adaptadores PUBLIC refugio_dominio refugio_archivo raylib)
    refugio_configurar_objetivo(refugio_adaptadores)

    add_executable(refugio src/app/main_juego.cpp)
    target_link_libraries(refugio PRIVATE refugio_adaptadores refugio_cli refugio_menu)
    refugio_configurar_objetivo(refugio)
endif()

# ---------------------------------------------------------------------------
# Pruebas
# ---------------------------------------------------------------------------
if(REFUGIO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
