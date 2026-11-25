#pragma once
/// @file ArchivosLocales.hpp
/// @brief Lectura y escritura de archivos locales (HU-19, RF-090 a RF-093, ADR-0011).
///
/// - Escritura atómica (RF-090, RNF-017): se escribe «destino.tmp», se vacía al disco y se
///   renombra sobre el destino. Si el proceso muere a mitad, el archivo anterior sigue intacto.
/// - Lectura con tope de tamaño (RNF-021): un archivo de más de 10 MB se rechaza sin leerlo.
/// - Ranuras de partida: saves/slotN.sav.
/// - Configuración: config/juego.json, config/controles.json y config/ia.json. Un archivo que
///   falta no es un error (se usan los valores por defecto); uno inválido sí se informa.
///
/// Nada de esto abre conexiones de red (RNF-022).

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "dominio/config/Configuracion.hpp"
#include "dominio/persistencia/PartidaGuardada.hpp"

namespace refugio::adaptadores {

inline constexpr int kNumRanuras = 3;

struct LecturaArchivo {
    bool existe{false};
    bool ok{false};  ///< Se leyó completo.
    bool grande{false};  ///< Supera el tope y no se leyó.
    std::string error;
    std::string datos;
};

/// Lee @p ruta completo si ocupa @p maxBytes o menos.
[[nodiscard]] LecturaArchivo leerArchivo(const std::filesystem::path& ruta,
                                         std::uintmax_t maxBytes = persistencia::kMaxBytesArchivo);

/// Escribe @p datos en @p ruta de forma atómica (temporal + renombrado), creando la carpeta si
/// hace falta. Devuelve "" si todo fue bien o el motivo del fallo.
[[nodiscard]] std::string escribirAtomico(const std::filesystem::path& ruta, std::string_view datos);

/// «saves/slot<N>.sav» dentro de @p carpeta.
[[nodiscard]] std::filesystem::path rutaRanura(const std::filesystem::path& carpeta, int ranura);

/// Guarda la partida en la ranura. Devuelve "" o el motivo del fallo.
[[nodiscard]] std::string guardarRanura(const std::filesystem::path& carpeta, int ranura,
                                        const persistencia::PartidaGuardada& g);

/// Carga y reconstruye la partida de la ranura (Vacia si no hay archivo). Nunca lanza.
[[nodiscard]] persistencia::PartidaRestaurada cargarRanura(const std::filesystem::path& carpeta, int ranura);

/// Toda la configuración leída al iniciar, con los problemas encontrados.
struct ConfigLocal {
    config::OpcionesJuego opciones{};
    config::Controles controles{};
    config::ParametrosIa ia{};
    std::vector<std::string> errores;
};

/// Lee juego.json, controles.json e ia.json de @p carpeta (normalmente «config»).
[[nodiscard]] ConfigLocal leerConfigLocal(const std::filesystem::path& carpeta);
/// Guarda juego.json y controles.json (lo que cambia el menú de opciones). "" o el motivo.
[[nodiscard]] std::string guardarConfigLocal(const std::filesystem::path& carpeta, const config::OpcionesJuego& o,
                                             const config::Controles& c);

}  // namespace refugio::adaptadores
