#include "dominio/agentes/Percepcion.hpp"

#include <cmath>
#include <cstdlib>

namespace refugio::agentes {

int paredesEntre(const mundo::GrafoRejilla& g, mundo::Celda a, mundo::Celda b) noexcept {
    int x = a.x;
    int y = a.y;
    const int dx = std::abs(b.x - a.x);
    const int dy = -std::abs(b.y - a.y);
    const int sx = a.x < b.x ? 1 : -1;
    const int sy = a.y < b.y ? 1 : -1;
    int err = dx + dy;
    int paredes = 0;
    for (;;) {
        if (x == b.x && y == b.y) break;
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y += sy;
        }
        if (x == b.x && y == b.y) break;
        if (!g.transitable(x, y)) ++paredes;
    }
    return paredes;
}

bool dentroDelCono(Vec2 origen, Vec2 orientacion, Vec2 objetivo, float radio, float aperturaGrados) noexcept {
    const float dx = objetivo.x - origen.x;
    const float dy = objetivo.y - origen.y;
    const float d2 = dx * dx + dy * dy;
    if (d2 > radio * radio) return false;
    if (d2 < 1e-6f) return true;
    const float d = std::sqrt(d2);
    const float cosAngulo = (dx * orientacion.x + dy * orientacion.y) / d;
    const float cosMitad = std::cos(aperturaGrados * 0.5f * 3.14159265f / 180.0f);
    return cosAngulo >= cosMitad;
}

bool ve(const mundo::GrafoRejilla& g, Vec2 origen, Vec2 orientacion, Vec2 objetivo, float radio,
        float aperturaGrados) noexcept {
    return dentroDelCono(origen, orientacion, objetivo, radio, aperturaGrados) &&
           lineaDeVisionLibre(g, celdaDe(origen), celdaDe(objetivo));
}

bool oye(const mundo::GrafoRejilla& g, Vec2 oyente, Vec2 origenRuido, float radio) noexcept {
    const float dx = oyente.x - origenRuido.x;
    const float dy = oyente.y - origenRuido.y;
    const float efectivo = radio - 2.0f * static_cast<float>(paredesEntre(g, celdaDe(origenRuido), celdaDe(oyente)));
    return efectivo > 0.0f && dx * dx + dy * dy <= efectivo * efectivo;
}

}  // namespace refugio::agentes
