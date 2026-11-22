#pragma once
/// @file VistaDepuracion.hpp
/// @brief Adaptador raylib que dibuja el grafo, la ruta y los nodos expandidos (RF-071).

#include "dominio/mundo/Grafo.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"

namespace refugio::adaptadores {

struct EstiloVista {
    int tamCelda{32};  ///< Tiles de 32 x 32 px (RES-04).
    int origenX{0};
    int origenY{0};
};

void dibujarGrafo(const mundo::GrafoRejilla& g, const EstiloVista& e);
void dibujarExpansion(const mundo::GrafoRejilla& g, const navegacion::ResultadoBusqueda& r, const EstiloVista& e);
void dibujarRuta(const mundo::GrafoRejilla& g, const navegacion::ResultadoBusqueda& r, const EstiloVista& e);

}  // namespace refugio::adaptadores
