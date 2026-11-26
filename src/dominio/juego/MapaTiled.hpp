#pragma once
/// @file MapaTiled.hpp
/// @brief Mapas de Tiled en JSON (RF-001, IS-01, ADR-0019).
///
/// Lee un mapa ortogonal de Tiled 1.10 o posterior guardado en JSON («Guardar como… → *.json»,
/// con las capas de patrones en CSV, que es lo que Tiled usa por defecto en JSON) y construye el
/// mismo Escenario que produce crearEscenarioAleatorio, así que el resto del juego no cambia.
///
/// Capas, por nombre (sin distinguir mayúsculas):
///
///   - «suelo» (patrones, opcional): solo decorativa; si está, debe tener el tamaño del mapa.
///   - «colision» (patrones, obligatoria): todo patrón distinto de 0 es muro.
///   - «costo» (patrones, opcional): multiplicador de terreno m (RF-031). El patrón número k de su
///     conjunto (k = gid − firstgid, desde 0) vale m = k + 1, recortado a [1, 10]; 0 es m = 1.
///   - «objetos» (objetos): medicamento, combustible, bateria, radio, almacen y puerta (RF-006).
///   - «spawns» (objetos): mara, superviviente e infectado; el rol de un infectado (RF-048) es su
///     propiedad «rol» («explorador» o «perseguidor»), o el tipo «explorador» / «perseguidor».
///   - «zonas» (objetos): extraccion; vale el centro del rectángulo.
///
/// El tipo de cada objeto se toma de «type» (o «class») y, si está vacío, de «name». Un objeto
/// rectangular cuenta por su centro y uno puntual por su posición; la celda es la posición en
/// píxeles dividida por tilewidth y tileheight.
///
/// Un mapa inválido nunca lanza: el resultado lleva el error con la capa, el objeto o el campo
/// (RNF-016), como la configuración.

#include <string>
#include <string_view>

#include "dominio/juego/Partida.hpp"

namespace refugio::juego {

/// Lado máximo del mapa en celdas (el mismo límite que el benchmark, RF-080).
inline constexpr int kMaxLadoTiled = 256;
/// Como en las partidas guardadas.
inline constexpr int kMaxInfectadosTiled = 256;

struct ResultadoMapaTiled {
    Escenario escenario;
    std::string error;  ///< Vacío si el mapa es válido.
    [[nodiscard]] bool valido() const noexcept { return error.empty(); }
};

/// Analiza el texto de un mapa de Tiled. @p archivo solo se usa en los mensajes de error.
[[nodiscard]] ResultadoMapaTiled leerMapaTiled(std::string_view texto, std::string_view archivo = "mapa");

}  // namespace refugio::juego
