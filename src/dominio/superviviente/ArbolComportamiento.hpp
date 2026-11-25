#pragma once
/// @file ArbolComportamiento.hpp
/// @brief Árbol de comportamiento (patrón Composite) para ejecutar las acciones del plan (RF-062).
///
/// Nodos: Secuencia y Selector (compuestos), Inversor y ForzarExito (decoradores), Condicion y
/// Accion (hojas). Cada tick devuelve Exito, Fallo o EnCurso. Los compuestos son reactivos:
/// vuelven a evaluar desde el primer hijo en cada tick, así una precondición que deja de
/// cumplirse hace fallar la rama en el siguiente tick (RF-064).
///
/// El árbol se construye una vez (al cargar) y tick() no asigna memoria. tick() es recursivo,
/// con profundidad máxima declarada kProfundidadMaxArbol = 10 (RNF-027).

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace refugio::superviviente {

enum class EstadoNodo : std::uint8_t { Exito, Fallo, EnCurso };
enum class TipoNodo : std::uint8_t { Secuencia, Selector, Inversor, ForzarExito, Condicion, Accion };

inline constexpr int kProfundidadMaxArbol = 10;

/// Quien sabe evaluar las hojas: el árbol solo conoce su clave numérica.
class EjecutorHojas {
public:
    virtual ~EjecutorHojas() = default;
    [[nodiscard]] virtual bool condicion(std::uint16_t clave) = 0;
    [[nodiscard]] virtual EstadoNodo accion(std::uint16_t clave) = 0;
};

class ArbolComportamiento {
public:
    using Id = std::uint16_t;

    Id secuencia(std::initializer_list<Id> hijos);
    Id selector(std::initializer_list<Id> hijos);
    Id inversor(Id hijo);
    Id forzarExito(Id hijo);
    Id condicion(std::uint16_t clave);
    Id accion(std::uint16_t clave);

    /// Evalúa el subárbol de @p raiz. Más allá de kProfundidadMaxArbol devuelve Fallo.
    [[nodiscard]] EstadoNodo tick(Id raiz, EjecutorHojas& hojas) const noexcept { return tick(raiz, hojas, 1); }
    /// Niveles del subárbol (una hoja sola tiene profundidad 1).
    [[nodiscard]] int profundidad(Id raiz) const noexcept;
    [[nodiscard]] std::size_t numNodos() const noexcept { return nodos_.size(); }
    [[nodiscard]] TipoNodo tipo(Id n) const noexcept { return nodos_[n].tipo; }

private:
    struct Nodo {
        TipoNodo tipo{};
        std::uint16_t primerHijo{0};
        std::uint16_t numHijos{0};
        std::uint16_t clave{0};
    };
    Id agregar(TipoNodo t, std::initializer_list<Id> hijos, std::uint16_t clave = 0);
    [[nodiscard]] EstadoNodo tick(Id n, EjecutorHojas& hojas, int prof) const noexcept;

    std::vector<Nodo> nodos_;
    std::vector<Id> hijos_;
};

}  // namespace refugio::superviviente
