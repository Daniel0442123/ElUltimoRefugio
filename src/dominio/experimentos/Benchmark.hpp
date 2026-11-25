#pragma once
/// @file Benchmark.hpp
/// @brief Comparación experimental de Dijkstra y A* (RF-080 a RF-083, RF-086, GUIA §7, HU-17).
///
/// Mismos mapas, orígenes, destinos y costos para ambos algoritmos; varios tamaños;
/// al menos 5 repeticiones por condición; semilla registrada. El escenario dinámico (RF-083)
/// hace caminar a un agente por su ruta y cambia el mapa en ticks fijados: cuando el cambio toca
/// la ruta que le queda, replanifica, y se registran replanificaciones, costo final y nodos.

#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "dominio/navegacion/BuscadorRuta.hpp"

namespace refugio::experimentos {

/// Cambio del mapa en un tick (RF-083). La celda es (x, y) si ambos son >= 0, la misma para los
/// dos algoritmos; si no, la celda que está @c adelante pasos por delante del agente en su ruta.
struct EventoDinamico {
    int tick{30};
    int x{-1};
    int y{-1};
    int adelante{4};
    std::uint8_t multiplicador{mundo::kBloqueado};  ///< 0 = barricada; 1..255 = nuevo costo de terreno.
};

struct ConfigDinamico {
    bool activo{false};
    std::vector<int> tamanos{64, 128};
    std::vector<EventoDinamico> eventos{EventoDinamico{}};
};

struct ConfigBenchmark {
    std::vector<int> tamanos{32, 64, 128, 256};
    int pares{20};
    int repeticiones{5};
    double densidad{0.25};
    double terrenoCaro{0.10};
    std::uint64_t semilla{42};
    std::string mapa{"ciudad"};     ///< Prefijo de la columna «mapa» (ciudad_64, ciudad_128...).
    std::string archivoConfig;      ///< Archivo del que se leyó la configuración (RF-081).
    std::string salida;             ///< Ruta del CSV; vacía = results/bench_<fecha>_<semilla>.csv.
    ConfigDinamico dinamico;
};

struct ResumenBenchmark {
    std::uint64_t mediciones{0};
    std::uint64_t discrepanciasOptimalidad{0};  ///< |costo A* - costo Dijkstra| > 1e-6 (RF-082).
    std::uint64_t consultasSinRuta{0};
    std::uint64_t aEstrellaExpandioMas{0};      ///< Consultas en que A* expandió más que Dijkstra.
};

struct ResumenDinamico {
    std::uint64_t filas{0};
    std::uint64_t ejecuciones{0};      ///< Recorridos por algoritmo (filas / repeticiones).
    std::uint64_t replanificaciones[2]{0, 0};  ///< Totales de Dijkstra [0] y A* [1].
    std::uint64_t sinReplanificar[2]{0, 0};    ///< Recorridos en que ningún evento tocó la ruta.
    std::uint64_t noLlegaron{0};
};

/// Crea un buscador para el algoritmo; permite inyectar uno defectuoso en las pruebas (CP-082).
using FabricaBuscador = std::function<std::unique_ptr<navegacion::IBuscadorRuta>(navegacion::Algoritmo)>;

/// Columnas del CSV, en orden (RF-080). Las usan las pruebas y tools/resumen_bench.py.
[[nodiscard]] const std::vector<std::string_view>& columnasBenchmark();

/// Ejecuta el benchmark estático y escribe el CSV en @p csv. Las líneas de @p metadatos se
/// escriben como cabecera con prefijo «# » (RF-081). El dominio no sabe de qué equipo se trata.
ResumenBenchmark ejecutarBenchmark(const ConfigBenchmark& cfg, const std::vector<std::string>& metadatos,
                                   std::ostream& csv, const FabricaBuscador& fabrica = {});

/// Escenario dinámico (RF-083) con las mismas columnas: costo es el costo recorrido, nodos y tiempo
/// son la suma de todas las búsquedas y replanificaciones las del recorrido.
ResumenDinamico ejecutarBenchmarkDinamico(const ConfigBenchmark& cfg, const std::vector<std::string>& metadatos,
                                          std::ostream& csv, const FabricaBuscador& fabrica = {});

/// Lee la configuración desde el texto de un JSON (bench/config.json). Las claves ausentes conservan
/// el valor por defecto. Lanza nucleo::ErrorJson con un mensaje descriptivo si algo no es válido.
[[nodiscard]] ConfigBenchmark leerConfigBenchmark(std::string_view json);

}  // namespace refugio::experimentos
