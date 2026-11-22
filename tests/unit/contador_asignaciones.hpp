#pragma once
// Contador global de asignaciones del montículo para CP-106 (RNF-006). Solo cuenta mientras
// está activo, así Catch2 puede asignar libremente fuera de la ventana medida.
#include <cstdint>

namespace refugio::pruebas {
void activarConteoAsignaciones(bool activo) noexcept;
[[nodiscard]] std::uint64_t asignacionesContadas() noexcept;

/// Cuenta las asignaciones hechas durante la vida del objeto.
class VentanaAsignaciones {
public:
    VentanaAsignaciones() noexcept : inicio_(asignacionesContadas()) { activarConteoAsignaciones(true); }
    ~VentanaAsignaciones() { activarConteoAsignaciones(false); }
    VentanaAsignaciones(const VentanaAsignaciones&) = delete;
    VentanaAsignaciones& operator=(const VentanaAsignaciones&) = delete;
    [[nodiscard]] std::uint64_t asignaciones() const noexcept { return asignacionesContadas() - inicio_; }

private:
    std::uint64_t inicio_;
};
}  // namespace refugio::pruebas
