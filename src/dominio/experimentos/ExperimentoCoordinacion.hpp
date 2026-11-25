#pragma once
/// @file ExperimentoCoordinacion.hpp
/// @brief Horda aislada frente a horda coordinada con la misma semilla (HU-18, RF-056, RF-084).
///
/// Cada repetición r usa la semilla maestra + r para generar el mapa y la partida, y se simula dos
/// veces: una con el modo aislado (sin alertas ni Contract Net, RF-056) y otra con el coordinado.
/// Mara no la controla nadie: sigue un guion fijo (GuionJugadora) que recoge los objetos, instala
/// la radio y va a la extracción. Así la única diferencia entre las dos simulaciones de un par es
/// la comunicación entre infectados. Hasta la primera detección las dos deben ser idénticas.

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "dominio/juego/Partida.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"

namespace refugio::experimentos {

enum class ModoHorda : std::uint8_t { Aislado, Coordinado };
[[nodiscard]] std::string_view nombreModo(ModoHorda m) noexcept;

struct ConfigCoordinacion {
    std::uint64_t semilla{7};
    int repeticiones{10};
    int ancho{32};       ///< Mismo mapa que la partida del juego (32 x 22 celdas).
    int alto{22};
    int infectados{10};
    double segundos{120.0};  ///< Tope de tiempo simulado por partida.
    juego::ConfigPartida partida{};  ///< Parámetros de la IA; el experimento solo cambia «coordinado».
};

/// Métricas de una simulación (una fila del CSV, RF-084).
struct MetricasSimulacion {
    ModoHorda modo{ModoHorda::Coordinado};
    std::uint64_t semilla{0};
    int repeticion{0};
    std::uint64_t ticks{0};
    double duracion{0.0};  ///< Segundos simulados: hasta la derrota, la victoria o el tope.
    juego::EstadoPartida estado{juego::EstadoPartida::EnCurso};
    juego::MotivoFin motivo{juego::MotivoFin::Ninguno};
    double tPrimeraDeteccion{-1.0};  ///< Segundos hasta que algún infectado ve a Mara; -1 si nunca.
    double tContacto{-1.0};          ///< Segundos hasta que alguno está a 1 celda o menos; -1 si nunca.
    double cobertura{0.0};           ///< Fracción de celdas transitables pisadas por algún infectado.
    std::uint64_t mensajes{0};       ///< Alertas + anuncios, pujas y adjudicaciones (RF-073).
    std::uint64_t mensajesAlerta{0};
    std::uint64_t mensajesContractNet{0};
    std::uint64_t alertas{0};
    std::uint64_t subastas{0};
    std::uint64_t celdasSobrecargadas{0};  ///< Suma por tick de IA de celdas con más de MAX_AGENTES_POR_CELDA.
    int maxSobrecargadas{0};               ///< Máximo de celdas sobrecargadas en un mismo tick de IA.
    int maxCerca{0};                       ///< Máximo de infectados a 2 celdas o menos de Mara a la vez.
    float saludFinal{0.0f};
    int objetos{0};                        ///< Objetos de misión que Mara llegó a recoger.
    std::uint64_t hashPrevioDeteccion{0};  ///< Estado al terminar el tick anterior a la detección (CP-056).
    std::uint64_t hashFinal{0};
};

/// Promedios de un modo para el resumen por pantalla.
struct ResumenModo {
    int simulaciones{0};
    int detecciones{0};
    int contactos{0};
    int victorias{0};
    int derrotas{0};
    double mediaDeteccion{0.0};  ///< Solo sobre las simulaciones con detección.
    double mediaContacto{0.0};   ///< Solo sobre las simulaciones con contacto.
    double mediaCobertura{0.0};
    double mediaMensajes{0.0};
    double mediaSobrecargadas{0.0};
    double mediaDuracion{0.0};   ///< Segundos simulados hasta el final (o el tope).
    double mediaMaxCerca{0.0};
};

struct ResumenCoordinacion {
    std::vector<MetricasSimulacion> simulaciones;  ///< Aislado y coordinado de cada repetición, en ese orden.
    ResumenModo aislado;
    ResumenModo coordinado;
    int paresDivergentes{0};       ///< Pares que difieren antes de la primera detección (debe ser 0).
    int contactoAntesCoordinado{0};  ///< Pares en que la horda coordinada alcanzó antes a Mara.
};

/// Guion fijo de la jugadora: va caminando por A* a cada objeto (en el orden del escenario), luego a
/// la radio, la instala manteniendo «interactuar» y termina en la zona de extracción. Replanifica
/// cada segundo desde donde esté. No mira a los infectados: es el mismo en los dos modos.
class GuionJugadora {
public:
    explicit GuionJugadora(const juego::Partida& p);
    /// Entrada para el siguiente paso de simulación.
    [[nodiscard]] juego::EntradaJugador entrada(const juego::Partida& p);
    /// Índice de la meta actual (0..objetos-1 = objetos, objetos = radio, objetos + 1 = extracción).
    [[nodiscard]] std::size_t meta() const noexcept { return meta_; }

private:
    [[nodiscard]] mundo::Celda celdaMeta(const juego::Partida& p) const noexcept;
    void planificar(const juego::Partida& p, mundo::Celda destino);

    navegacion::BuscadorMejorPrimero buscador_{navegacion::Algoritmo::AEstrella};
    navegacion::ResultadoBusqueda ruta_;
    std::size_t siguiente_{0};
    std::size_t meta_{0};
    mundo::Celda destino_{-1, -1};
    std::uint64_t tickPlan_{0};
    bool planificado_{false};
};

/// Huella del estado de la simulación (posiciones, estados y objetivos de los infectados y Mara).
/// Dos partidas con la misma huella en cada tick evolucionaron igual (CP-056, RNF-020).
[[nodiscard]] std::uint64_t hashEstado(const juego::Partida& p) noexcept;

/// Simula la repetición @p repeticion en el modo @p modo y devuelve sus métricas.
[[nodiscard]] MetricasSimulacion simularHorda(const ConfigCoordinacion& cfg, ModoHorda modo, int repeticion);

/// Columnas del CSV, en orden (RF-084).
[[nodiscard]] const std::vector<std::string_view>& columnasCoordinacion();

/// Ejecuta las 2 x repeticiones simulaciones y escribe el CSV en @p csv, con @p metadatos como
/// cabecera de comentarios «# » (RF-081).
ResumenCoordinacion ejecutarExperimentoCoordinacion(const ConfigCoordinacion& cfg,
                                                    const std::vector<std::string>& metadatos, std::ostream& csv);

}  // namespace refugio::experimentos
