#include "dominio/agentes/Memoria.hpp"

#include <cmath>

namespace refugio::agentes {

std::string_view nombreEvidencia(TipoEvidencia t) noexcept {
    switch (t) {
        case TipoEvidencia::Avistamiento: return "avistamiento";
        case TipoEvidencia::Ruido: return "ruido";
        case TipoEvidencia::Alerta: return "alerta";
        case TipoEvidencia::Senuelo: return "senuelo";
    }
    return "?";
}

float confianzaActual(const Evidencia& e, double ahora, float semivida) noexcept {
    const double edad = ahora > e.tiempo ? ahora - e.tiempo : 0.0;
    return static_cast<float>(static_cast<double>(e.confianza) * std::exp2(-edad / static_cast<double>(semivida)));
}

float utilidad(const Evidencia& e, double ahora, float distancia, float semivida) noexcept {
    return confianzaActual(e, ahora, semivida) / (1.0f + distancia / 10.0f);
}

bool MemoriaEvidencias::insertar(const Evidencia& e, double ahora, float semivida) noexcept {
    // La misma evidencia (tipo y celda) se refresca en vez de ocupar otro hueco.
    for (std::size_t i = 0; i < n_; ++i) {
        auto& d = datos_[i];
        if (d.tipo != e.tipo || !(d.celda == e.celda)) continue;
        if (confianzaActual(e, ahora, semivida) >= confianzaActual(d, ahora, semivida)) d = e;
        return true;
    }
    if (n_ < datos_.size()) {
        datos_[n_++] = e;
        return true;
    }
    std::size_t peor = 0;
    float minimo = confianzaActual(datos_[0], ahora, semivida);
    for (std::size_t i = 1; i < n_; ++i) {
        const float c = confianzaActual(datos_[i], ahora, semivida);
        if (c < minimo) {
            minimo = c;
            peor = i;
        }
    }
    if (confianzaActual(e, ahora, semivida) <= minimo) return false;
    datos_[peor] = e;
    return true;
}

}  // namespace refugio::agentes
