#include "adaptadores/raylib/VistaDepuracion.hpp"

#include <raylib.h>

namespace refugio::adaptadores {
namespace {
Rectangle rectCelda(const mundo::GrafoRejilla& g, mundo::Nodo n, const EstiloVista& e) {
    const auto c = g.celda(n);
    return {static_cast<float>(e.origenX + c.x * e.tamCelda), static_cast<float>(e.origenY + c.y * e.tamCelda),
            static_cast<float>(e.tamCelda), static_cast<float>(e.tamCelda)};
}
}  // namespace

void dibujarGrafo(const mundo::GrafoRejilla& g, const EstiloVista& e) {
    for (mundo::Nodo n = 0; n < g.numCeldas(); ++n) {
        const auto m = g.multiplicador(n);
        Color color = {40, 44, 52, 255};                       // pared o barricada
        if (m == 1) color = {200, 200, 190, 255};               // calle
        else if (m > 1) color = {130, 100, 70, 255};            // escombros (más caro): marrón
        DrawRectangleRec(rectCelda(g, n, e), color);
    }
    for (int x = 0; x <= g.ancho(); ++x)
        DrawLine(e.origenX + x * e.tamCelda, e.origenY, e.origenX + x * e.tamCelda, e.origenY + g.alto() * e.tamCelda,
                 Fade(BLACK, 0.15f));
    for (int y = 0; y <= g.alto(); ++y)
        DrawLine(e.origenX, e.origenY + y * e.tamCelda, e.origenX + g.ancho() * e.tamCelda, e.origenY + y * e.tamCelda,
                 Fade(BLACK, 0.15f));
}

void dibujarExpansion(const mundo::GrafoRejilla& g, const navegacion::ResultadoBusqueda& r, const EstiloVista& e) {
    // Cian para Dijkstra y magenta para A*: ninguno se confunde con el marrón de los escombros.
    const Color c = r.algoritmo == navegacion::Algoritmo::Dijkstra ? Color{0, 200, 220, 110} : Color{220, 60, 220, 110};
    for (const auto n : r.ordenExpansion) DrawRectangleRec(rectCelda(g, n, e), c);
}

void dibujarRuta(const mundo::GrafoRejilla& g, const navegacion::ResultadoBusqueda& r, const EstiloVista& e) {
    const float mitad = static_cast<float>(e.tamCelda) / 2.0f;
    for (std::size_t i = 1; i < r.ruta.size(); ++i) {
        const auto a = rectCelda(g, r.ruta[i - 1], e);
        const auto b = rectCelda(g, r.ruta[i], e);
        DrawLineEx({a.x + mitad, a.y + mitad}, {b.x + mitad, b.y + mitad}, 4.0f, RED);
    }
}

}  // namespace refugio::adaptadores
