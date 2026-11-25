#include "dominio/navegacion/Steering.hpp"

#include <algorithm>
#include <cmath>

namespace refugio::navegacion {

Vec2 recortar(Vec2 v, float maximo) noexcept {
    const float l2 = v.x * v.x + v.y * v.y;
    if (l2 <= maximo * maximo || l2 <= 0.0f) return v;
    const float k = maximo / std::sqrt(l2);
    return {v.x * k, v.y * k};
}

Vec2 fuerzaSteering(std::uint32_t yo, const Movil* todos, const std::vector<std::uint32_t>& vecinos, Vec2 velDeseada,
                    const mundo::GrafoRejilla& g, float radioCuerpo, const ParametrosSteering& p) noexcept {
    const Movil& m = todos[yo];
    // Seguir: diferencia entre la velocidad deseada y la actual.
    Vec2 f{p.pesoSeguir * (velDeseada.x - m.vel.x), p.pesoSeguir * (velDeseada.y - m.vel.y)};

    Vec2 sep{};
    Vec2 anticipa{};
    Vec2 velMedia{};
    Vec2 centro{};
    int n = 0;
    for (const auto j : vecinos) {
        if (j == yo) continue;
        const Vec2 d{m.pos.x - todos[j].pos.x, m.pos.y - todos[j].pos.y};
        const float l = std::sqrt(d.x * d.x + d.y * d.y);
        if (l > p.radioVecindad) continue;
        if (l < p.radioSeparacion) {
            // Empuje que crece al acercarse; si coinciden, una dirección fija según el índice.
            const Vec2 u = l > 1e-4f ? Vec2{d.x / l, d.y / l} : Vec2{yo < j ? 1.0f : -1.0f, 0.0f};
            const float k = (p.radioSeparacion - l) / p.radioSeparacion;
            sep.x += u.x * k;
            sep.y += u.y * k;
        }
        // Anticipación (máximo acercamiento con velocidades constantes).
        const Vec2 dp{-d.x, -d.y};  // del yo al vecino
        const Vec2 dv{todos[j].vel.x - m.vel.x, todos[j].vel.y - m.vel.y};
        const float dv2 = dv.x * dv.x + dv.y * dv.y;
        if (dv2 > 1e-6f) {
            const float t = -(dp.x * dv.x + dp.y * dv.y) / dv2;
            if (t > 0.0f && t < p.horizonteAnticipacion) {
                const Vec2 s{dp.x + dv.x * t, dp.y + dv.y * t};  // vecino respecto al yo en ese instante
                const float ls = std::sqrt(s.x * s.x + s.y * s.y);
                if (ls < p.distanciaSegura) {
                    // De frente exacto: cada uno a su derecha respecto a la velocidad relativa, así
                    // los dos se apartan en sentidos opuestos sin necesitar acuerdo.
                    const float inv = 1.0f / std::sqrt(dv2);
                    const Vec2 u = ls > 1e-3f ? Vec2{-s.x / ls, -s.y / ls} : Vec2{-dv.y * inv, dv.x * inv};
                    const float k = p.pesoAnticipacion * (1.0f - t / p.horizonteAnticipacion) *
                                    (p.distanciaSegura - ls) / p.distanciaSegura;
                    anticipa.x += u.x * k;
                    anticipa.y += u.y * k;
                }
            }
        }
        velMedia.x += todos[j].vel.x;
        velMedia.y += todos[j].vel.y;
        centro.x += todos[j].pos.x;
        centro.y += todos[j].pos.y;
        ++n;
    }
    f.x += p.pesoSeparacion * sep.x + anticipa.x;
    f.y += p.pesoSeparacion * sep.y + anticipa.y;
    if (n > 0) {
        const float inv = 1.0f / static_cast<float>(n);
        f.x += p.pesoAlineacion * (velMedia.x * inv - m.vel.x) + p.pesoCohesion * (centro.x * inv - m.pos.x);
        f.y += p.pesoAlineacion * (velMedia.y * inv - m.vel.y) + p.pesoCohesion * (centro.y * inv - m.pos.y);
    }

    // Evitación: empuje desde el punto más cercano de cada celda bloqueada de la vecindad 3x3.
    const int cx = static_cast<int>(std::floor(m.pos.x));
    const int cy = static_cast<int>(std::floor(m.pos.y));
    const float margen = radioCuerpo + 0.35f;
    for (int y = cy - 1; y <= cy + 1; ++y) {
        for (int x = cx - 1; x <= cx + 1; ++x) {
            if (g.transitable(x, y)) continue;
            const float qx = std::clamp(m.pos.x, static_cast<float>(x), static_cast<float>(x + 1));
            const float qy = std::clamp(m.pos.y, static_cast<float>(y), static_cast<float>(y + 1));
            const Vec2 d{m.pos.x - qx, m.pos.y - qy};
            const float l = std::sqrt(d.x * d.x + d.y * d.y);
            if (l >= margen || l < 1e-5f) continue;
            const float k = (margen - l) / margen;
            f.x += p.pesoEvitacion * d.x / l * k;
            f.y += p.pesoEvitacion * d.y / l * k;
        }
    }
    return recortar(f, p.aceleracionMax);
}

Vec2 integrarVelocidad(Vec2 vel, Vec2 fuerza, float velMax, float dt) noexcept {
    return recortar({vel.x + fuerza.x * dt, vel.y + fuerza.y * dt}, velMax);
}

}  // namespace refugio::navegacion
