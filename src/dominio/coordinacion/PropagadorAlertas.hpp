#pragma once
/// @file PropagadorAlertas.hpp
/// @brief Propagación recursiva acotada de alertas en el grafo de comunicación (RF-047, ADR-0009).
///
/// STB §03: «Recursividad: propagación acotada de alertas en el grafo de comunicación, con
/// conjunto de visitados». El grafo de comunicación no se construye: dos agentes están
/// conectados si su distancia es <= radioComunicacion, y los vecinos se piden al Quadtree.
///
/// propagar(a, p):
///   caso base 1: p == profundidadMax          -> no se reenvía
///   para cada vecino b de a no visitado:      -> caso base 2: ya visitado
///       visitados += b; entregar(b, salto p+1, confianza * atenuacion^(p+1))
///   para cada b recién alertado: propagar(b, p+1)   (progreso: p crece estrictamente)
///
/// Se marcan primero todos los vecinos directos y luego se recurre, de modo que cada agente
/// queda con el menor salto posible desde quien lo alertó (evita el sesgo de un DFS puro).
///
/// Terminación: p está acotado por profundidadMax y |visitados| por n.
/// Tiempo: O(n * k) con k vecinos promedio. Pila: O(profundidadMax) marcos. Visitados: O(n).

#include <cstdint>
#include <vector>

#include "dominio/navegacion/Quadtree.hpp"

namespace refugio::coordinacion {

using navegacion::Quadtree;
using navegacion::Vec2;

struct ParametrosAlerta {
    float radioComunicacion{5.0f};  ///< COM_RADIO, en celdas.
    int profundidadMax{3};           ///< ALERTA_PROF_MAX.
    float atenuacion{0.7f};          ///< ALERTA_ATENUACION por salto.
};

struct MensajeAlerta {
    std::uint32_t emisor;
    std::uint32_t receptor;
    int salto;
    float confianza;
};

struct ResultadoPropagacion {
    std::vector<MensajeAlerta> mensajes;   ///< Para el registro de mensajes (RF-073).
    std::vector<int> saltoPorAgente;        ///< -1 si no fue alertado; 0 para el origen.
    int profundidadAlcanzada{0};            ///< Mayor profundidad de llamada observada.
    std::uint64_t llamadas{0};              ///< Número de invocaciones recursivas.
    std::size_t bytesPilaEstimados{0};      ///< Distancia entre marcos de pila (análisis de pila).
};

class PropagadorAlertas {
public:
    explicit PropagadorAlertas(ParametrosAlerta parametros = {}) : parametros_(parametros) {}

    /// @param posiciones posición de cada agente (el índice es su id).
    /// @param indice     Quadtree ya construido con esas posiciones.
    /// @param origen     agente que detectó a Mara.
    /// @param confianza  confianza de la evidencia original (>= 0,6 según RF-047).
    ResultadoPropagacion propagar(const std::vector<Vec2>& posiciones, const Quadtree& indice,
                                  std::uint32_t origen, float confianza);

    /// Igual que propagar() pero escribe en @p r reutilizando su memoria. Tras reservar(n), no
    /// asigna memoria con hasta n agentes (RNF-006): se usa dentro del bucle de juego.
    void propagarEn(const std::vector<Vec2>& posiciones, const Quadtree& indice, std::uint32_t origen,
                    float confianza, ResultadoPropagacion& r);

    /// Reserva visitados, búferes por nivel y mensajes para @p maxAgentes agentes.
    void reservar(std::size_t maxAgentes, ResultadoPropagacion& r);

    [[nodiscard]] const ParametrosAlerta& parametros() const noexcept { return parametros_; }

private:
    void propagarRec(std::uint32_t agente, int profundidad, float confianza);

    ParametrosAlerta parametros_;
    const std::vector<Vec2>* posiciones_{nullptr};
    const Quadtree* indice_{nullptr};
    ResultadoPropagacion* resultado_{nullptr};
    std::vector<std::uint8_t> visitado_;
    std::vector<std::vector<std::uint32_t>> bufferPorNivel_;  ///< Reutilizados: sin asignar en régimen.
    std::uintptr_t direccionPilaBase_{0};
};

}  // namespace refugio::coordinacion
