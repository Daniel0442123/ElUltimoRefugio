#include "dominio/navegacion/BuscadorRuta.hpp"

#include <algorithm>
#include <chrono>
#include <limits>

namespace refugio::navegacion {

std::string_view nombreAlgoritmo(Algoritmo a) noexcept {
    return a == Algoritmo::Dijkstra ? "Dijkstra" : "AEstrella";
}

// ----------------------------------------------------------------- MonticuloBinario
void MonticuloBinario::insertar(Entrada e) {
    datos_.push_back(e);
    std::size_t i = datos_.size() - 1;
    while (i > 0) {
        const std::size_t p = (i - 1) / 2;
        if (!menor(datos_[i], datos_[p])) break;
        std::swap(datos_[i], datos_[p]);
        i = p;
    }
}

MonticuloBinario::Entrada MonticuloBinario::extraer() {
    const Entrada cima = datos_.front();
    datos_.front() = datos_.back();
    datos_.pop_back();
    std::size_t i = 0;
    const std::size_t n = datos_.size();
    for (;;) {
        const std::size_t izq = 2 * i + 1;
        const std::size_t der = izq + 1;
        std::size_t m = i;
        if (izq < n && menor(datos_[izq], datos_[m])) m = izq;
        if (der < n && menor(datos_[der], datos_[m])) m = der;
        if (m == i) break;
        std::swap(datos_[i], datos_[m]);
        i = m;
    }
    return cima;
}

// ------------------------------------------------------------- BuscadorMejorPrimero
void BuscadorMejorPrimero::prepararEspacio(std::uint32_t numNodos) {
    if (g_.size() != numNodos) {
        g_.assign(numNodos, 0.0);
        padre_.assign(numNodos, 0);
        marca_.assign(numNodos, 0);
        cerrado_.assign(numNodos, 0);
        generacion_ = 0;
    }
    ++generacion_;
    if (generacion_ == 0) {  // desbordamiento tras 2^32 búsquedas: reinicio completo
        std::fill(marca_.begin(), marca_.end(), 0u);
        std::fill(cerrado_.begin(), cerrado_.end(), 0u);
        generacion_ = 1;
    }
    abierta_.limpiar();
}

ResultadoBusqueda BuscadorMejorPrimero::buscar(const GrafoRejilla& grafo, Nodo origen, Nodo destino,
                                               bool registrarExpansion) {
    using Reloj = std::chrono::steady_clock;
    const auto inicio = Reloj::now();

    ResultadoBusqueda r;
    r.algoritmo = algoritmo_;
    prepararEspacio(grafo.numCeldas());

    const bool usarH = algoritmo_ == Algoritmo::AEstrella;
    auto h = [&](Nodo n) { return usarH ? grafo.distanciaOctil(n, destino) : 0.0; };

    if (grafo.transitable(origen) && grafo.transitable(destino)) {
        g_[origen] = 0.0;
        padre_[origen] = origen;
        marca_[origen] = generacion_;
        abierta_.insertar({h(origen), 0.0, origen});
        r.generados = 1;
        r.maxAbierta = 1;

        while (!abierta_.vacio()) {
            const auto actual = abierta_.extraer();
            const Nodo u = actual.nodo;
            if (cerrado_[u] == generacion_) continue;  // copia vieja (eliminación perezosa)
            cerrado_[u] = generacion_;
            ++r.expandidos;
            if (registrarExpansion) r.ordenExpansion.push_back(u);
            if (u == destino) {
                r.encontrada = true;
                break;
            }
            const double gu = g_[u];
            grafo.paraCadaVecino(u, [&](Nodo v, double w) {
                if (cerrado_[v] == generacion_) return;  // sin reapertura: h consistente
                const double gv = gu + w;
                if (marca_[v] != generacion_ || gv < g_[v]) {
                    marca_[v] = generacion_;
                    g_[v] = gv;
                    padre_[v] = u;
                    abierta_.insertar({gv + h(v), gv, v});
                    ++r.generados;
                }
            });
            r.maxAbierta = std::max(r.maxAbierta, static_cast<std::uint32_t>(abierta_.tamano()));
        }
    }

    if (r.encontrada) {
        r.costo = g_[destino];
        for (Nodo n = destino;; n = padre_[n]) {
            r.ruta.push_back(n);
            if (n == origen) break;
        }
        std::reverse(r.ruta.begin(), r.ruta.end());
    }

    r.memoriaBytes = g_.capacity() * sizeof(double) + padre_.capacity() * sizeof(Nodo) +
                     (marca_.capacity() + cerrado_.capacity()) * sizeof(std::uint32_t) +
                     static_cast<std::size_t>(r.maxAbierta) * sizeof(MonticuloBinario::Entrada);
    r.tiempoUs = std::chrono::duration<double, std::micro>(Reloj::now() - inicio).count();
    return r;
}

std::unique_ptr<IBuscadorRuta> crearBuscador(Algoritmo a) { return std::make_unique<BuscadorMejorPrimero>(a); }

}  // namespace refugio::navegacion
