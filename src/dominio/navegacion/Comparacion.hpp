#pragma once
/// @file Comparacion.hpp
/// @brief Misma consulta con Dijkstra y con A* sobre el mismo grafo (RF-074, RF-082, HU-16).
///
/// Lo usan el panel del juego (F3), el laboratorio (Tab) y el benchmark, así los tres comparan
/// con el mismo criterio: costo igual con tolerancia 1e-6 y nodos expandidos de A* <= Dijkstra.

#include "dominio/navegacion/BuscadorRuta.hpp"

namespace refugio::navegacion {

/// Diferencia máxima admitida entre el costo de A* y el de Dijkstra (RF-082).
inline constexpr double kToleranciaCosto = 1e-6;

/// Resultado de ejecutar una consulta con los dos algoritmos.
struct ComparacionBusqueda {
    bool valida{false};  ///< Hay una consulta comparada.
    Nodo origen{0};
    Nodo destino{0};
    ResultadoBusqueda dijkstra;
    ResultadoBusqueda aEstrella;

    /// |costo A* - costo Dijkstra|; 0 si ninguno encontró ruta.
    [[nodiscard]] double diferenciaCosto() const noexcept;
    /// Ambos encuentran ruta (o ninguno) y con el mismo costo (RF-082).
    [[nodiscard]] bool mismoCosto() const noexcept;
    /// A* no expande más nodos que Dijkstra (heurística consistente).
    [[nodiscard]] bool aEstrellaNoExpandeMas() const noexcept { return aEstrella.expandidos <= dijkstra.expandidos; }
    /// Fracción de nodos que A* se ahorra respecto a Dijkstra, en [0, 1]; 0 si Dijkstra no expandió.
    [[nodiscard]] double ahorroExpandidos() const noexcept;
    /// Cociente de tiempos Dijkstra / A*; 0 si A* tardó 0 us.
    [[nodiscard]] double aceleracion() const noexcept;
};

/// Ejecuta la consulta (@p origen, @p destino) con @p dijkstra y con @p aEstrella en ese orden y
/// escribe ambos resultados en @p salida reutilizando su memoria. No modifica el grafo.
void compararAlgoritmos(const GrafoRejilla& grafo, Nodo origen, Nodo destino, IBuscadorRuta& dijkstra,
                        IBuscadorRuta& aEstrella, ComparacionBusqueda& salida);

/// Comodidad: crea los buscadores y devuelve la comparación.
[[nodiscard]] ComparacionBusqueda compararAlgoritmos(const GrafoRejilla& grafo, Nodo origen, Nodo destino);

}  // namespace refugio::navegacion
