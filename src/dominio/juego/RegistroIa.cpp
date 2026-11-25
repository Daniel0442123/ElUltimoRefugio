#include "dominio/juego/RegistroIa.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>

namespace refugio::juego {

std::string_view nombreEventoIa(TipoEventoIa t) noexcept {
    switch (t) {
        case TipoEventoIa::Transicion: return "transicion";
        case TipoEventoIa::Plan: return "plan";
        case TipoEventoIa::Mensaje: return "mensaje";
        case TipoEventoIa::Superviviente: return "superviviente";
    }
    return "?";
}

void RegistroIa::anotar(std::uint64_t tick, TipoEventoIa tipo, std::uint16_t agente, const char* formato, ...) noexcept {
    auto& l = lineas_[static_cast<std::size_t>(total_ % kCapacidad)];
    l.tick = tick;
    l.tipo = tipo;
    l.agente = agente;
    std::va_list args;
    va_start(args, formato);
    const int n = std::vsnprintf(l.texto.data(), l.texto.size(), formato, args);
    va_end(args);
    l.largo = static_cast<std::uint16_t>(n < 0 ? 0 : std::min<std::size_t>(static_cast<std::size_t>(n), l.texto.size() - 1));
    ++total_;
    ++porTipo_[static_cast<std::size_t>(tipo)];
    if (sumidero_ != nullptr) sumidero_->escribir(tick, nombreEventoIa(tipo), l.vista());
}

std::size_t RegistroIa::tamano() const noexcept {
    return static_cast<std::size_t>(std::min<std::uint64_t>(total_, kCapacidad));
}

const LineaIa& RegistroIa::ultima(std::size_t haceN) const noexcept {
    return lineas_[static_cast<std::size_t>((total_ - 1 - haceN) % kCapacidad)];
}

}  // namespace refugio::juego
