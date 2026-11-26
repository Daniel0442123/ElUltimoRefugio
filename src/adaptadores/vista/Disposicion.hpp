#pragma once
/// @file Disposicion.hpp
/// @brief Reparto de la ventana entre el mapa, el panel lateral y la franja de estado (sin raylib).
///
/// La interfaz se diseña en «unidades» sobre un lienzo de referencia de 1280 x 720. Cada frame se
/// calcula cuántos píxeles de la ventana vale una unidad (la escala) a partir del tamaño real de
/// la ventana, así que el HUD crece con la pantalla (1,5 en 1920 x 1080, 3 en 3840 x 2160) y los
/// textos se dibujan con fuentes rasterizadas a ese tamaño en píxeles, sin perder nitidez.
///
/// El lienzo lógico conserva al menos 1280 x 720 unidades: si la ventana es más ancha o más alta
/// que 16:9, sobran unidades y se las queda el mapa; el panel conserva su ancho y su alto crece.
/// El mapa se dibuja aparte, en píxeles, con el zoom que lo hace caber entero en su zona.

namespace refugio::vista {

struct Rect {
    float x{0};
    float y{0};
    float ancho{0};
    float alto{0};
};

inline constexpr float kAnchoReferencia = 1280.0f;
inline constexpr float kAltoReferencia = 720.0f;
inline constexpr float kAnchoPanel = 256.0f;   ///< Unidades, a la derecha del mapa.
inline constexpr float kAltoFranja = 16.0f;    ///< Unidades, bajo el mapa (semilla, tick, velocidad).
inline constexpr float kEscalaMinima = 0.5f;   ///< Por debajo los textos dejarían de leerse.
inline constexpr float kEscalaMaxima = 6.0f;

struct Disposicion {
    float escala{1.0f};  ///< Píxeles de la ventana por unidad de interfaz.
    float ancho{kAnchoReferencia};  ///< Ancho de la ventana en unidades.
    float alto{kAltoReferencia};    ///< Alto de la ventana en unidades.
    Rect mapa;       ///< Zona del mapa en unidades (arriba a la izquierda).
    Rect mapaPx;     ///< La misma zona en píxeles de la ventana.
    float panelX{0};  ///< Borde izquierdo del panel lateral, en unidades.
    float zoomMapa{1.0f};  ///< Píxeles de ventana por píxel de mundo para ver el mapa entero.
    Rect mundoPx;    ///< Dónde queda el mapa entero con zoomMapa, centrado en mapaPx.

    /// Convierte un punto en píxeles de la ventana a unidades de interfaz.
    [[nodiscard]] float aUnidades(float px) const noexcept { return px / escala; }
};

/// Escala de la interfaz para una ventana de @p anchoPx x @p altoPx píxeles: la mayor que deja
/// al menos 1280 x 720 unidades, limitada a [kEscalaMinima, kEscalaMaxima].
[[nodiscard]] float escalaInterfaz(int anchoPx, int altoPx) noexcept;

/// Disposición completa para una ventana de @p anchoPx x @p altoPx píxeles y un mundo de
/// @p anchoMundo x @p altoMundo píxeles de mundo (el mapa a 32 px por celda).
[[nodiscard]] Disposicion calcularDisposicion(int anchoPx, int altoPx, float anchoMundo, float altoMundo) noexcept;

/// Punto de mira de una cámara que sigue a @p objetivo (en píxeles de mundo) con @p zoom sin
/// enseñar nada fuera del mundo; si el mundo es más pequeño que la vista en un eje, se centra.
/// @p vistaAncho y @p vistaAlto son el tamaño de la vista en píxeles de ventana.
struct Punto {
    float x{0};
    float y{0};
};
[[nodiscard]] Punto encuadrar(Punto objetivo, float zoom, float vistaAncho, float vistaAlto, float anchoMundo,
                              float altoMundo) noexcept;

}  // namespace refugio::vista
