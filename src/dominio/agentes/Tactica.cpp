#include "dominio/agentes/Tactica.hpp"

#include <cmath>

#include "dominio/agentes/Percepcion.hpp"

namespace refugio::agentes {

ResultadoTactica elegirTactica(const EntradaTactica& e, const ParametrosTactica& p) noexcept {
    ResultadoTactica r;
    auto& u = r.puntuacion;
    u[static_cast<std::size_t>(Tactica::Explorar)] = p.utilidadExplorar;
    if (e.hayEvidencia) {
        float d = e.distancia;
        if (e.tipo == TipoEvidencia::Avistamiento)
            d += (e.tieneRumbo ? e.velocidadMara : p.velocidadSupuesta) * e.edad;
        u[static_cast<std::size_t>(Tactica::Perseguir)] = e.confianza / (1.0f + d / 10.0f);
        if (e.tieneRumbo && e.hayCorte)
            u[static_cast<std::size_t>(Tactica::Interceptar)] = p.pesoInterceptar * e.confianza / (1.0f + e.distanciaCorte / 10.0f);
    }
    r.tactica = Tactica::Perseguir;
    for (const Tactica t : {Tactica::Interceptar, Tactica::Explorar})
        if (u[static_cast<std::size_t>(t)] > u[static_cast<std::size_t>(r.tactica)]) r.tactica = t;
    return r;
}

std::optional<PuntoCorte> puntoDeCorte(const mundo::GrafoRejilla& g, Vec2 posMara, Vec2 velMara, Vec2 posAgente,
                                       float velAgente, const ParametrosTactica& p) noexcept {
    const float v = std::sqrt(velMara.x * velMara.x + velMara.y * velMara.y);
    if (v < p.velocidadMinimaRumbo || velAgente <= 0.0f) return std::nullopt;
    std::optional<PuntoCorte> mejor;
    constexpr float kPaso = 0.25f;
    for (float t = kPaso; t <= p.horizonteMax + 1e-4f; t += kPaso) {
        const Vec2 q{posMara.x + velMara.x * t, posMara.y + velMara.y * t};
        const mundo::Celda c = celdaDe(q);
        if (!g.transitable(c.x, c.y)) break;  // la ruta prevista termina en la pared
        if (t < p.horizonteMin - 1e-4f) continue;
        const Vec2 centro = centroDe(c);
        const float d = std::hypot(centro.x - posAgente.x, centro.y - posAgente.y);
        if (d / velAgente > t) continue;  // Mara llegaría antes
        if (!mejor || d < mejor->distancia) mejor = PuntoCorte{c, t, d};
    }
    return mejor;
}

}  // namespace refugio::agentes
