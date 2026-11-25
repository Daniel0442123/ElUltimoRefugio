#pragma once
/// @file Configuracion.hpp
/// @brief Configuración local en JSON (HU-19, RF-092, RF-093, ADR-0011).
///
/// Tres archivos en config/: juego.json (volumen y tutorial), controles.json (una tecla por
/// acción) e ia.json (parámetros de la simulación). Aquí solo se analizan y se escriben textos:
/// leer y guardar los archivos es cosa del adaptador (adaptadores/archivo/ArchivosLocales).
///
/// Un archivo inválido nunca detiene el juego (RF-093, RNF-016): si el JSON está mal formado se
/// usan todos los valores por defecto; si un campo tiene un tipo o un rango incorrecto, solo ese
/// campo vuelve a su valor por defecto. Cada problema queda en Lectura::errores con el archivo y
/// el campo, para mostrarlo en pantalla y en la consola.

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace refugio::juego {
struct ConfigPartida;
}

namespace refugio::config {

/// Resultado de analizar un archivo: el valor (con los valores por defecto donde hizo falta) y
/// un mensaje por cada problema encontrado.
template <class T>
struct Lectura {
    T valor{};
    std::vector<std::string> errores;
    [[nodiscard]] bool valida() const noexcept { return errores.empty(); }
};

// ----------------------------------------------------------------------------- juego.json
/// Opciones del menú (config/juego.json).
struct OpcionesJuego {
    float volumen{0.8f};  ///< 0..1, volumen general.
    bool tutorial{true};  ///< RF-017: mostrar el tutorial en una partida nueva.
};

[[nodiscard]] Lectura<OpcionesJuego> leerOpciones(std::string_view texto, std::string_view archivo = "config/juego.json");
[[nodiscard]] std::string escribirOpciones(const OpcionesJuego& o);

// ------------------------------------------------------------------------- controles.json
/// Acciones de juego reasignables (RNF-012). El menú (Esc), Enter y las flechas están reservados.
enum class Accion : std::uint8_t {
    Arriba,
    Abajo,
    Izquierda,
    Derecha,
    Correr,
    Sigilo,
    Interactuar,
    Senuelo,
    Barricada,
    GuardadoRapido,
    CargaRapida,
    SaltarTutorial,
    Depuracion,
    Algoritmo,
    Comparar,
    ModoHorda,
    Nodos,
    Consola,
    NuevaPartida,
    Laboratorio,
};
inline constexpr std::size_t kNumAcciones = 20;

/// Clave en controles.json («arriba», «guardado_rapido», …).
[[nodiscard]] std::string_view claveAccion(Accion a) noexcept;
/// Texto para el menú de opciones («Mover arriba», «Guardado rápido (ranura 1)», …).
[[nodiscard]] std::string_view textoAccion(Accion a) noexcept;

/// Códigos de tecla: los mismos números que KEY_* de raylib (el adaptador lo comprueba con
/// static_assert), para que el dominio no dependa de raylib (RNF-023).
namespace tecla {
inline constexpr int kNinguna = 0;
inline constexpr int kEspacio = 32;
inline constexpr int kEscape = 256;
inline constexpr int kEnter = 257;
inline constexpr int kTab = 258;
inline constexpr int kRetroceso = 259;
inline constexpr int kDerecha = 262;
inline constexpr int kIzquierda = 263;
inline constexpr int kAbajo = 264;
inline constexpr int kArriba = 265;
inline constexpr int kF1 = 290;  ///< F1..F12 son 290..301.
inline constexpr int kShiftIzq = 340;
inline constexpr int kCtrlIzq = 341;
inline constexpr int kAltIzq = 342;
inline constexpr int kShiftDer = 344;
inline constexpr int kCtrlDer = 345;
inline constexpr int kAltDer = 346;
}  // namespace tecla

/// Nombre de una tecla asignable («W», «F5», «LEFT_SHIFT», «SPACE»…) o "" si no lo es.
[[nodiscard]] std::string nombreTecla(int codigo);
/// Texto corto para mostrar en pantalla («W», «F5», «Shift», «Ctrl der.», «Espacio»…).
[[nodiscard]] std::string textoTecla(int codigo);
/// Código de una tecla por su nombre (sin distinguir mayúsculas) o tecla::kNinguna.
[[nodiscard]] int teclaDesdeNombre(std::string_view nombre);
/// Esc, Enter y las flechas manejan el menú y no se pueden asignar.
[[nodiscard]] bool teclaReservada(int codigo) noexcept;
/// Tiene nombre y no está reservada.
[[nodiscard]] bool teclaAsignable(int codigo);

/// Una tecla por acción, sin repetir.
class Controles {
public:
    Controles();  ///< Los valores por defecto (WASD, Shift, Ctrl, E, Q, B, F5, F9, T, F1-F4, N, L, R, Tab).
    [[nodiscard]] int tecla(Accion a) const noexcept { return teclas_[static_cast<std::size_t>(a)]; }
    /// Asigna @p codigo a @p a. Si otra acción ya la usaba, intercambian sus teclas para que no
    /// queden repetidas. Devuelve false (sin cambiar nada) si la tecla no es asignable.
    bool asignar(Accion a, int codigo);
    /// Acción que usa @p codigo, o -1.
    [[nodiscard]] int accionDe(int codigo) const noexcept;
    friend bool operator==(const Controles&, const Controles&) = default;

private:
    std::array<int, kNumAcciones> teclas_{};
};

[[nodiscard]] Lectura<Controles> leerControles(std::string_view texto, std::string_view archivo = "config/controles.json");
[[nodiscard]] std::string escribirControles(const Controles& c);

// ------------------------------------------------------------------------------- ia.json
/// Parámetros de config/ia.json que usa la partida. Los «prototipos» de infectado aún no se leen:
/// la partida usa su perseguidor ajustado (ConfigPartida). Los valores por defecto son los de
/// ConfigPartida, así que aplicar los parámetros por defecto no cambia nada.
struct ParametrosIa {
    int tickSimHz{60};
    int tickIaHz{10};
    float velSigilo{1.5f}, velCaminar{3.0f}, velCorrer{6.0f};
    float ruidoSigilo{1.0f}, ruidoCaminar{3.0f}, ruidoCorrer{6.0f};
    float tInstalacion{8.0f};
    float tExtraccion{60.0f};
    float saludMax{100.0f};
    float integridadMax{100.0f};
    float senueloAlcance{8.0f}, senueloRadio{8.0f}, senueloDuracion{10.0f}, senueloPeriodo{1.0f};
    float semividaMemoria{10.0f};
    float radioComunicacion{5.0f};
    int profundidadMax{3};
    float atenuacion{0.7f};
    int maxAgentesPorCelda{2};
    int flowfieldUmbral{8};
    int presupuestoNodosTick{4000};
    float factorDesvioBarricada{2.0f};
    friend bool operator==(const ParametrosIa&, const ParametrosIa&) = default;
};

/// "" si todos los valores están en rango; si no, qué campo falla. Lo usan la lectura del JSON y
/// la carga de una partida guardada (que lleva sus propios parámetros).
[[nodiscard]] std::string validar(const ParametrosIa& p);
[[nodiscard]] Lectura<ParametrosIa> leerParametrosIa(std::string_view texto, std::string_view archivo = "config/ia.json");
/// Copia los parámetros en @p cfg (el resto de ConfigPartida no cambia).
void aplicar(const ParametrosIa& p, juego::ConfigPartida& cfg);

}  // namespace refugio::config
