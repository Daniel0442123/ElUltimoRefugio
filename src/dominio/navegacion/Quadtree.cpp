#include "dominio/navegacion/Quadtree.hpp"

#include <algorithm>
#include <cassert>

namespace refugio::navegacion {

bool Rect::intersecaCirculo(Vec2 c, float r) const noexcept {
    const float px = std::clamp(c.x, x, x + ancho);
    const float py = std::clamp(c.y, y, y + alto);
    const float dx = c.x - px;
    const float dy = c.y - py;
    return dx * dx + dy * dy <= r * r;
}

Quadtree::Quadtree(Rect limites) : raiz_(limites) { limpiar(); }

void Quadtree::limpiar() {
    nodos_.clear();
    nodos_.push_back(NodoQT{raiz_, -1, 0, {}});
    profundidadMaxima_ = 0;
}

bool Quadtree::insertar(std::uint32_t id, Vec2 p) {
    if (!raiz_.contiene(p)) return false;
    insertarRec(0, Punto{id, p});
    return true;
}

void Quadtree::dividir(std::int32_t indice) {
    const Rect r = nodos_[static_cast<std::size_t>(indice)].limites;
    const std::int32_t nivel = nodos_[static_cast<std::size_t>(indice)].nivel + 1;
    const float w = r.ancho / 2.0f;
    const float h = r.alto / 2.0f;
    const auto primero = static_cast<std::int32_t>(nodos_.size());
    nodos_.push_back(NodoQT{{r.x, r.y, w, h}, -1, nivel, {}});
    nodos_.push_back(NodoQT{{r.x + w, r.y, w, h}, -1, nivel, {}});
    nodos_.push_back(NodoQT{{r.x, r.y + h, w, h}, -1, nivel, {}});
    nodos_.push_back(NodoQT{{r.x + w, r.y + h, w, h}, -1, nivel, {}});
    profundidadMaxima_ = std::max(profundidadMaxima_, static_cast<int>(nivel));
    // Ojo: push_back puede reubicar nodos_, por eso se vuelve a indexar aquí.
    auto& padre = nodos_[static_cast<std::size_t>(indice)];
    padre.primerHijo = primero;
    std::vector<Punto> pendientes;
    pendientes.swap(padre.puntos);
    for (const Punto& pt : pendientes) insertarRec(indice, pt);
}

// Recursión: caso base = hoja con espacio o en el nivel máximo; progreso = nivel + 1 en cada
// llamada, acotado por kProfundidadMax, luego la profundidad de pila es <= kProfundidadMax + 1.
void Quadtree::insertarRec(std::int32_t indice, const Punto& punto) {
    NodoQT& nodo = nodos_[static_cast<std::size_t>(indice)];
    assert(nodo.nivel <= kProfundidadMax);
    if (nodo.primerHijo < 0) {
        if (static_cast<int>(nodo.puntos.size()) < kCapacidadHoja || nodo.nivel >= kProfundidadMax) {
            nodo.puntos.push_back(punto);
            return;
        }
        dividir(indice);  // reubica los puntos en los hijos
    }
    const NodoQT& actual = nodos_[static_cast<std::size_t>(indice)];
    const Rect r = actual.limites;
    const bool derecha = punto.p.x >= r.x + r.ancho / 2.0f;
    const bool abajo = punto.p.y >= r.y + r.alto / 2.0f;
    const std::int32_t hijo = actual.primerHijo + (derecha ? 1 : 0) + (abajo ? 2 : 0);
    insertarRec(hijo, punto);
}

void Quadtree::consultarRadio(Vec2 centro, float radio, std::vector<std::uint32_t>& salida) const {
    consultarRec(0, centro, radio, salida);
}

void Quadtree::consultarRec(std::int32_t indice, Vec2 centro, float radio,
                            std::vector<std::uint32_t>& salida) const {
    const NodoQT& nodo = nodos_[static_cast<std::size_t>(indice)];
    if (!nodo.limites.intersecaCirculo(centro, radio)) return;  // caso base: poda
    if (nodo.primerHijo < 0) {                                     // caso base: hoja
        const float r2 = radio * radio;
        for (const Punto& pt : nodo.puntos) {
            const float dx = pt.p.x - centro.x;
            const float dy = pt.p.y - centro.y;
            if (dx * dx + dy * dy <= r2) salida.push_back(pt.id);
        }
        return;
    }
    for (std::int32_t k = 0; k < 4; ++k) consultarRec(nodo.primerHijo + k, centro, radio, salida);
}

}  // namespace refugio::navegacion
