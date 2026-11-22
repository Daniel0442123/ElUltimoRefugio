#pragma once
/// @file MaquinaEstados.hpp
/// @brief Máquina de estados finitos del infectado (RF-044, ADR-0008).
///
/// Primer corte de RF-044 con los estados Explorar (deambular), Investigar, Perseguir, Buscar y
/// Atacar. Interceptar y AtacarBarricada llegan con la coordinación (RF-049, RF-052).
///
/// La transición es una función pura de (estado, percepción, contexto): no lee el mundo, así se
/// prueba fila por fila contra la tabla del SRS (CP-044) y se puede reemplazar por el modelo
/// híbrido de utilidad de ADR-0008 sin tocar la partida.

#include <cstdint>
#include <string_view>

#include "dominio/mundo/Grafo.hpp"

namespace refugio::agentes {

enum class EstadoInfectado : std::uint8_t { Explorar, Investigar, Perseguir, Buscar, Atacar };

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
};

[[nodiscard]] std::string_view nombreEstado(EstadoInfectado e) noexcept;
[[nodiscard]] std::string_view nombreMotivo(MotivoTransicion m) noexcept;

/// Lo que el agente percibió en este tick de IA (construido con Percepcion.hpp).
struct PercepcionInfectado {
    bool veAMara{false};
    float distanciaMara{0.0f};  ///< Solo válida si veAMara.
    bool oyeRuido{false};
    bool recibeAlerta{false};  ///< Llegó una alerta de otro infectado (modo coordinado, RF-047).
};

/// Estado interno del agente que la FSM necesita además de la percepción.
struct ContextoFsm {
    float tiempoEnEstado{0.0f};  ///< Segundos desde la última transición.
    bool llegoAlObjetivo{false};
    bool sinRuta{false};          ///< La última búsqueda de ruta falló (RF-027).
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
