#pragma once
/// @file Optimizacion.hpp
/// @brief Optimización offline de los pesos de la IA con algoritmo genético y recocido simulado
///        (HU-21, RF-085, ADR-0015).
///
/// Un individuo es un vector de genes en [0, 1], uno por campo de la sección «pesos» de
/// config/ia.json (config::camposPesos()); cada gen se traduce a su rango de búsqueda (Gen).
/// La aptitud se mide con simulaciones sin ventana del experimento de coordinación (HU-18): la
/// horda coordinada contra el guion fijo de Mara, con las mismas semillas para todos los
/// candidatos (números aleatorios comunes), así que dos candidatos se comparan en igualdad de
/// condiciones y la misma configuración da siempre el mismo resultado (RNF-020).
///
/// Los dos algoritmos reciben el evaluador como función: las pruebas los ejercitan con funciones
/// sintéticas rápidas y el modo --optimize con las simulaciones. Toda la aleatoriedad sale de un
/// PCG32 con la semilla de la configuración y un flujo propio (RF-086).

#include <cstdint>
#include <functional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "dominio/config/Configuracion.hpp"

namespace refugio::experimentos {

enum class AlgoritmoOptimizacion : std::uint8_t { Genetico, Recocido };
/// «ga» o «sa».
[[nodiscard]] std::string_view nombreAlgoritmo(AlgoritmoOptimizacion a) noexcept;

/// Flujo PCG32 de la optimización (los del mapa y la partida usan otros).
inline constexpr std::uint64_t kFlujoOptimizacion = 0x4F50544DULL;  // «OPTM»

/// Rango de búsqueda de un gen: más estrecho que el rango válido de ia.json, para no perder
/// evaluaciones en valores absurdos.
struct Gen {
    const config::CampoPesos* campo;
    float minimo;
    float maximo;
};
/// Un gen por campo de config::camposPesos(), en el mismo orden.
[[nodiscard]] const std::vector<Gen>& genes();
/// «steering.seguir», «infectados.vel_perseguir», …
[[nodiscard]] std::string nombreGen(const Gen& g);

/// Pesos -> genes en [0, 1] (recorta a los rangos de búsqueda).
[[nodiscard]] std::vector<double> codificar(const config::PesosIa& p);
/// Genes en [0, 1] -> pesos. Los genes fuera de [0, 1] se recortan.
[[nodiscard]] config::PesosIa decodificar(std::span<const double> x);

// ---------------------------------------------------------------------------- Algoritmos
/// Algoritmo genético (ADR-0015): torneo k = 3, cruce uniforme, mutación gaussiana σ = 0,1 y
/// elitismo de 2.
struct ParametrosGa {
    int poblacion{30};
    int generaciones{40};
    int torneo{3};
    double probCruce{0.9};      ///< Si no se cruzan, el hijo es copia del primer padre.
    double probMutacion{0.2};   ///< Por gen.
    double sigma{0.1};          ///< Desviación de la mutación, en el espacio [0, 1].
    int elites{2};              ///< Los mejores pasan sin cambios (y sin volver a evaluarse).
};

/// Recocido simulado (ADR-0015): enfriamiento geométrico T <- α·T.
struct ParametrosSa {
    int iteraciones{2000};
    double temperaturaInicial{0.05};
    double alfa{0.995};
    double sigma{0.1};  ///< Vecino: todos los genes con ruido gaussiano de esta desviación.
};

/// Evalúa un lote de candidatos y devuelve una aptitud por candidato (mayor es mejor). Debe ser
/// determinista: el mismo candidato siempre recibe la misma aptitud.
using Evaluador = std::function<std::vector<double>(const std::vector<std::vector<double>>&)>;

struct GeneracionGa {
    int generacion{0};
    double mejor{0.0};
    double media{0.0};
    double peor{0.0};
    std::vector<double> mejorIndividuo;
};

struct ResultadoGa {
    std::vector<GeneracionGa> generaciones;  ///< La 0 es la población inicial.
    std::vector<double> mejor;
    double aptitudMejor{0.0};
    int evaluaciones{0};
};

/// @p inicial entra en la población inicial (los demás individuos son aleatorios); vacío = todos
/// aleatorios. El mejor de cada generación nunca empeora (elitismo).
[[nodiscard]] ResultadoGa ejecutarGa(const ParametrosGa& p, std::size_t numGenes, std::uint64_t semilla,
                                     const Evaluador& evaluar, const std::vector<double>& inicial = {});

struct IteracionSa {
    int iteracion{0};  ///< 0 = solución inicial.
    double temperatura{0.0};
    double aptitudVecino{0.0};
    double aptitudActual{0.0};
    double aptitudMejor{0.0};
    bool aceptada{false};
    std::vector<double> actual;  ///< Solución actual tras la iteración.
};

struct ResultadoSa {
    std::vector<IteracionSa> iteraciones;
    std::vector<double> mejor;
    double aptitudMejor{0.0};
    int evaluaciones{0};
    int aceptadas{0};
    int empeoramientosAceptados{0};
};

/// Empieza en @p inicial (vacío = punto aleatorio).
[[nodiscard]] ResultadoSa ejecutarSa(const ParametrosSa& p, std::size_t numGenes, std::uint64_t semilla,
                                     const Evaluador& evaluar, const std::vector<double>& inicial = {});

// ----------------------------------------------------------------------------- Aptitud
/// Simulaciones con las que se mide un candidato.
struct ConfigEvaluacion {
    int simulaciones{8};      ///< Escenarios por candidato: semillas semilla, semilla + 1, …
    double segundos{90.0};    ///< Tope por simulación.
    int ancho{32};
    int alto{22};
    int infectados{10};
    int hilos{0};             ///< 0 = los núcleos lógicos del equipo. No cambia el resultado.
};

/// Pesos de la aptitud (ADR-0015):
///   aptitud = contacto·presión + cobertura·cobertura − sobrecarga·sobrecargadas
///             − dificultad·(max(0, victorias_minimas − v) + max(0, v − victorias_maximas))
/// con v = tasa de victorias del guion, presión = 1 − t_contacto / segundos (0 si no hay contacto) y sobrecargadas = celdas con más
/// de MAX_AGENTES_POR_CELDA por tick de IA. Todo es la media de las simulaciones.
struct PesosAptitud {
    double contacto{1.0};
    double cobertura{1.0};
    double sobrecarga{0.5};
    double dificultad{4.0};
    double victoriasMinimas{0.2};  ///< El guion debe ganar al menos el 20 % (horda justa)…
    double victoriasMaximas{0.4};  ///< …y no más del 40 % (horda exigente).
};

struct MetricasAptitud {
    double aptitud{0.0};
    int simulaciones{0};
    int victorias{0};
    int contactos{0};
    double tasaVictorias{0.0};
    double presion{0.0};
    double tContactoMedio{0.0};  ///< Solo sobre las simulaciones con contacto.
    double cobertura{0.0};
    double sobrecargadasPorTick{0.0};
    double duracionMedia{0.0};
};

/// Simula los escenarios con @p pesos (horda coordinada, guion fijo de Mara) y calcula la aptitud.
[[nodiscard]] MetricasAptitud evaluarPesos(const ConfigEvaluacion& e, const PesosAptitud& w, std::uint64_t semilla,
                                           const config::PesosIa& pesos);
/// Evaluador por lotes con simulaciones; reparte (candidato, escenario) entre hilos.
[[nodiscard]] Evaluador evaluadorSimulacion(const ConfigEvaluacion& e, const PesosAptitud& w, std::uint64_t semilla);

// ---------------------------------------------------------------------------- Ejecución
struct ConfigOptimizacion {
    AlgoritmoOptimizacion algoritmo{AlgoritmoOptimizacion::Genetico};
    std::uint64_t semilla{11};
    ParametrosGa ga{};
    ParametrosSa sa{};
    ConfigEvaluacion evaluacion{};
    PesosAptitud aptitud{};
    config::PesosIa inicial{};  ///< Punto de partida (los pesos actuales del juego).
    std::string salida;         ///< "" = results/opt_<ga|sa>_<semilla>.json
    std::string archivoConfig;
};

/// Lee opt/ga.json u opt/sa.json. Lanza nucleo::ErrorJson con el campo que falla. Las claves
/// desconocidas son un error (evita erratas que pasarían inadvertidas).
[[nodiscard]] ConfigOptimizacion leerConfigOptimizacion(std::string_view json);

struct ResumenOptimizacion {
    MetricasAptitud base;    ///< Los pesos iniciales.
    MetricasAptitud mejor;
    config::PesosIa pesosMejor{};
    int evaluaciones{0};
    int simulaciones{0};
    bool mejorNoDecreciente{true};  ///< GA: la aptitud del mejor nunca baja entre generaciones.
};

/// Ejecuta el algoritmo de @p cfg y escribe el JSON de resultados en @p json (RF-085): metadatos,
/// configuración, métricas de los pesos iniciales y del mejor, la sección «pesos» lista para
/// copiar a config/ia.json y la evolución (por generación o por iteración).
ResumenOptimizacion ejecutarOptimizacion(const ConfigOptimizacion& cfg, const std::vector<std::string>& metadatos,
                                         std::ostream& json);

}  // namespace refugio::experimentos
