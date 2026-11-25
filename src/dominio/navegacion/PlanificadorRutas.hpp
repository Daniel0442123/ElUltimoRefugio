#pragma once
/// @file PlanificadorRutas.hpp
/// @brief Cola de solicitudes de ruta con presupuesto de nodos por tick de IA (HU-23, RF-026, ADR-0007).
///
/// Decenas de infectados pueden pedir ruta en el mismo tick. Resolverlas todas de golpe produce
/// picos (RNF-002); aquí se reparten:
///
///   - Cada tick de IA, procesar() expande como mucho `PRESUPUESTO_NODOS_TICK` nodos en total.
///   - Las búsquedas son reanudables (BuscadorMejorPrimero::iniciar/avanzar): si el presupuesto se
///     acaba a mitad de una, sigue en el tick siguiente donde se quedó, con el mismo resultado que
///     si se hubiera hecho de una vez. Solo hay una búsqueda en curso a la vez, así la memoria es
///     O(V) y no O(V · agentes).
///   - Las solicitudes en espera se atienden por prioridad y antigüedad: prioridad efectiva =
///     prioridad + ticks esperando / kTicksEnvejecimiento. Una petición de baja prioridad sube un
///     nivel cada kTicksEnvejecimiento ticks y nunca se queda esperando para siempre. Con la misma
///     prioridad efectiva va antes la más antigua y, después, el agente de menor índice.
///   - Todo cuenta nodos, no tiempo: con la misma entrada el reparto es el mismo en cualquier
///     equipo y la simulación sigue siendo determinista (RNF-020).
///
/// El planificador no sabe nada de infectados: pregunta el origen y el destino de cada agente al
/// empezar su búsqueda (así usa la posición y el objetivo de ese momento) y entrega el resultado
/// con una llamada. Toda la memoria se reserva con reservar() (RNF-006).

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "dominio/mundo/Grafo.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"

namespace refugio::navegacion {

/// Métricas del último tick de IA (panel, RF-070, y prueba CP-026).
struct MetricasPlanificador {
    std::uint32_t expandidos{0};     ///< Nodos expandidos en el tick (<= presupuesto).
    std::uint32_t terminadas{0};     ///< Búsquedas que acabaron en el tick.
    std::uint32_t pendientes{0};     ///< Solicitudes esperando al final del tick (sin la que está en curso).
    bool enCurso{false};             ///< Quedó una búsqueda a medias para el tick siguiente.
    double tiempoUs{0.0};            ///< Tiempo de búsqueda del tick.
    std::uint32_t esperaMaxima{0};   ///< Ticks que esperó (desde la solicitud) la más lenta de las terminadas.
};

class PlanificadorRutas {
public:
    static constexpr std::uint32_t kTicksEnvejecimiento = 5;  ///< 0,5 s a 10 Hz: un nivel de prioridad
    static constexpr std::uint32_t kSinAgente = UINT32_MAX;

    /// Reserva para @p numAgentes agentes en un mapa de @p numNodos celdas.
    void reservar(std::uint32_t numAgentes, std::uint32_t numNodos);

    /// Algoritmo de las búsquedas siguientes (F2). Si cambia con una búsqueda a medias, esa
    /// búsqueda vuelve a empezar con el algoritmo nuevo en el próximo procesar().
    void fijarAlgoritmo(Algoritmo a) noexcept;
    [[nodiscard]] Algoritmo algoritmo() const noexcept { return buscador_.algoritmo(); }

    /// Pide ruta para @p agente con @p prioridad (mayor = más urgente). Si ya la había pedido, solo
    /// se actualiza la prioridad y se conserva su antigüedad.
    void solicitar(std::uint32_t agente, std::uint8_t prioridad);
    /// Retira la solicitud de @p agente (y abandona su búsqueda si estaba en curso).
    void cancelar(std::uint32_t agente) noexcept;
    /// El mapa cambió (RF-025): la búsqueda en curso vuelve a empezar con el mapa nuevo.
    void reiniciarEnCurso() noexcept;
    /// Olvida todas las solicitudes.
    void vaciar() noexcept;

    [[nodiscard]] bool solicitado(std::uint32_t agente) const noexcept {
        return agente < enCola_.size() && enCola_[agente] != 0;
    }
    [[nodiscard]] std::uint32_t agenteEnCurso() const noexcept { return enCurso_ ? actual_.agente : kSinAgente; }
    [[nodiscard]] std::size_t pendientes() const noexcept { return cola_.size(); }
    [[nodiscard]] const MetricasPlanificador& metricas() const noexcept { return metricas_; }
    /// Ticks de IA procesados (la antigüedad se mide con este reloj).
    [[nodiscard]] std::uint64_t ticks() const noexcept { return tick_; }

    /// Un tick de IA: expande como mucho @p presupuesto nodos atendiendo la cola.
    /// @p consulta(agente) devuelve std::pair<Nodo, Nodo> con el origen y el destino de ese agente
    /// ahora; se llama al empezar su búsqueda. @p entregar(agente, const ResultadoBusqueda&) recibe
    /// cada búsqueda terminada. Se registra el orden de expansión del agente @p registrarAgente.
    template <class Consulta, class Entrega>
    void procesar(const mundo::GrafoRejilla& grafo, std::uint32_t presupuesto, Consulta&& consulta, Entrega&& entregar,
                  std::uint32_t registrarAgente = kSinAgente);

private:
    struct Solicitud {
        std::uint32_t agente;
        std::uint8_t prioridad;
        std::uint64_t tick;  ///< Tick de IA de la solicitud (antigüedad).
    };
    /// Índice en cola_ de la solicitud que toca atender.
    [[nodiscard]] std::size_t siguiente() const noexcept;
    void quitarDeCola(std::size_t i) noexcept;

    BuscadorMejorPrimero buscador_{Algoritmo::AEstrella};
    ResultadoBusqueda resultado_;
    std::vector<Solicitud> cola_;
    std::vector<std::uint8_t> enCola_;  ///< Por agente: 1 si espera o está en curso.
    Solicitud actual_{};
    bool enCurso_{false};
    bool reiniciar_{false};  ///< La búsqueda en curso tiene que volver a empezar.
    std::uint64_t tick_{0};
    MetricasPlanificador metricas_;
};

template <class Consulta, class Entrega>
void PlanificadorRutas::procesar(const mundo::GrafoRejilla& grafo, std::uint32_t presupuesto, Consulta&& consulta,
                                 Entrega&& entregar, std::uint32_t registrarAgente) {
    ++tick_;
    metricas_ = {};
    while (metricas_.expandidos < presupuesto) {
        if (!enCurso_) {
            if (cola_.empty()) break;
            const std::size_t i = siguiente();
            actual_ = cola_[i];
            quitarDeCola(i);
            enCurso_ = true;
            reiniciar_ = true;
        }
        if (reiniciar_) {
            reiniciar_ = false;
            const std::pair<mundo::Nodo, mundo::Nodo> od = consulta(actual_.agente);
            buscador_.iniciar(grafo, od.first, od.second, resultado_, actual_.agente == registrarAgente);
        }
        const std::uint32_t antes = resultado_.expandidos;
        const double tAntes = resultado_.tiempoUs;
        const EstadoBusqueda e = buscador_.avanzar(grafo, resultado_, presupuesto - metricas_.expandidos);
        metricas_.expandidos += resultado_.expandidos - antes;
        metricas_.tiempoUs += resultado_.tiempoUs - tAntes;
        if (e == EstadoBusqueda::EnCurso) break;  // presupuesto agotado: sigue en el próximo tick
        enCurso_ = false;
        enCola_[actual_.agente] = 0;
        ++metricas_.terminadas;
        const auto espera = static_cast<std::uint32_t>(tick_ - actual_.tick);
        if (espera > metricas_.esperaMaxima) metricas_.esperaMaxima = espera;
        entregar(actual_.agente, static_cast<const ResultadoBusqueda&>(resultado_));
    }
    metricas_.pendientes = static_cast<std::uint32_t>(cola_.size());
    metricas_.enCurso = enCurso_;
}

}  // namespace refugio::navegacion
