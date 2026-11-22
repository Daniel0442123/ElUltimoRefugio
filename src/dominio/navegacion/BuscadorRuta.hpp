#pragma once
/// @file BuscadorRuta.hpp
/// @brief Dijkstra y A* sobre el mismo grafo y las mismas estructuras (RF-021 a RF-024, ADR-0006).
///
/// Dijkstra es A* con h(n) = 0. Ambos comparten EspacioBusqueda, MonticuloBinario y GrafoRejilla,
/// así la comparación mide solo el efecto de la heurística.
///
/// Heurística de A*: distancia octil. Es admisible (h <= h*) porque es el costo exacto con m = 1
/// y sin obstáculos, y los obstáculos y m >= 1 solo aumentan el costo real. Es consistente
/// (h(n) <= w(n, n') + h(n')) por la desigualdad triangular de la métrica octil y w >= d.
/// Con h consistente un nodo cerrado ya tiene g óptimo, por eso no se reabren nodos.
///
/// Complejidad: O((V + E) log V) en tiempo y O(V) en espacio, con E <= 8V.

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "dominio/mundo/Grafo.hpp"

namespace refugio::navegacion {

using mundo::GrafoRejilla;
using mundo::Nodo;

enum class Algoritmo : std::uint8_t { Dijkstra, AEstrella };

[[nodiscard]] std::string_view nombreAlgoritmo(Algoritmo a) noexcept;

/// Métricas que exige el panel (GUIA §6) y el benchmark (GUIA §7). RF-023.
struct ResultadoBusqueda {
    Algoritmo algoritmo{Algoritmo::AEstrella};
    bool encontrada{false};
    std::vector<Nodo> ruta;            ///< De origen a destino, ambos incluidos.
    double costo{0.0};
    std::uint32_t expandidos{0};       ///< Nodos extraídos de la abierta y cerrados.
    std::uint32_t generados{0};        ///< Inserciones en la abierta.
    std::uint32_t maxAbierta{0};       ///< Tamaño máximo de la lista abierta.
    double tiempoUs{0.0};
    std::size_t memoriaBytes{0};       ///< Memoria de las estructuras de búsqueda.
    std::vector<Nodo> ordenExpansion;  ///< Solo si se pide (superposición de depuración, RF-071).
};

/// Interfaz Strategy para los algoritmos de búsqueda (RF-024).
class IBuscadorRuta {
public:
    virtual ~IBuscadorRuta() = default;
    virtual ResultadoBusqueda buscar(const GrafoRejilla& grafo, Nodo origen, Nodo destino,
                                     bool registrarExpansion = false) = 0;
    [[nodiscard]] virtual Algoritmo algoritmo() const noexcept = 0;
};

/// Montículo binario mínimo con desempate determinista (f menor; con igual f, g mayor).
/// Usa eliminación perezosa: un nodo puede estar varias veces y las copias viejas se descartan.
class MonticuloBinario {
public:
    struct Entrada {
        double f;
        double g;
        Nodo nodo;
    };
    void limpiar() noexcept { datos_.clear(); }
    void reservar(std::size_t n) { datos_.reserve(n); }
    [[nodiscard]] bool vacio() const noexcept { return datos_.empty(); }
    [[nodiscard]] std::size_t tamano() const noexcept { return datos_.size(); }
    [[nodiscard]] std::size_t capacidad() const noexcept { return datos_.capacity(); }
    void insertar(Entrada e);
    Entrada extraer();

private:
    static bool menor(const Entrada& a, const Entrada& b) noexcept {
        return a.f < b.f || (a.f == b.f && (a.g > b.g || (a.g == b.g && a.nodo < b.nodo)));
    }
    std::vector<Entrada> datos_;
};

/// Implementación común: Dijkstra si usarHeuristica == false, A* octil si true.
class BuscadorMejorPrimero final : public IBuscadorRuta {
public:
    explicit BuscadorMejorPrimero(Algoritmo algoritmo) : algoritmo_(algoritmo) {}
    ResultadoBusqueda buscar(const GrafoRejilla& grafo, Nodo origen, Nodo destino,
                             bool registrarExpansion = false) override;
    [[nodiscard]] Algoritmo algoritmo() const noexcept override { return algoritmo_; }

private:
    void prepararEspacio(std::uint32_t numNodos);

    Algoritmo algoritmo_;
    // Arreglos indexados por nodo. Se reutilizan entre búsquedas: un valor solo es válido si
    // marca[n] == generacion_, así reiniciar es O(1) en vez de O(V) (arc42 §8.5).
    std::vector<double> g_;
    std::vector<Nodo> padre_;
    std::vector<std::uint32_t> marca_;
    std::vector<std::uint32_t> cerrado_;
    std::uint32_t generacion_{0};
    MonticuloBinario abierta_;
};

/// Fábrica de buscadores (patrón Factory). RNF-026: un algoritmo nuevo se registra aquí.
[[nodiscard]] std::unique_ptr<IBuscadorRuta> crearBuscador(Algoritmo a);

}  // namespace refugio::navegacion
