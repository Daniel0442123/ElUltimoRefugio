/// @file CarpetaEjecutable.hpp
/// @brief Carpeta de datos del juego relativa al ejecutable (versión portable, RNF-025).
///
/// El juego lee config/, maps/, bench/ y opt/ y escribe saves/, logs/ y results/ con rutas
/// relativas al directorio de trabajo. En la versión portable esos archivos están junto a
/// refugio.exe, pero el programa puede abrirse desde otra carpeta (un acceso directo, una consola
/// abierta en otro sitio). Al arrancar, si el directorio de trabajo no tiene config/ia.json y la
/// carpeta del ejecutable sí, el programa pasa a trabajar en la carpeta del ejecutable.
/// En el repositorio (build/Release/refugio.exe lanzado desde la raíz) no cambia nada.

#pragma once

#include <filesystem>
#include <optional>

namespace refugio::adaptadores {

/// Archivo que marca una carpeta de datos del juego.
inline constexpr const char* kMarcaCarpetaDatos = "config/ia.json";

/// Ruta absoluta del ejecutable en marcha; vacía si el sistema no la da.
[[nodiscard]] std::filesystem::path rutaEjecutable();

/// Carpeta a la que hay que cambiar: la del ejecutable cuando `actual` no tiene la marca y esa
/// carpeta sí. nullopt si hay que quedarse en `actual`.
[[nodiscard]] std::optional<std::filesystem::path> carpetaDatos(const std::filesystem::path& actual,
                                                               const std::filesystem::path& ejecutable);

/// Aplica carpetaDatos al proceso. Devuelve la carpeta nueva, o nullopt si no hubo cambio.
std::optional<std::filesystem::path> usarCarpetaDelEjecutable();

}  // namespace refugio::adaptadores
