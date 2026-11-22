#include "dominio/experimentos/Benchmark.hpp"

#include <cmath>
#include <iomanip>

#include "dominio/experimentos/GeneradorMapas.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"

namespace refugio::experimentos {

using navegacion::Algoritmo;

ResumenBenchmark ejecutarBenchmark(const ConfigBenchmark& cfg, const std::vector<std::string>& metadatos,
                                   std::ostream& csv) {
    ResumenBenchmark resumen;
    csv << std::setprecision(10);
    for (const auto& linea : metadatos) csv << "# " << linea << '\n';
    csv << "# semilla_maestra=" << cfg.semilla << " pares=" << cfg.pares << " repeticiones=" << cfg.repeticiones
        << " densidad=" << cfg.densidad << '\n';
    csv << "algoritmo,ancho,alto,densidad_obstaculos,semilla,par,repeticion,origen_x,origen_y,destino_x,destino_y,"
           "encontrada,costo,longitud,nodos_expandidos,nodos_generados,max_abierta,tiempo_us,memoria_pico_bytes\n";

    auto dijkstra = navegacion::crearBuscador(Algoritmo::Dijkstra);
    auto aestrella = navegacion::crearBuscador(Algoritmo::AEstrella);

    for (const int tam : cfg.tamanos) {
        // Semilla derivada por tamaño: cada mapa se puede regenerar de forma independiente.
        const std::uint64_t semillaMapa = cfg.semilla * 1000003ULL + static_cast<std::uint64_t>(tam);
        nucleo::Pcg32 rng(semillaMapa, 1u);
        const auto grafo = generarMapa({tam, tam, cfg.densidad, 0.10}, rng);
        const auto pares = generarPares(grafo, cfg.pares, rng);

        for (std::size_t i = 0; i < pares.size(); ++i) {
            const auto [o, d] = pares[i];
            for (int rep = 0; rep < cfg.repeticiones; ++rep) {
                // Se alterna el orden para no favorecer a ninguno por efectos de caché.
                navegacion::IBuscadorRuta* orden[2] = {dijkstra.get(), aestrella.get()};
                if (rep % 2 == 1) std::swap(orden[0], orden[1]);
                navegacion::ResultadoBusqueda res[2];
                for (int k = 0; k < 2; ++k) res[k] = orden[k]->buscar(grafo, o, d);
                const auto& rd = res[orden[0] == dijkstra.get() ? 0 : 1];
                const auto& ra = res[orden[0] == dijkstra.get() ? 1 : 0];

                if (rd.encontrada != ra.encontrada || (rd.encontrada && std::abs(rd.costo - ra.costo) > 1e-6))
                    ++resumen.discrepanciasOptimalidad;
                if (!rd.encontrada) ++resumen.consultasSinRuta;

                for (const auto* r : {&rd, &ra}) {
                    const auto co = grafo.celda(o);
                    const auto cd = grafo.celda(d);
                    csv << navegacion::nombreAlgoritmo(r->algoritmo) << ',' << tam << ',' << tam << ','
                        << cfg.densidad << ',' << semillaMapa << ',' << i << ',' << rep << ',' << co.x << ','
                        << co.y << ',' << cd.x << ',' << cd.y << ',' << (r->encontrada ? 1 : 0) << ','
                        << r->costo << ',' << r->ruta.size() << ',' << r->expandidos << ',' << r->generados << ','
                        << r->maxAbierta << ',' << r->tiempoUs << ',' << r->memoriaBytes << '\n';
                    ++resumen.mediciones;
                }
            }
        }
    }
    return resumen;
}

}  // namespace refugio::experimentos
