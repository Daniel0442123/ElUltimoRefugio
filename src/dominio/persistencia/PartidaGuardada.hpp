#pragma once
/// @file PartidaGuardada.hpp
/// @brief Guardar y cargar la partida (HU-19, RF-090, RF-091, RF-093, ADR-0011, ADR-0016).
///
/// La simulación es determinista (ADR-0012): con la misma semilla, los mismos parámetros y las
/// mismas entradas por tick se llega siempre al mismo estado. Por eso la partida no se guarda
/// copiando sus ~200 campos internos, sino como lo que la define por completo (ADR-0016):
///
///   - el inicio: semilla, tamaño del mapa, número de infectados y parámetros de IA, y
///   - las entradas de la jugadora en cada tick (comprimidas en tramos de entradas iguales),
///     junto con el algoritmo (F2) y el modo de la horda (F4) que estaban activos.
///
/// Cargar reconstruye la partida y repite las entradas; el estado del generador aleatorio, las
/// memorias de los infectados, las rutas, etc. quedan exactamente como estaban. Para comprobarlo
/// el archivo lleva la huella del estado al guardar, y la carga la compara (CP-091).
///
/// Formato binario (todo en little-endian):
///
///   "REFG" | versión u16 | reservado u16 | longitud u32 | CRC-32 u32 | carga (longitud bytes)
///
/// La carga es: versión del juego, inicio, parámetros de IA, ticks, huella y tramos. Cualquier
/// byte alterado lo detecta el CRC (CP-121), y los rangos se validan aunque el CRC sea correcto.

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "dominio/config/Configuracion.hpp"
#include "dominio/juego/Partida.hpp"

namespace refugio::persistencia {

/// 1: hasta la 0.12.0. 2: añade los pesos de ia.json (HU-21); el 1 se sigue leyendo.
inline constexpr std::uint16_t kVersionFormato = 2;
inline constexpr std::size_t kTamCabecera = 16;
/// RNF-021: no se analiza ningún archivo de más de 10 MB.
inline constexpr std::size_t kMaxBytesArchivo = 10u * 1024u * 1024u;
/// Seis horas de juego a 60 Hz: ninguna partida real llega; un archivo con más se rechaza.
inline constexpr std::uint32_t kMaxTicks = 6u * 3600u * 60u;

/// Todo lo necesario para crear la partida desde cero.
struct InicioPartida {
    std::uint64_t semilla{42};
    int ancho{32};
    int alto{22};
    int infectados{10};
    config::ParametrosIa ia{};
    int flowfieldUmbral{5};  ///< El prototipo usa 5 con 10 infectados (ver main_juego.cpp).
    navegacion::Algoritmo algoritmo{navegacion::Algoritmo::AEstrella};
    bool coordinado{true};
    friend bool operator==(const InicioPartida&, const InicioPartida&) = default;
};

/// ConfigPartida por defecto con los parámetros de IA, el umbral, el algoritmo y el modo del inicio.
[[nodiscard]] juego::ConfigPartida configDe(const InicioPartida& inicio);
/// Crea la partida del inicio (lanza std::exception si no se puede generar el mapa).
[[nodiscard]] std::unique_ptr<juego::Partida> crearPartida(const InicioPartida& inicio);

/// Entrada de un tick tal como la recibe Partida::paso, más los dos interruptores que cambian la
/// simulación entre pasos (F2 y F4).
struct EntradaTick {
    juego::EntradaJugador entrada{};
    navegacion::Algoritmo algoritmo{navegacion::Algoritmo::AEstrella};
    bool coordinado{true};
};
[[nodiscard]] bool iguales(const EntradaTick& a, const EntradaTick& b) noexcept;

/// @p repeticiones ticks seguidos con la misma entrada.
struct Tramo {
    EntradaTick tick{};
    std::uint32_t repeticiones{0};
};

/// Huella del estado de la partida (FNV-1a sobre Mara, infectados, objetos, herramientas,
/// refugio, extracción y superviviente). Dos partidas en el mismo estado dan la misma huella.
[[nodiscard]] std::uint64_t huellaPartida(const juego::Partida& p) noexcept;

/// Graba las entradas de la partida en curso. Va junto a la Partida: cada paso de simulación
/// pasa primero por registrar().
class GrabadorPartida {
public:
    GrabadorPartida() = default;
    explicit GrabadorPartida(const InicioPartida& inicio);

    /// Anota la entrada del tick que va a correr @p p y devuelve la que hay que pasar a
    /// p.paso(): igual a @p e, salvo que «apuntar» se pone a 0 cuando no se lanza señuelo ni se
    /// coloca barricada (solo se usa entonces); así el ratón no rompe los tramos.
    juego::EntradaJugador registrar(const juego::EntradaJugador& e, const juego::Partida& p);

    [[nodiscard]] const InicioPartida& inicio() const noexcept { return inicio_; }
    [[nodiscard]] const std::vector<Tramo>& tramos() const noexcept { return tramos_; }
    [[nodiscard]] std::uint64_t ticks() const noexcept { return ticks_; }

    /// Para restaurar: añade un tramo ya grabado.
    void anadir(const Tramo& t);

private:
    InicioPartida inicio_{};
    std::vector<Tramo> tramos_;
    std::uint64_t ticks_{0};
};

/// Lo que va en el archivo.
struct PartidaGuardada {
    std::string versionJuego;  ///< REFUGIO_VERSION del juego que la guardó.
    InicioPartida inicio{};
    std::vector<Tramo> tramos;
    std::uint32_t ticks{0};
    std::uint64_t huella{0};
};

/// Datos para guardar la partida en curso.
[[nodiscard]] PartidaGuardada instantanea(const GrabadorPartida& g, const juego::Partida& p);

/// Archivo completo (cabecera + carga) listo para escribir.
[[nodiscard]] std::vector<std::uint8_t> codificar(const PartidaGuardada& g);

enum class ErrorCarga : std::uint8_t {
    Ninguno,
    Vacia,          ///< La ranura no tiene archivo.
    Lectura,        ///< El archivo existe pero no se pudo leer.
    Grande,         ///< Más de 10 MB (RNF-021).
    Cabecera,       ///< Corto o sin «REFG».
    Version,        ///< Formato más nuevo que el que conoce el juego.
    Longitud,       ///< La longitud de la cabecera no coincide con el archivo.
    Crc,            ///< CRC-32 distinto: algún byte cambió.
    Formato,        ///< Campos truncados, fuera de rango o sobrantes.
    Huella,         ///< Al repetir las entradas no se llega al estado guardado.
    OtraVersionJuego,  ///< Huella distinta y guardada con otra versión del juego.
};

/// Mensaje para la jugadora. Todos los errores de integridad dicen «La partida guardada está
/// dañada» (HU-19); el detalle técnico va en ResultadoCarga::detalle.
[[nodiscard]] std::string mensajeError(ErrorCarga e, int ranura = 1);

struct ResultadoLecturaPartida {
    ErrorCarga error{ErrorCarga::Ninguno};
    std::string detalle;
    PartidaGuardada datos;
    [[nodiscard]] bool valida() const noexcept { return error == ErrorCarga::Ninguno; }
};

/// Valida cabecera, versión, longitud, CRC y rangos. Nunca lanza.
[[nodiscard]] ResultadoLecturaPartida decodificar(std::span<const std::uint8_t> bytes);

/// Partida reconstruida, con el grabador listo para seguir grabando donde se quedó.
struct PartidaRestaurada {
    ErrorCarga error{ErrorCarga::Ninguno};
    std::string detalle;
    std::unique_ptr<juego::Partida> partida;
    GrabadorPartida grabador;
    [[nodiscard]] bool valida() const noexcept { return error == ErrorCarga::Ninguno && partida != nullptr; }
};

/// Crea la partida, repite las entradas y compara la huella. Nunca lanza.
[[nodiscard]] PartidaRestaurada restaurar(const PartidaGuardada& g);

/// decodificar() + restaurar().
[[nodiscard]] PartidaRestaurada cargar(std::span<const std::uint8_t> bytes);

}  // namespace refugio::persistencia
