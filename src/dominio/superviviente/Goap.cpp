#include "dominio/superviviente/Goap.hpp"

#include <algorithm>
#include <bit>
#include <limits>

namespace refugio::superviviente {

PlanGoap PlanificadorGoap::planificar(Hechos estado, Hechos meta, std::span<const AccionGoap> acciones,
                                      std::size_t profundidadMax) noexcept {
    PlanGoap plan;
    profundidadMax = std::min(profundidadMax, kProfundidadMaxGoap);
    float costoMin = std::numeric_limits<float>::max();
    int maxAnade = 1;
    for (const auto& a : acciones) {
        if (!a.habilitada) continue;
        costoMin = std::min(costoMin, a.costo);
        maxAnade = std::max(maxAnade, std::popcount(a.anade));
    }
    if (costoMin == std::numeric_limits<float>::max()) costoMin = 0.0f;
    auto h = [&](Hechos exigidos) {
        return static_cast<float>(std::popcount(exigidos & ~estado)) * costoMin / static_cast<float>(maxAnade);
    };

    std::size_t usados = 1;
    nodos_[0] = Nodo{meta, 0.0f, h(meta), -1, 0, 0, false};
    while (true) {
        // Nodo abierto de menor f (pocos nodos: búsqueda lineal, sin montículo ni asignaciones).
        std::size_t mejor = usados;
        for (std::size_t i = 0; i < usados; ++i)
            if (!nodos_[i].cerrado && (mejor == usados || nodos_[i].f < nodos_[mejor].f)) mejor = i;
        if (mejor == usados) return plan;  // sin plan
        Nodo& n = nodos_[mejor];
        n.cerrado = true;
        ++plan.expandidos;
        if ((n.exigidos & ~estado) == 0) {
            // El nodo final es el primer paso: subir hasta la raíz da el orden de ejecución.
            plan.encontrado = true;
            plan.costo = n.g;
            for (std::int16_t k = static_cast<std::int16_t>(mejor); nodos_[static_cast<std::size_t>(k)].padre >= 0;
                 k = nodos_[static_cast<std::size_t>(k)].padre)
                plan.acciones[plan.longitud++] = nodos_[static_cast<std::size_t>(k)].accion;
            return plan;
        }
        if (n.profundidad >= profundidadMax) continue;
        for (std::size_t a = 0; a < acciones.size(); ++a) {
            const auto& ac = acciones[a];
            // Relevante si añade algo exigido (aunque ya sea cierto ahora: otra acción anterior del
            // plan podría borrarlo, como IrAlmacen borra EnPuerta) y no borra nada exigido.
            if (!ac.habilitada || (ac.anade & n.exigidos) == 0 || (ac.borra & n.exigidos) != 0) continue;
            const Hechos exigidos = (n.exigidos & ~ac.anade) | ac.pre;
            const float g = n.g + ac.costo;
            // Un nodo con los mismos hechos exigidos y costo no peor ya cubre este.
            bool repetido = false;
            for (std::size_t i = 0; i < usados && !repetido; ++i)
                repetido = nodos_[i].exigidos == exigidos && nodos_[i].g <= g;
            if (repetido || usados >= kMaxNodosGoap) continue;
            nodos_[usados++] = Nodo{exigidos, g, g + h(exigidos), static_cast<std::int16_t>(mejor), static_cast<std::uint8_t>(a),
                                    static_cast<std::uint8_t>(n.profundidad + 1), false};
        }
    }
}

}  // namespace refugio::superviviente
