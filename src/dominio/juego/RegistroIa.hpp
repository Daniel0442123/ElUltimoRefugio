#pragma once
/// @file RegistroIa.hpp
/// @brief Registro de eventos de IA: cambios de estado y de plan, y mensajes entre agentes (RF-072, RF-073).
///
/// Cada evento se formatea una sola vez como texto «campo\tcampo…» (arc42 §8.9) y se guarda en un
/// anillo de líneas de tamaño fijo, que es lo que lee la consola del panel. Si hay un sumidero
/// (puertos::IRegistro, por ejemplo el archivo logs/ia_<semilla>.log) la misma línea se le
/// entrega en el momento. Así panel y archivo muestran exactamente lo mismo.
///
/// Sin asignaciones de memoria (RNF-006): las líneas viven en un std::array y se escriben con
/// vsnprintf sobre ellas; una línea más larga que kLargoLinea se recorta.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "dominio/puertos/Puertos.hpp"

#if defined(__GNUC__) || defined(__clang__)
#define REFUGIO_FORMATO_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define REFUGIO_FORMATO_PRINTF(fmt, args)
#endif

namespace refugio::juego {

enum class TipoEventoIa : std::uint8_t { Transicion, Plan, Mensaje, Superviviente };
inline constexpr int kNumTiposEventoIa = 4;
[[nodiscard]] std::string_view nombreEventoIa(TipoEventoIa t) noexcept;

/// Id de agente que usa el superviviente de la radio en el registro.
inline constexpr std::uint16_t kAgenteSuperviviente = 0xFFFF;

struct LineaIa {
    static constexpr std::size_t kLargoLinea = 160;
    std::uint64_t tick{};
    TipoEventoIa tipo{};
    std::uint16_t agente{};
    std::uint16_t largo{};
    std::array<char, kLargoLinea> texto{};
    [[nodiscard]] std::string_view vista() const noexcept { return {texto.data(), largo}; }
};

class RegistroIa {
public:
    static constexpr std::size_t kCapacidad = 256;

    /// Sumidero opcional (no es dueño). nullptr = solo el anillo en memoria.
    void fijarSumidero(puertos::IRegistro* sumidero) noexcept { sumidero_ = sumidero; }
    [[nodiscard]] puertos::IRegistro* sumidero() const noexcept { return sumidero_; }

    /// Añade una línea con formato printf. No asigna memoria.
    void anotar(std::uint64_t tick, TipoEventoIa tipo, std::uint16_t agente, const char* formato, ...) noexcept
        REFUGIO_FORMATO_PRINTF(5, 6);

    [[nodiscard]] std::size_t tamano() const noexcept;
    /// La más reciente es ultima(0).
    [[nodiscard]] const LineaIa& ultima(std::size_t haceN) const noexcept;
    [[nodiscard]] std::uint64_t total() const noexcept { return total_; }
    [[nodiscard]] std::uint64_t totalDe(TipoEventoIa t) const noexcept { return porTipo_[static_cast<std::size_t>(t)]; }

private:
    std::array<LineaIa, kCapacidad> lineas_{};
    std::array<std::uint64_t, kNumTiposEventoIa> porTipo_{};
    std::uint64_t total_{0};
    puertos::IRegistro* sumidero_{nullptr};
};

}  // namespace refugio::juego
