#pragma once
/// @file Tactica.hpp
/// @brief Selección de táctica por utilidad (RF-054) y punto de corte para interceptar (RF-053).
///
/// Funciones puras: reciben la mejor evidencia del agente ya resumida y devuelven la táctica con
/// sus tres puntuaciones, para registrarlas (RF-072) y probarlas fila por fila (CP-054).
///
/// Puntuaciones (c = confianza con decaimiento, d = distancia a la evidencia en celdas):
/// - Perseguir: c / (1 + d_ef/10). Para un avistamiento d_ef = d + v · edad, porque Mara pudo
///   moverse desde entonces (v = su velocidad estimada o, sin rumbo, velocidadSupuesta). Para un
///   ruido o una alerta es la utilidad de RF-050 (ir al punto de la pista).
/// - Interceptar: solo con rumbo y un punto de corte alcanzable antes que Mara:
///   pesoInterceptar · c / (1 + dCorte/10).
/// - Explorar: utilidadExplorar, constante. Gana cuando la pista ya vale poco.

#include <array>
#include <optional>

#include "dominio/agentes/MaquinaEstados.hpp"
#include "dominio/agentes/Memoria.hpp"
#include "dominio/mundo/Grafo.hpp"
#include "dominio/navegacion/Quadtree.hpp"

namespace refugio::agentes {

using navegacion::Vec2;

struct ParametrosTactica {
    float utilidadExplorar{0.15f};
    float pesoInterceptar{0.8f};
    float velocidadSupuesta{3.0f};  ///< Celdas/s que Mara pudo recorrer si no se sabe su rumbo.
    float velocidadMinimaRumbo{0.5f};  ///< Por debajo, Mara está quieta: no hay rumbo.
    float horizonteMin{2.0f};       ///< RF-053: el corte va al menos 2 s por delante de Mara.
    float horizonteMax{6.0f};
};

/// Mejor evidencia del agente, resumida para decidir.
struct EntradaTactica {
    bool hayEvidencia{false};
    TipoEvidencia tipo{};
    float confianza{0.0f};  ///< Ya con decaimiento.
    float edad{0.0f};       ///< Segundos.
    float distancia{0.0f};  ///< Del agente a la celda de la evidencia.
    bool tieneRumbo{false};
    float velocidadMara{0.0f};
    bool hayCorte{false};       ///< Hay punto de corte al que llega antes que Mara.
    float distanciaCorte{0.0f};  ///< Del agente al punto de corte.
};

struct ResultadoTactica {
    Tactica tactica{Tactica::Explorar};
    std::array<float, kNumTacticas> puntuacion{};  ///< Indexado por Tactica.
};

/// RF-054. Complejidad O(1). A igualdad gana Perseguir, luego Interceptar.
[[nodiscard]] ResultadoTactica elegirTactica(const EntradaTactica& e, const ParametrosTactica& p) noexcept;

struct PuntoCorte {
    mundo::Celda celda{};
    float tiempoMara{0.0f};    ///< Segundos que tarda Mara en llegar.
    float distancia{0.0f};     ///< Del agente al punto.
};

/// RF-053: ruta prevista de Mara = recta desde @p posMara con velocidad @p velMara, cortada en la
/// primera pared. Entre horizonteMin y horizonteMax (pasos de 0,25 s) devuelve la celda de esa ruta
/// más cercana al agente entre aquellas a las que el agente (a @p velAgente) llega antes que Mara.
[[nodiscard]] std::optional<PuntoCorte> puntoDeCorte(const mundo::GrafoRejilla& g, Vec2 posMara, Vec2 velMara,
                                                     Vec2 posAgente, float velAgente,
                                                     const ParametrosTactica& p) noexcept;

}  // namespace refugio::agentes
