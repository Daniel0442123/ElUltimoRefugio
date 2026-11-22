#pragma once
/// @file Quadtree.hpp
/// @brief Partición espacial para consultas de vecinos por radio (RF-030, ADR-0010).
///
/// Inserción y consulta son recursivas con profundidad máxima kProfundidadMax (RNF-027).
/// Los nodos viven en un arreglo (pool) y se enlazan por índice, no por punteros: se conserva
/// la estructura de árbol con la localidad de caché de un arreglo, y reconstruir no libera memoria.
///
/// Complejidad: construcción O(n log n); consulta O(log n + k) en promedio, k = puntos devueltos.

#include <array>
#include <cstdint>
#include <vector>

namespace refugio::navegacion {

struct Vec2 {
    float x{};
    float y{};
};

struct Rect {
    float x{}, y{}, ancho{}, alto{};
    [[nodiscard]] bool contiene(Vec2 p) const noexcept {
        return p.x >= x && p.y >= y && p.x < x + ancho && p.y < y + alto;
    }
    [[nodiscard]] bool intersecaCirculo(Vec2 c, float r) const noexcept;
};

class Quadtree {
public:
    static constexpr int kCapacidadHoja = 8;
    static constexpr int kProfundidadMax = 8;

    explicit Quadtree(Rect limites);

    /// Vacía el árbol conservando la memoria reservada (se reconstruye cada tick).
    void limpiar();
    /// Reserva el pool para hasta @p maxPuntos puntos: tras llamarla, limpiar() + insertar() no
    /// asignan memoria (RNF-006). Cota: un nodo se divide al recibir su punto kCapacidadHoja + 1,
    /// así que en cada nivel hay a lo sumo maxPuntos / (kCapacidadHoja + 1) nodos internos.
    void reservar(std::size_t maxPuntos);
    /// Inserta el punto @p p con identificador @p id. Devuelve false si está fuera de los límites.
    bool insertar(std::uint32_t id, Vec2 p);
    /// Agrega a @p salida los ids a distancia <= radio de @p centro.
    void consultarRadio(Vec2 centro, float radio, std::vector<std::uint32_t>& salida) const;

    [[nodiscard]] int profundidadMaxima() const noexcept { return profundidadMaxima_; }
    [[nodiscard]] std::size_t numNodos() const noexcept { return usados_; }

private:
    struct Punto {
        std::uint32_t id;
        Vec2 p;
    };
    struct NodoQT {
        Rect limites;
        std::int32_t primerHijo{-1};  ///< Índice del primero de 4 hijos contiguos; -1 si es hoja.
        std::int32_t nivel{0};
        std::vector<Punto> puntos;   ///< Solo en hojas. En el nivel máximo puede superar la capacidad.
    };

    void insertarRec(std::int32_t indice, const Punto& punto);
    void consultarRec(std::int32_t indice, Vec2 centro, float radio, std::vector<std::uint32_t>& salida) const;
    void dividir(std::int32_t indice);
    std::int32_t nuevoNodo(Rect limites, std::int32_t nivel);

    Rect raiz_;
    std::vector<NodoQT> nodos_;  ///< Pool; solo los primeros usados_ forman el árbol actual.
    std::size_t usados_{0};
    int profundidadMaxima_{0};
};

}  // namespace refugio::navegacion
