#include "dominio/navegacion/Comparacion.hpp"

#include <cmath>

namespace refugio::navegacion {

double ComparacionBusqueda::diferenciaCosto() const noexcept {
    if (!dijkstra.encontrada || !aEstrella.encontrada) return 0.0;
    return std::abs(aEstrella.costo - dijkstra.costo);
}

bool ComparacionBusqueda::mismoCosto() const noexcept {
    if (dijkstra.encontrada != aEstrella.encontrada) return false;
    return diferenciaCosto() <= kToleranciaCosto;
}

double ComparacionBusqueda::ahorroExpandidos() const noexcept {
    if (dijkstra.expandidos == 0 || aEstrella.expandidos >= dijkstra.expandidos) return 0.0;
    return 1.0 - static_cast<double>(aEstrella.expandidos) / static_cast<double>(dijkstra.expandidos);
}

double ComparacionBusqueda::aceleracion() const noexcept {
    return aEstrella.tiempoUs > 0.0 ? dijkstra.tiempoUs / aEstrella.tiempoUs : 0.0;
}

void compararAlgoritmos(const GrafoRejilla& grafo, Nodo origen, Nodo destino, IBuscadorRuta& dijkstra,
                        IBuscadorRuta& aEstrella, ComparacionBusqueda& salida) {
    salida.origen = origen;
    salida.destino = destino;
    dijkstra.buscarEn(grafo, origen, destino, salida.dijkstra);
    aEstrella.buscarEn(grafo, origen, destino, salida.aEstrella);
    salida.valida = true;
}

ComparacionBusqueda compararAlgoritmos(const GrafoRejilla& grafo, Nodo origen, Nodo destino) {
    BuscadorMejorPrimero d(Algoritmo::Dijkstra);
    BuscadorMejorPrimero a(Algoritmo::AEstrella);
    ComparacionBusqueda c;
    compararAlgoritmos(grafo, origen, destino, d, a, c);
    return c;
}

}  // namespace refugio::navegacion
