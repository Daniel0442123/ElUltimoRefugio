#pragma once
/// @file RecursosProceso.hpp
/// @brief CPU, memoria y edad del proceso actual, leídos del sistema operativo (RNF-004, RNF-005,
///        RNF-008). Windows: GetProcessTimes y GetProcessMemoryInfo; Linux: getrusage y /proc.
///
/// Está fuera del dominio (RNF-023) y no incluye raylib: windows.h choca con raylib.h, así que
/// todo lo del sistema queda en RecursosProceso.cpp.

#include <cstdint>

namespace refugio::adaptadores {

struct RecursosProceso {
    double cpuSegundos{0.0};          ///< Tiempo de CPU del proceso (usuario + núcleo), todos los hilos.
    std::uint64_t memoriaBytes{0};     ///< Memoria residente actual (RSS / Working Set); 0 si no se sabe.
    std::uint64_t memoriaPicoBytes{0}; ///< Pico de memoria residente desde que empezó el proceso.
};

/// Lectura instantánea; barata (unos microsegundos), se puede llamar una vez por segundo.
[[nodiscard]] RecursosProceso leerRecursosProceso() noexcept;

/// Segundos desde que el sistema creó el proceso (incluye la carga del ejecutable y de las DLL,
/// antes de main). Negativo si el sistema no lo ofrece. En Linux la resolución es de 10 ms.
[[nodiscard]] double segundosDesdeCreacionProceso() noexcept;

}  // namespace refugio::adaptadores
