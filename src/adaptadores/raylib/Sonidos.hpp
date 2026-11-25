#pragma once
/// @file Sonidos.hpp
/// @brief Efectos de sonido sintetizados en memoria (sin archivos de audio) para que el volumen
/// del menú de opciones (RF-092) tenga algo que controlar.
///
/// Si no hay dispositivo de audio, todo queda en silencio y el juego sigue.

#include <array>

#include <raylib.h>

namespace refugio::adaptadores {

enum class Efecto : unsigned char { Recoger, Senuelo, Barricada, Menu, Guardado, Error, Tutorial };
inline constexpr int kNumEfectos = 7;

class Sonidos {
public:
    Sonidos();
    ~Sonidos();
    Sonidos(const Sonidos&) = delete;
    Sonidos& operator=(const Sonidos&) = delete;

    void reproducir(Efecto e) const;
    /// 0..1, volumen general (SetMasterVolume).
    void volumen(float v) const;
    [[nodiscard]] bool disponibles() const noexcept { return listo_; }

private:
    bool listo_{false};
    std::array<Sound, kNumEfectos> sonidos_{};
};

}  // namespace refugio::adaptadores
