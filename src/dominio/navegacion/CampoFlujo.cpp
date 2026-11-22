#include "dominio/navegacion/CampoFlujo.hpp"

#include <algorithm>
#include <chrono>

namespace refugio::navegacion {

void CampoFlujo::reservar(std::uint32_t numNodos) {
    costo_.assign(numNodos, kInfinito);
    siguiente_.assign(numNodos, 0);
    cerrado_.assign(numNodos, 0);
    abierta_.reservar(8u * static_cast<std::size_t>(numNodos) + 1u);  // una entrada por arista relajada
}

bool CampoFlujo::calcular(const mundo::GrafoRejilla& g, Nodo destino) {
    using Reloj = std::chrono::steady_clock;
    const auto inicio = Reloj::now();
    if (costo_.size() != g.numCeldas()) reservar(g.numCeldas());  // solo si cambia el mapa

    std::fill(costo_.begin(), costo_.end(), kInfinito);
    std::fill(cerrado_.begin(), cerrado_.end(), std::uint8_t{0});
    for (Nodo n = 0; n < g.numCeldas(); ++n) siguiente_[n] = n;
    abierta_.limpiar();
    expandidos_ = 0;
    destino_ = destino;
    valido_ = destino < g.numCeldas() && g.transitable(destino);

    if (valido_) {
        costo_[destino] = 0.0;
        abierta_.insertar({0.0, 0.0f, destino});
        while (!abierta_.vacio()) {
            const Nodo u = abierta_.extraer().nodo;
            if (cerrado_[u] != 0) continue;  // copia vieja (eliminación perezosa)
            cerrado_[u] = 1;
            ++expandidos_;
            const double cu = costo_[u];
            const double mu = static_cast<double>(g.multiplicador(u));
            const mundo::Celda cu2 = g.celda(u);
            // paraCadaVecino(u) da w(u, v) = d · m(v); aquí hace falta w(v, u) = d · m(u), con
            // d = 1 o sqrt(2) según la dirección (se calcula igual que en el grafo, sin dividir).
            g.paraCadaVecino(u, [&](Nodo v, double) {
                if (cerrado_[v] != 0) return;
                const mundo::Celda cv2 = g.celda(v);
                const double d = (cv2.x != cu2.x && cv2.y != cu2.y) ? mundo::kRaiz2 : 1.0;
                const double cv = cu + d * mu;
                if (cv < costo_[v]) {
                    costo_[v] = cv;
                    siguiente_[v] = u;
                    abierta_.insertar({cv, 0.0f, v});
                }
            });
        }
    }
    tiempoUs_ = std::chrono::duration<double, std::micro>(Reloj::now() - inicio).count();
    return valido_;
}

}  // namespace refugio::navegacion
