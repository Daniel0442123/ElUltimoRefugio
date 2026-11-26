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
/// Acciones de juego reasignables (RNF-012). El menú (Esc), Enter, las flechas y F11 (pantalla
/// completa) están reservados.
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
    PausaSimulacion,  ///< HU-22, RF-075
    PasoSimulacion,
    MasLento,
    MasRapido,
    Camara,  ///< Cámara que sigue a Mara con zoom o mapa completo
};
inline constexpr std::size_t kNumAcciones = 25;

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
/// F11 alterna pantalla completa en toda la aplicación, como en la mayoría de programas.
inline constexpr int kPantallaCompleta = kF1 + 10;
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
    Controles();  ///< Los valores por defecto (WASD, Shift, Ctrl, E, Q, B, F5, F9, T, F1-F4, N, L, R, Tab, P, F6-F8).
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
/// Sección «pesos» de config/ia.json (HU-21, RF-085): los pesos de steering y de utilidad de las
/// tácticas y los parámetros de dificultad del perseguidor que ajusta la optimización offline.
/// Los valores por defecto son los de ConfigPartida.
struct PesosIa {
    float steeringSeguir{8.0f};
    float steeringSeparacion{14.0f};
    float steeringAlineacion{0.4f};
    float steeringCohesion{0.2f};
    float steeringAnticipacion{30.0f};
    float utilidadExplorar{0.15f};
    float pesoInterceptar{0.8f};
    float histeresisTactica{1.1f};
    float visionRadio{6.0f};
    float velPerseguir{3.4f};
    float danioPorSegundo{10.0f};
    friend bool operator==(const PesosIa&, const PesosIa&) = default;
};

/// Un campo de «pesos»: sección y clave en ia.json, rango válido y miembro de PesosIa.
struct CampoPesos {
    std::string_view seccion;  ///< «steering», «tactica» o «infectados».
    std::string_view clave;
    float minimo;
    float maximo;
    float PesosIa::*campo;
};
inline constexpr std::size_t kNumPesos = 11;
/// Los 11 campos, en el orden en que se guardan en la partida y se optimizan.
[[nodiscard]] const std::array<CampoPesos, kNumPesos>& camposPesos() noexcept;

/// Parámetros de config/ia.json que usa la partida. Los «prototipos» de infectado aún no se leen:
/// la partida usa su perseguidor ajustado (ConfigPartida) con la sección «pesos». Los valores por
/// defecto son los de ConfigPartida, así que aplicar los parámetros por defecto no cambia nada.
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
    PesosIa pesos{};
    friend bool operator==(const ParametrosIa&, const ParametrosIa&) = default;
};

/// "" si todos los valores están en rango; si no, qué campo falla. Lo usan la lectura del JSON y
/// la carga de una partida guardada (que lleva sus propios parámetros).
[[nodiscard]] std::string validar(const ParametrosIa& p);
[[nodiscard]] Lectura<ParametrosIa> leerParametrosIa(std::string_view texto, std::string_view archivo = "config/ia.json");
/// Copia los parámetros en @p cfg (el resto de ConfigPartida no cambia).
void aplicar(const ParametrosIa& p, juego::ConfigPartida& cfg);
/// Copia solo los pesos en @p cfg.
void aplicar(const PesosIa& p, juego::ConfigPartida& cfg);
/// Sección «pesos» en el formato de config/ia.json, sangrada con @p sangria espacios.
[[nodiscard]] std::string escribirPesos(const PesosIa& p, int sangria = 2);

}  // namespace refugio::config
