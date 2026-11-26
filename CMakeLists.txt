cmake_minimum_required(VERSION 3.24)
project(ElUltimoRefugio VERSION 0.17.0 LANGUAGES CXX C)

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
    src/dominio/navegacion/PlanificadorRutas.cpp
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

# Lectura visual del mapa para el arte pixel (edificios, aceras, rayos de luz): sin raylib para
# poder probarla en CI.
add_library(refugio_vista STATIC src/adaptadores/vista/Escenario.cpp src/adaptadores/vista/Disposicion.cpp)
target_link_libraries(refugio_vista PUBLIC refugio_dominio)
refugio_configurar_objetivo(refugio_vista)

# Menú de pausa (HU-19): lógica sin raylib para poder probarla.
add_library(refugio_menu STATIC src/app/MenuPausa.cpp)
target_link_libraries(refugio_menu PUBLIC refugio_dominio)
refugio_configurar_objetivo(refugio_menu)

# Medición de rendimiento con ventana (RNF-001, RNF-002, RNF-004, RNF-005, RNF-008): estadísticas
# y resultados sin raylib, y CPU y memoria del proceso leídas del sistema operativo.
add_library(refugio_medicion STATIC src/app/MedicionRendimiento.cpp src/adaptadores/sistema/RecursosProceso.cpp)
target_link_libraries(refugio_medicion PUBLIC refugio_dominio)
if(WIN32)
    target_link_libraries(refugio_medicion PUBLIC psapi)
endif()
refugio_configurar_objetivo(refugio_medicion)

add_executable(refugio_bench src/app/main_bench.cpp)
target_link_libraries(refugio_bench PRIVATE refugio_cli)
refugio_configurar_objetivo(refugio_bench)

# ---------------------------------------------------------------------------
# Juego (adaptadores raylib)
# ---------------------------------------------------------------------------
if(REFUGIO_BUILD_GAME)
    # Tipografía de la interfaz: Inter (SIL OFL 1.1, assets/fuentes/OFL.txt), reducida a latín
    # básico y Latin-1 (tildes, ñ, ¿, ¡, «») e incrustada en el ejecutable.
    include(cmake/Embeber.cmake)
    set(REFUGIO_FUENTES_CPP "${CMAKE_BINARY_DIR}/generado/FuentesEmbebidas.cpp")
    refugio_embeber("${REFUGIO_FUENTES_CPP}" refugio::adaptadores::embebido
        kInterMedium "${CMAKE_SOURCE_DIR}/assets/fuentes/Inter-Medium.ttf"
        kInterBold   "${CMAKE_SOURCE_DIR}/assets/fuentes/Inter-Bold.ttf")

    add_library(refugio_adaptadores STATIC src/adaptadores/raylib/VistaDepuracion.cpp
                                           src/adaptadores/raylib/VistaPartida.cpp
                                           src/adaptadores/raylib/Sonidos.cpp
                                           src/adaptadores/raylib/ArteMundo.cpp
                                           src/adaptadores/raylib/Interfaz.cpp
                                           "${REFUGIO_FUENTES_CPP}")
    target_link_libraries(refugio_adaptadores PUBLIC refugio_dominio refugio_archivo refugio_vista raylib)
    refugio_configurar_objetivo(refugio_adaptadores)

    add_executable(refugio src/app/main_juego.cpp)
    target_link_libraries(refugio PRIVATE refugio_adaptadores refugio_cli refugio_menu refugio_medicion)
    refugio_configurar_objetivo(refugio)
endif()

# ---------------------------------------------------------------------------
# Pruebas
# ---------------------------------------------------------------------------
if(REFUGIO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
