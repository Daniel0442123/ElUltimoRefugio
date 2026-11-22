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
///
/// Rendimiento (RNF-003, RNF-006): el bucle interno no llama a GrafoRejilla::paraCadaVecino sino
/// que lee los multiplicadores por desplazamiento de índice, calcula h sin divisiones a partir de
/// las coordenadas del vecino y guarda g, padre y estado de cada nodo en un solo registro de
/// 16 bytes (una línea de caché por nodo). buscarEn() reutiliza la memoria de un resultado previo:
/// tras la primera búsqueda en un mapa no hay asignaciones del montículo.

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
    /// Busca y escribe en @p salida reutilizando su memoria (sin asignar si ya tiene capacidad).
    virtual void buscarEn(const GrafoRejilla& grafo, Nodo origen, Nodo destino, ResultadoBusqueda& salida,
                          bool registrarExpansion = false) = 0;
    /// Comodidad para pruebas y herramientas: asigna un resultado nuevo.
    ResultadoBusqueda buscar(const GrafoRejilla& grafo, Nodo origen, Nodo destino, bool registrarExpansion = false) {
        ResultadoBusqueda r;
        buscarEn(grafo, origen, destino, r, registrarExpansion);
        return r;
    }
    [[nodiscard]] virtual Algoritmo algoritmo() const noexcept = 0;
};

/// Montículo binario mínimo con desempate determinista (f menor; con igual f, h menor, que es lo
/// mismo que g mayor porque f = g + h; con igual h, nodo menor).
/// Usa eliminación perezosa: un nodo puede estar varias veces y las copias viejas se descartan.
class MonticuloBinario {
public:
    /// 16 bytes: cuatro entradas por línea de caché de 64 bytes. h solo sirve para desempatar,
    /// así que basta con float; f se guarda en double para no alterar el orden de expansión.
    struct Entrada {
        double f;
        float h;
        Nodo nodo;
    };
    static_assert(sizeof(Entrada) == 16);
    void limpiar() noexcept { datos_.clear(); }
    void reservar(std::size_t n) { datos_.reserve(n); }
    [[nodiscard]] bool vacio() const noexcept { return datos_.empty(); }
    [[nodiscard]] std::size_t tamano() const noexcept { return datos_.size(); }
    [[nodiscard]] std::size_t capacidad() const noexcept { return datos_.capacity(); }
    void insertar(Entrada e);
    Entrada extraer();

private:
    static bool menor(const Entrada& a, const Entrada& b) noexcept {
        // Operadores & y | sobre bool en lugar de && y ||: se evalúa todo sin saltos.
        const int r = (a.f < b.f) | ((a.f == b.f) & ((a.h < b.h) | ((a.h == b.h) & (a.nodo < b.nodo))));
        return r != 0;
    }
    std::vector<Entrada> datos_;
};

/// Implementación común: Dijkstra si usarHeuristica == false, A* octil si true.
class BuscadorMejorPrimero final : public IBuscadorRuta {
public:
    explicit BuscadorMejorPrimero(Algoritmo algoritmo) : algoritmo_(algoritmo) {}
    using IBuscadorRuta::buscar;
    void buscarEn(const GrafoRejilla& grafo, Nodo origen, Nodo destino, ResultadoBusqueda& salida,
                  bool registrarExpansion = false) override;
    [[nodiscard]] Algoritmo algoritmo() const noexcept override { return algoritmo_; }

    /// Reserva toda la memoria que puede necesitar una búsqueda en un mapa de @p numNodos celdas:
    /// la abierta nunca supera 8·V + 1 entradas (una por arista relajada más el origen). Tras
    /// llamarla, buscarEn() no asigna memoria en ese mapa (RNF-006).
    void reservar(std::uint32_t numNodos);

private:
    /// Estado de un nodo en la búsqueda actual. estado == 2·gen: abierto; 2·gen + 1: cerrado;
    /// cualquier otro valor: no visto en esta búsqueda (valor de una búsqueda anterior).
    struct EstadoNodo {
        double g;
        Nodo padre;
        std::uint32_t estado;
    };
    static_assert(sizeof(EstadoNodo) == 16);

    void prepararEspacio(std::uint32_t numNodos);
    template <bool UsarH>
    void expandir(const GrafoRejilla& grafo, Nodo origen, Nodo destino, ResultadoBusqueda& r, bool registrar);

    Algoritmo algoritmo_;
    // Arreglo indexado por nodo que se reutiliza entre búsquedas: reiniciar es O(1) en vez de
    // O(V) porque solo cambia la generación (arc42 §8.5).
    std::vector<EstadoNodo> nodos_;
    std::uint32_t generacion_{0};
    MonticuloBinario abierta_;
};

/// Fábrica de buscadores (patrón Factory). RNF-026: un algoritmo nuevo se registra aquí.
[[nodiscard]] std::unique_ptr<IBuscadorRuta> crearBuscador(Algoritmo a);

}  // namespace refugio::navegacion
