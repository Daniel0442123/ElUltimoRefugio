#pragma once
/// @file Puertos.hpp
/// @brief Puertos del dominio hacia el exterior (arquitectura hexagonal, ADR-0003).
///
/// El dominio solo conoce estas interfaces; los adaptadores (raylib, archivos, Tiled) las
/// implementan en src/adaptadores. Se irán completando con cada historia de usuario.

#include <cstdint>
#include <string_view>

namespace refugio::puertos {

/// Fuente de tiempo. El juego usa el reloj real; pruebas y benchmark usan un reloj fijo.
class IReloj {
public:
    virtual ~IReloj() = default;
    [[nodiscard]] virtual double segundos() const = 0;
};

/// Registro de eventos de IA: cambios de plan y mensajes entre agentes (RF-072, RF-073).
class IRegistro {
public:
    virtual ~IRegistro() = default;
    virtual void escribir(std::uint64_t tick, std::string_view tipo, std::string_view linea) = 0;
};

}  // namespace refugio::puertos
