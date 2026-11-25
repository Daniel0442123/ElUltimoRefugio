#pragma once
/// @file RegistroArchivo.hpp
/// @brief Adaptador de puertos::IRegistro que escribe logs/ia_<semilla>.log (RF-072, RF-073, arc42 §8.9).
///
/// Formato de cada línea: «tick\ttipo\tcampos…», con los campos tal como los formatea el
/// dominio (RegistroIa). La primera línea es un comentario con la versión y la semilla.
/// Si la carpeta no se puede crear o el archivo no se puede abrir, el registro queda inactivo
/// (abierto() == false) y el juego sigue sin él.

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

#include "dominio/puertos/Puertos.hpp"

namespace refugio::adaptadores {

class RegistroArchivo final : public puertos::IRegistro {
public:
    /// Abre (y vacía) @p carpeta/ia_<semilla>.log, creando la carpeta si hace falta.
    RegistroArchivo(const std::filesystem::path& carpeta, std::uint64_t semilla);
    ~RegistroArchivo() override;
    RegistroArchivo(const RegistroArchivo&) = delete;
    RegistroArchivo& operator=(const RegistroArchivo&) = delete;

    void escribir(std::uint64_t tick, std::string_view tipo, std::string_view linea) override;
    /// Fuerza el volcado al disco del búfer de stdio.
    void vaciar() noexcept;

    [[nodiscard]] bool abierto() const noexcept { return archivo_ != nullptr; }
    [[nodiscard]] const std::filesystem::path& ruta() const noexcept { return ruta_; }
    [[nodiscard]] std::uint64_t lineas() const noexcept { return lineas_; }

    /// «ia_<semilla>.log»
    [[nodiscard]] static std::string nombreArchivo(std::uint64_t semilla);

private:
    std::FILE* archivo_{nullptr};
    std::filesystem::path ruta_;
    std::uint64_t lineas_{0};
};

}  // namespace refugio::adaptadores
