#pragma once
/// @file Goap.hpp
/// @brief Planificación GOAP con búsqueda A* regresiva (RF-061, ADR-0008).
///
/// El estado del mundo es un conjunto de hechos booleanos (máscara de 32 bits). Cada acción
/// tiene precondiciones, efectos que añaden y efectos que borran hechos, y un costo.
/// La búsqueda parte de la meta y retrocede: una acción es relevante si añade algún hecho que
/// se exige y no borra ninguno de los que se exigen. Al retroceder por ella, los hechos que añade
/// dejan de exigirse y se exigen sus precondiciones. Termina cuando el estado actual cumple
/// todo lo exigido. El camino del nodo final a la raíz da el plan en orden de ejecución.
///
/// Profundidad máxima declarada: kProfundidadMaxGoap = 6 acciones (RF-061, RNF-027). La búsqueda
/// es iterativa y usa un arreglo fijo de nodos, así que no asigna memoria al planificar.

#include <array>
#include <cstdint>
#include <span>

namespace refugio::superviviente {

using Hechos = std::uint32_t;

inline constexpr std::size_t kProfundidadMaxGoap = 6;
inline constexpr std::size_t kMaxNodosGoap = 1024;

struct AccionGoap {
    Hechos pre{0};      ///< Hechos que deben ser ciertos para ejecutarla.
    Hechos anade{0};    ///< Hechos que pasan a ser ciertos.
    Hechos borra{0};    ///< Hechos que dejan de ser ciertos.
    float costo{1.0f};  ///< > 0
    bool habilitada{true};
};

struct PlanGoap {
    std::array<std::uint8_t, kProfundidadMaxGoap> acciones{};  ///< Índices en el catálogo, en orden.
    std::uint8_t longitud{0};
    float costo{0.0f};
    std::uint32_t expandidos{0};
    bool encontrado{false};
};

class PlanificadorGoap {
public:
    /// Plan de menor costo que lleva de @p estado a un estado que cumple @p meta (todos sus
    /// hechos ciertos), con a lo sumo @p profundidadMax acciones (tope kProfundidadMaxGoap).
    /// Heurística: hechos pendientes · costo mínimo / máximo de hechos que añade una acción
    /// (admisible: ninguna acción puede cumplir más hechos de golpe ni costar menos).
    [[nodiscard]] PlanGoap planificar(Hechos estado, Hechos meta, std::span<const AccionGoap> acciones,
                                      std::size_t profundidadMax = kProfundidadMaxGoap) noexcept;

private:
    struct Nodo {
        Hechos exigidos{0};
        float g{0.0f};
        float f{0.0f};
        std::int16_t padre{-1};
        std::uint8_t accion{0};
        std::uint8_t profundidad{0};
        bool cerrado{false};
    };
    std::array<Nodo, kMaxNodosGoap> nodos_{};
};

}  // namespace refugio::superviviente
