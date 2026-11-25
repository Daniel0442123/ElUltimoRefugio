#pragma once
/// @file Tutorial.hpp
/// @brief Tutorial de sigilo de la Escena 1 (HU-20, RF-017, RNF-011).
///
/// Cuatro indicaciones en orden: moverse, moverse en sigilo, correr (y saber que hace ruido) y
/// recoger un objeto. Solo se muestra la indicación actual y desaparece cuando la jugadora hace
/// la acción; las acciones hechas antes de tiempo no cuentan para los pasos siguientes. El
/// tutorial solo observa la partida: no cambia la simulación.

#include <cstdint>
#include <string_view>

namespace refugio::juego {

class Partida;

enum class PasoTutorial : std::uint8_t { Mover, Sigilo, Correr, Recoger, Terminado };
inline constexpr int kNumPasosTutorial = 4;

[[nodiscard]] std::string_view nombrePaso(PasoTutorial p) noexcept;

class Tutorial {
public:
    /// Celdas que hay que recorrer en cada paso de movimiento.
    static constexpr float kDistanciaMover = 2.0f;
    static constexpr float kDistanciaSigilo = 1.5f;
    static constexpr float kDistanciaCorrer = 3.0f;

    /// Con @p activo == false empieza ya terminado (sin indicaciones).
    explicit Tutorial(bool activo = true) noexcept;

    /// Llamar una vez tras cada paso de simulación de @p p.
    void observar(const Partida& p) noexcept;
    /// «Saltar tutorial»: termina sin mostrar más indicaciones.
    void saltar() noexcept;

    [[nodiscard]] PasoTutorial paso() const noexcept { return paso_; }
    [[nodiscard]] bool activo() const noexcept { return paso_ != PasoTutorial::Terminado; }
    [[nodiscard]] bool saltado() const noexcept { return saltado_; }
    /// Fracción hecha del paso actual (0..1), para la barra de la indicación.
    [[nodiscard]] float progreso() const noexcept;
    /// RNF-011: segundos de simulación desde el inicio hasta completar el último paso (o hasta
    /// ahora, si no ha terminado). Al saltarlo se congela.
    [[nodiscard]] double segundos() const noexcept { return segundos_; }
    /// Cuántos pasos se completaron (los saltados no cuentan).
    [[nodiscard]] int completados() const noexcept { return completados_; }

private:
    void avanzar() noexcept;

    PasoTutorial paso_{PasoTutorial::Mover};
    bool saltado_{false};
    bool iniciado_{false};
    float posX_{0.0f}, posY_{0.0f};
    float recorrido_{0.0f};
    int objetosInicio_{0};
    int completados_{0};
    double segundos_{0.0};
    double tInicio_{0.0};
};

}  // namespace refugio::juego
