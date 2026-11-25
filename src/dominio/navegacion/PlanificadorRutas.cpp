#include "dominio/navegacion/PlanificadorRutas.hpp"

namespace refugio::navegacion {

void PlanificadorRutas::reservar(std::uint32_t numAgentes, std::uint32_t numNodos) {
    cola_.reserve(numAgentes);
    enCola_.assign(numAgentes, std::uint8_t{0});
    resultado_.ruta.reserve(numNodos);
    resultado_.ordenExpansion.reserve(numNodos);  // RF-071: expansión del agente seleccionado
    buscador_.reservar(numNodos);
}

void PlanificadorRutas::fijarAlgoritmo(Algoritmo a) noexcept {
    if (a == buscador_.algoritmo()) return;
    buscador_.fijarAlgoritmo(a);
    if (enCurso_) reiniciar_ = true;
}

void PlanificadorRutas::solicitar(std::uint32_t agente, std::uint8_t prioridad) {
    if (agente >= enCola_.size()) enCola_.resize(agente + 1u, std::uint8_t{0});
    if (enCola_[agente] != 0) {
        for (auto& s : cola_)
            if (s.agente == agente) s.prioridad = prioridad;
        return;  // ya espera (o está en curso): conserva su antigüedad
    }
    enCola_[agente] = 1;
    cola_.push_back({agente, prioridad, tick_});
}

void PlanificadorRutas::cancelar(std::uint32_t agente) noexcept {
    if (!solicitado(agente)) return;
    enCola_[agente] = 0;
    if (enCurso_ && actual_.agente == agente) {
        enCurso_ = false;
        reiniciar_ = false;
        buscador_.abandonar();
        return;
    }
    for (std::size_t i = 0; i < cola_.size(); ++i) {
        if (cola_[i].agente == agente) {
            quitarDeCola(i);
            return;
        }
    }
}

void PlanificadorRutas::reiniciarEnCurso() noexcept {
    if (enCurso_) reiniciar_ = true;
}

void PlanificadorRutas::vaciar() noexcept {
    for (const auto& s : cola_) enCola_[s.agente] = 0;
    cola_.clear();
    if (enCurso_) enCola_[actual_.agente] = 0;
    enCurso_ = false;
    reiniciar_ = false;
    buscador_.abandonar();
}

std::size_t PlanificadorRutas::siguiente() const noexcept {
    // Prioridad efectiva en unidades de tick: prioridad · kTicksEnvejecimiento + espera. Así no
    // hay divisiones y una solicitud sube un nivel cada kTicksEnvejecimiento ticks de espera.
    const auto clave = [this](const Solicitud& s) {
        return static_cast<std::uint64_t>(s.prioridad) * kTicksEnvejecimiento + (tick_ - s.tick);
    };
    std::size_t mejor = 0;
    for (std::size_t i = 1; i < cola_.size(); ++i) {
        const Solicitud& a = cola_[i];
        const Solicitud& b = cola_[mejor];
        const auto ka = clave(a);
        const auto kb = clave(b);
        if (ka > kb || (ka == kb && (a.tick < b.tick || (a.tick == b.tick && a.agente < b.agente)))) mejor = i;
    }
    return mejor;
}

void PlanificadorRutas::quitarDeCola(std::size_t i) noexcept {
    // Se conserva el orden de llegada (no se intercambia con el último): no cambia el resultado,
    // porque siguiente() desempata por tick y agente, pero así la cola se lee mejor al depurar.
    cola_.erase(cola_.begin() + static_cast<std::ptrdiff_t>(i));
}

}  // namespace refugio::navegacion
