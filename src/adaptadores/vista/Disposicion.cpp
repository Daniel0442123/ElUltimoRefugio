#include "adaptadores/vista/Disposicion.hpp"

#include <algorithm>

namespace refugio::vista {

float escalaInterfaz(int anchoPx, int altoPx) noexcept {
    const float s = std::min(static_cast<float>(std::max(anchoPx, 1)) / kAnchoReferencia,
                             static_cast<float>(std::max(altoPx, 1)) / kAltoReferencia);
    return std::clamp(s, kEscalaMinima, kEscalaMaxima);
}

Disposicion calcularDisposicion(int anchoPx, int altoPx, float anchoMundo, float altoMundo) noexcept {
    Disposicion d;
    d.escala = escalaInterfaz(anchoPx, altoPx);
    d.ancho = static_cast<float>(std::max(anchoPx, 1)) / d.escala;
    d.alto = static_cast<float>(std::max(altoPx, 1)) / d.escala;
    d.panelX = std::max(0.0f, d.ancho - kAnchoPanel);
    d.mapa = {0.0f, 0.0f, d.panelX, std::max(0.0f, d.alto - kAltoFranja)};
    d.mapaPx = {d.mapa.x * d.escala, d.mapa.y * d.escala, d.mapa.ancho * d.escala, d.mapa.alto * d.escala};
    if (anchoMundo > 0.0f && altoMundo > 0.0f)
        d.zoomMapa = std::max(0.01f, std::min(d.mapaPx.ancho / anchoMundo, d.mapaPx.alto / altoMundo));
    const float w = anchoMundo * d.zoomMapa;
    const float h = altoMundo * d.zoomMapa;
    d.mundoPx = {d.mapaPx.x + (d.mapaPx.ancho - w) / 2.0f, d.mapaPx.y + (d.mapaPx.alto - h) / 2.0f, w, h};
    return d;
}

Punto encuadrar(Punto objetivo, float zoom, float vistaAncho, float vistaAlto, float anchoMundo, float altoMundo) noexcept {
    const auto eje = [zoom](float o, float vista, float mundo) {
        const float medio = vista / (2.0f * zoom);  // media vista en píxeles de mundo
        if (2.0f * medio >= mundo) return mundo / 2.0f;
        return std::clamp(o, medio, mundo - medio);
    };
    return {eje(objetivo.x, vistaAncho, anchoMundo), eje(objetivo.y, vistaAlto, altoMundo)};
}

}  // namespace refugio::vista
