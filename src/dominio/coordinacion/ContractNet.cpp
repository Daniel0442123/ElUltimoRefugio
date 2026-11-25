#include "dominio/coordinacion/ContractNet.hpp"

#include <algorithm>
#include <cstdlib>

namespace refugio::coordinacion {

std::string_view nombreTarea(TipoTarea t) noexcept {
    switch (t) {
        case TipoTarea::Perseguir: return "Perseguir";
        case TipoTarea::Interceptar: return "Interceptar";
        case TipoTarea::Flanquear: return "Flanquear";
    }
    return "?";
}

double costoPuja(mundo::Celda desde, mundo::Celda hacia) noexcept {
    const double dx = std::abs(desde.x - hacia.x);
    const double dy = std::abs(desde.y - hacia.y);
    return (dx + dy) + (mundo::kRaiz2 - 2.0) * std::min(dx, dy);
}

void ejecutarContractNet(const std::vector<Tarea>& tareas, const std::vector<Postor>& postores,
                         ResultadoContractNet& salida) {
    salida.adjudicaciones.clear();
    salida.anuncios = static_cast<std::uint32_t>(tareas.size() * postores.size());
    salida.pujas = salida.anuncios;  // cada postor puja por cada tarea anunciada
    for (std::uint32_t t = 0; t < tareas.size(); ++t) {
        // La puja más baja entre los postores libres; empate: el agente de menor id (determinista).
        std::size_t mejor = postores.size();
        double costoMejor = 0.0;
        for (std::size_t k = 0; k < postores.size(); ++k) {
            const auto& p = postores[k];
            const bool ocupado = std::any_of(salida.adjudicaciones.begin(), salida.adjudicaciones.end(),
                                             [&](const Adjudicacion& a) { return a.agente == p.agente; });
            if (ocupado) continue;
            const double c = costoPuja(p.celda, tareas[t].destino);
            if (mejor == postores.size() || c < costoMejor ||
                (c == costoMejor && p.agente < postores[mejor].agente)) {
                mejor = k;
                costoMejor = c;
            }
        }
        if (mejor == postores.size()) break;  // no quedan postores libres
        salida.adjudicaciones.push_back({postores[mejor].agente, t, costoMejor});
    }
}

}  // namespace refugio::coordinacion
