#include "dominio/coordinacion/AsignadorAccesos.hpp"

#include <algorithm>

#include "dominio/coordinacion/ContractNet.hpp"

namespace refugio::coordinacion {

void AsignadorAccesos::reservar(std::size_t maxAgentes) {
    accesos_.reserve(8);
    ocupacion_.reserve(8);
    pares_.reserve(maxAgentes * 8);
}

void AsignadorAccesos::asignar(const mundo::GrafoRejilla& g, mundo::Celda objetivo,
                               const std::vector<std::uint32_t>& ids, const std::vector<mundo::Celda>& celdas,
                               int maxPorCelda, std::vector<Asignacion>& salida) {
    accesos_.clear();
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if ((dx != 0 || dy != 0) && g.transitable(objetivo.x + dx, objetivo.y + dy))
                accesos_.push_back({objetivo.x + dx, objetivo.y + dy});
    ocupacion_.assign(accesos_.size(), 0);

    salida.clear();
    for (const auto id : ids) salida.push_back({id, objetivo, false});

    pares_.clear();
    for (std::uint32_t a = 0; a < ids.size(); ++a)
        for (std::uint32_t s = 0; s < accesos_.size(); ++s)
            pares_.push_back({costoPuja(celdas[a], accesos_[s]), a, s});
    // Orden total (costo, agente, acceso): el resultado es determinista (RNF-020).
    std::sort(pares_.begin(), pares_.end(), [](const Par& x, const Par& y) {
        if (x.costo != y.costo) return x.costo < y.costo;
        if (x.agente != y.agente) return x.agente < y.agente;
        return x.acceso < y.acceso;
    });
    for (const auto& p : pares_) {
        auto& s = salida[p.agente];
        if (s.asignado || ocupacion_[p.acceso] >= maxPorCelda) continue;
        s.acceso = accesos_[p.acceso];
        s.asignado = true;
        ++ocupacion_[p.acceso];
    }
}

}  // namespace refugio::coordinacion
