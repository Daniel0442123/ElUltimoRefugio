#pragma once
/// @file EstresRecursion.hpp
/// @brief Prueba de estrés de las funciones recursivas (RNF-027, CP-127).
///
/// Mide, para hordas cada vez más grandes y densas, la profundidad alcanzada, el número de
/// llamadas, los mensajes, el tiempo y la pila usada por la propagación de alertas, y la
/// profundidad del quadtree con puntos coincidentes (peor caso de división).

#include <cstdint>
#include <ostream>
#include <vector>

namespace refugio::experimentos {

struct ConfigEstres {
    std::vector<int> agentes{50, 100, 200, 500, 1000, 2000};
    std::vector<int> profundidades{1, 2, 3, 6, 10};
    float radioComunicacion{5.0f};
    float lado{40.0f};  ///< Lado del área en celdas: cuanto menor, más denso el grafo de comunicación.
    int repeticiones{5};
    std::uint64_t semilla{42};
};

struct ResumenEstres {
    int filas{0};
    int violacionesLimite{0};       ///< Veces que la profundidad superó el límite declarado (debe ser 0).
    int violacionesUnicidad{0};     ///< Agentes alertados más de una vez (debe ser 0).
    std::size_t pilaMaximaBytes{0};
    int profundidadQuadtreeCoincidentes{0};
};

ResumenEstres ejecutarEstresRecursion(const ConfigEstres& cfg, std::ostream& csv);

}  // namespace refugio::experimentos
