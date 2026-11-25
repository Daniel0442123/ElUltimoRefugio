#pragma once
/// @file MaquinaEstados.hpp
/// @brief Máquina de estados finitos del infectado (RF-044, ADR-0008).
///
/// Estados de RF-044: Explorar (deambular), Investigar, Perseguir, Buscar, Atacar e Interceptar
/// (tarea Interceptar o Flanquear adjudicada por Contract Net, RF-049) y AtacarBarricada (la ruta
/// quedó bloqueada por una barricada y el rodeo cuesta más de FACTOR_DESVIO_BARRICADA, RF-052).
///
/// La transición es una función pura de (estado, percepción, contexto): no lee el mundo, así se
/// prueba fila por fila contra la tabla del SRS (CP-044) y se puede reemplazar por el modelo
/// híbrido de utilidad de ADR-0008 sin tocar la partida.

#include <cstdint>
#include <string_view>

#include "dominio/mundo/Grafo.hpp"

namespace refugio::agentes {

enum class EstadoInfectado : std::uint8_t { Explorar, Investigar, Perseguir, Buscar, Atacar, Interceptar, AtacarBarricada };
inline constexpr int kNumEstadosInfectado = 7;

/// Táctica elegida por utilidad (RF-054). Perseguir es el valor neutro: con ella la FSM se comporta
/// como antes de existir la selección de táctica.
enum class Tactica : std::uint8_t { Perseguir, Interceptar, Explorar };
inline constexpr int kNumTacticas = 3;
[[nodiscard]] std::string_view nombreTactica(Tactica t) noexcept;

enum class MotivoTransicion : std::uint8_t {
    Ninguno,
    VeAMara,          ///< Explorar / Investigar / Buscar -> Perseguir
    OyeRuido,         ///< Explorar / Buscar -> Investigar
    LlegaAlOrigen,    ///< Investigar -> Buscar (sin evidencia nueva)
    PierdeVision,     ///< Perseguir -> Buscar en la última posición conocida
    BusquedaAgotada,  ///< Buscar -> Explorar tras tBuscar segundos
    EnAlcance,        ///< Perseguir -> Atacar (distancia <= alcance)
    FueraDeAlcance,   ///< Atacar -> Perseguir / Buscar
    SinRuta,          ///< RF-027: la búsqueda no encontró ruta -> otro comportamiento
    RecibeAlerta,     ///< Explorar / Buscar -> Investigar el punto que avisó otro infectado (RF-047)
    AsignaTarea,      ///< Contract Net adjudica Interceptar o Flanquear -> Interceptar (RF-049)
    RutaBloqueada,    ///< Barricada sin rodeo razonable -> AtacarBarricada (RF-052)
    BarricadaDestruida,  ///< AtacarBarricada -> Investigar (o Perseguir si ve a Mara)
    EligeTactica,     ///< RF-054: la táctica Interceptar puntúa más que Perseguir -> Interceptar
    EvidenciaDebil,   ///< RF-054: Explorar puntúa más que seguir la pista -> Explorar
};

[[nodiscard]] std::string_view nombreEstado(EstadoInfectado e) noexcept;
[[nodiscard]] std::string_view nombreMotivo(MotivoTransicion m) noexcept;

/// Lo que el agente percibió en este tick de IA (construido con Percepcion.hpp).
struct PercepcionInfectado {
    bool veAMara{false};
    float distanciaMara{0.0f};  ///< Solo válida si veAMara.
    bool oyeRuido{false};
    bool recibeAlerta{false};  ///< Llegó una alerta de otro infectado (modo coordinado, RF-047).
    bool tareaInterceptar{false};  ///< Le adjudicaron Interceptar o Flanquear (RF-049).
    Tactica tactica{Tactica::Perseguir};  ///< Mejor táctica según sus evidencias (RF-054).
};

/// Estado interno del agente que la FSM necesita además de la percepción.
struct ContextoFsm {
    float tiempoEnEstado{0.0f};  ///< Segundos desde la última transición.
    bool llegoAlObjetivo{false};
    bool sinRuta{false};          ///< La última búsqueda de ruta falló (RF-027).
    bool rutaBloqueada{false};    ///< Barricada en la ruta y rodeo > FACTOR_DESVIO_BARRICADA (RF-052).
    bool barricadaDestruida{false};  ///< La barricada que atacaba ya no está.
};

struct ParametrosFsm {
    float alcanceAtaque{0.9f};      ///< Celdas.
    float histeresisAtaque{0.5f};   ///< Sale de Atacar a alcance + histéresis (evita parpadeo).
    float tiempoBuscar{8.0f};       ///< Segundos en Buscar antes de volver a Explorar (RF-044).
};

struct Transicion {
    EstadoInfectado hacia;
    MotivoTransicion motivo;  ///< Ninguno si no hay cambio.
};

/// Tabla de transiciones de RF-044. Complejidad O(1).
[[nodiscard]] Transicion decidirTransicion(EstadoInfectado actual, const PercepcionInfectado& p,
                                          const ContextoFsm& c, const ParametrosFsm& par) noexcept;

}  // namespace refugio::agentes
