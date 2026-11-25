#include "dominio/juego/Tutorial.hpp"

#include <algorithm>
#include <cmath>

#include "dominio/juego/Partida.hpp"

namespace refugio::juego {

namespace {
int objetosEnInventario(const Partida& p) noexcept {
    int n = 0;
    for (const bool b : p.mara().inventario) n += b ? 1 : 0;
    return n;
}
}  // namespace

std::string_view nombrePaso(PasoTutorial p) noexcept {
    switch (p) {
        case PasoTutorial::Mover: return "mover";
        case PasoTutorial::Sigilo: return "sigilo";
        case PasoTutorial::Correr: return "correr";
        case PasoTutorial::Recoger: return "recoger";
        case PasoTutorial::Terminado: return "terminado";
    }
    return "?";
}

Tutorial::Tutorial(bool activo) noexcept : paso_(activo ? PasoTutorial::Mover : PasoTutorial::Terminado) {}

void Tutorial::observar(const Partida& p) noexcept {
    const Vec2 pos = p.mara().pos;
    if (!iniciado_) {
        iniciado_ = true;
        posX_ = pos.x;
        posY_ = pos.y;
        objetosInicio_ = objetosEnInventario(p);
        tInicio_ = p.tiempo() - static_cast<double>(p.dt());  // el paso que acaba de correr cuenta
    }
    if (!activo()) return;
    segundos_ = p.tiempo() - tInicio_;
    const float d = std::hypot(pos.x - posX_, pos.y - posY_);
    posX_ = pos.x;
    posY_ = pos.y;
    const ModoMovimiento modo = p.mara().modo;
    switch (paso_) {
        case PasoTutorial::Mover:
            recorrido_ += d;
            if (recorrido_ >= kDistanciaMover) avanzar();
            break;
        case PasoTutorial::Sigilo:
            if (modo == ModoMovimiento::Sigilo) recorrido_ += d;
            if (recorrido_ >= kDistanciaSigilo) avanzar();
            break;
        case PasoTutorial::Correr:
            if (modo == ModoMovimiento::Correr) recorrido_ += d;
            if (recorrido_ >= kDistanciaCorrer) {
                avanzar();
                objetosInicio_ = objetosEnInventario(p);  // solo cuenta lo que recoja desde ahora
            }
            break;
        case PasoTutorial::Recoger:
            if (objetosEnInventario(p) > objetosInicio_) avanzar();
            break;
        case PasoTutorial::Terminado: break;
    }
}

void Tutorial::avanzar() noexcept {
    ++completados_;
    recorrido_ = 0.0f;
    paso_ = static_cast<PasoTutorial>(static_cast<int>(paso_) + 1);
}

void Tutorial::saltar() noexcept {
    if (!activo()) return;
    saltado_ = true;
    paso_ = PasoTutorial::Terminado;
}

float Tutorial::progreso() const noexcept {
    switch (paso_) {
        case PasoTutorial::Mover: return std::min(1.0f, recorrido_ / kDistanciaMover);
        case PasoTutorial::Sigilo: return std::min(1.0f, recorrido_ / kDistanciaSigilo);
        case PasoTutorial::Correr: return std::min(1.0f, recorrido_ / kDistanciaCorrer);
        case PasoTutorial::Recoger:
        case PasoTutorial::Terminado: return 0.0f;
    }
    return 0.0f;
}

}  // namespace refugio::juego
