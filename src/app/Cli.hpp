#pragma once
/// @file Cli.hpp
/// @brief Modos sin ventana: --bench, --estres-recursion y --coord-exp (RF-080, RNF-027, RF-084).

#include <string>
#include <vector>

namespace refugio::app {

/// Devuelve true si los argumentos piden un modo sin ventana; en ese caso lo ejecuta y deja
/// el código de salida en @p codigo.
bool ejecutarModoSinVentana(int argc, char** argv, int& codigo);

/// Metadatos del equipo para la cabecera de los resultados (RF-081).
std::vector<std::string> metadatosEquipo();

}  // namespace refugio::app
