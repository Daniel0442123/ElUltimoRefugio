#include "dominio/experimentos/EstresRecursion.hpp"

#include <algorithm>
#include <chrono>

#include "dominio/coordinacion/PropagadorAlertas.hpp"
#include "dominio/nucleo/Pcg32.hpp"

namespace refugio::experimentos {

using navegacion::Quadtree;
using navegacion::Vec2;

ResumenEstres ejecutarEstresRecursion(const ConfigEstres& cfg, std::ostream& csv) {
    ResumenEstres resumen;
    csv << "# semilla=" << cfg.semilla << " radio_com=" << cfg.radioComunicacion << " lado=" << cfg.lado << '\n';
    csv << "agentes,profundidad_max,repeticion,profundidad_alcanzada,llamadas,mensajes,alertados,tiempo_us,"
           "pila_bytes\n";

    for (const int n : cfg.agentes) {
        nucleo::Pcg32 rng(cfg.semilla + static_cast<std::uint64_t>(n), 2u);
        std::vector<Vec2> pos(static_cast<std::size_t>(n));
        Quadtree qt({0.0f, 0.0f, cfg.lado, cfg.lado});
        for (std::size_t i = 0; i < pos.size(); ++i) {
            pos[i] = {static_cast<float>(rng.real01()) * cfg.lado, static_cast<float>(rng.real01()) * cfg.lado};
            qt.insertar(static_cast<std::uint32_t>(i), pos[i]);
        }
        for (const int prof : cfg.profundidades) {
            coordinacion::PropagadorAlertas prop({cfg.radioComunicacion, prof, 0.7f});
            for (int rep = 0; rep < cfg.repeticiones; ++rep) {
                const auto origen = rng.enteroMenorQue(static_cast<std::uint32_t>(n));
                const auto t0 = std::chrono::steady_clock::now();
                const auto r = prop.propagar(pos, qt, origen, 1.0f);
                const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();

                std::vector<int> recibidos(pos.size(), 0);
                for (const auto& m : r.mensajes) ++recibidos[m.receptor];
                if (std::any_of(recibidos.begin(), recibidos.end(), [](int c) { return c > 1; }))
                    ++resumen.violacionesUnicidad;
                if (r.profundidadAlcanzada > prof) ++resumen.violacionesLimite;
                const auto alertados = std::count_if(r.saltoPorAgente.begin(), r.saltoPorAgente.end(),
                                                     [](int s) { return s >= 0; });
                resumen.pilaMaximaBytes = std::max(resumen.pilaMaximaBytes, r.bytesPilaEstimados);
                csv << n << ',' << prof << ',' << rep << ',' << r.profundidadAlcanzada << ',' << r.llamadas << ','
                    << r.mensajes.size() << ',' << alertados << ',' << us << ',' << r.bytesPilaEstimados << '\n';
                ++resumen.filas;
            }
        }
    }

    // Peor caso del quadtree: 10 000 puntos en la misma posición. La profundidad debe quedarse en el límite.
    Quadtree qt({0.0f, 0.0f, 64.0f, 64.0f});
    for (std::uint32_t i = 0; i < 10000; ++i) qt.insertar(i, {10.0f, 10.0f});
    resumen.profundidadQuadtreeCoincidentes = qt.profundidadMaxima();
    if (resumen.profundidadQuadtreeCoincidentes > Quadtree::kProfundidadMax) ++resumen.violacionesLimite;
    return resumen;
}

}  // namespace refugio::experimentos
