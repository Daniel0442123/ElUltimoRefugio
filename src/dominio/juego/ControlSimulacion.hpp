#pragma once
/// @file ControlSimulacion.hpp
/// @brief Pausa, paso a paso y velocidad de la simulación (HU-22, RF-075, ADR-0005).
///
/// El bucle de juego tiene paso fijo: cada frame suma su tiempo real a un acumulador y ejecuta
/// tantos pasos de 1/60 s como quepan. El control solo cambia ese acumulador:
///
///   - con velocidad v, el frame suma v · t_real (0,25× → 15 pasos por segundo real a 60 Hz);
///   - en pausa no se acumula nada y solo corren los pasos pedidos uno a uno;
///   - el frame se recorta a 0,25 s reales para no entrar en espiral tras un tirón.
///
/// La partida nunca ve el tiempo real: recibe los mismos pasos con las mismas entradas a
/// cualquier velocidad, así que el resultado determinista no cambia (CP-075, RNF-020).
///
/// El acumulador usa double y una tolerancia de 1 µs: sin ella, cuatro frames de 1/240 s
/// pueden sumar un poco menos de 1/60 s y el paso caería en el quinto frame.

#include <array>
#include <cstddef>

namespace refugio::juego {

class ControlSimulacion {
public:
    /// Velocidades disponibles, de la más lenta a la más rápida (RF-075).
    static constexpr std::array<double, 5> kVelocidades{0.25, 0.5, 1.0, 2.0, 4.0};
    static constexpr std::size_t kIndiceNormal = 2;  ///< 1×
    static constexpr double kFrameMaximo = 0.25;     ///< segundos reales por frame como máximo

    /// @p dt duración de un paso de simulación (1 / TICK_SIM_HZ).
    explicit ControlSimulacion(double dt = 1.0 / 60.0) noexcept : dt_(dt > 0.0 ? dt : 1.0 / 60.0) {}

    [[nodiscard]] bool pausada() const noexcept { return pausada_; }
    void pausar(bool p) noexcept {
        pausada_ = p;
        acumulado_ = 0.0;
        if (!p) pasosPedidos_ = 0;
    }
    void alternarPausa() noexcept { pausar(!pausada_); }

    /// Pide exactamente un paso. Solo tiene efecto en pausa: sin pausa la simulación ya avanza.
    void pedirPaso() noexcept {
        if (pausada_) ++pasosPedidos_;
    }

    [[nodiscard]] std::size_t indiceVelocidad() const noexcept { return indice_; }
    [[nodiscard]] double velocidad() const noexcept { return kVelocidades[indice_]; }
    /// Fija la velocidad por índice (se recorta al rango válido).
    void fijarIndiceVelocidad(std::size_t i) noexcept { indice_ = i < kVelocidades.size() ? i : kVelocidades.size() - 1; }
    /// Siguiente velocidad (se queda en 4×). Devuelve la nueva velocidad.
    double masRapido() noexcept {
        if (indice_ + 1 < kVelocidades.size()) ++indice_;
        return velocidad();
    }
    /// Velocidad anterior (se queda en 0,25×). Devuelve la nueva velocidad.
    double masLento() noexcept {
        if (indice_ > 0) --indice_;
        return velocidad();
    }

    /// Pasos de simulación que hay que ejecutar en un frame que duró @p segundosReales.
    [[nodiscard]] int pasosParaFrame(double segundosReales) noexcept {
        if (pausada_) {
            const int n = pasosPedidos_;
            pasosPedidos_ = 0;
            return n;
        }
        double t = segundosReales > 0.0 ? segundosReales : 0.0;
        if (t > kFrameMaximo) t = kFrameMaximo;
        acumulado_ += t * velocidad();
        int n = 0;
        while (acumulado_ + kTolerancia >= dt_) {
            acumulado_ -= dt_;
            ++n;
        }
        if (acumulado_ < 0.0) acumulado_ = 0.0;
        return n;
    }

    /// Olvida el tiempo acumulado (al cerrar el menú o al cargar una partida).
    void reiniciarAcumulador() noexcept { acumulado_ = 0.0; }
    [[nodiscard]] double dt() const noexcept { return dt_; }

private:
    static constexpr double kTolerancia = 1e-6;  // 1 µs: cubre el redondeo de dt en float
    double dt_;
    double acumulado_{0.0};
    std::size_t indice_{kIndiceNormal};
    bool pausada_{false};
    int pasosPedidos_{0};
};

}  // namespace refugio::juego
