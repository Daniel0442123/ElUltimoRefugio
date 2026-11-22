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

// Los nodos y sus vectores de puntos se conservan entre reconstrucciones: tras los primeros
// ticks no se asigna memoria (RNF-006).
void Quadtree::limpiar() {
    if (nodos_.empty()) nodos_.push_back(NodoQT{raiz_, -1, 0, {}});
    for (std::size_t i = 0; i < usados_; ++i) nodos_[i].puntos.clear();
    nodos_[0].primerHijo = -1;
    usados_ = 1;
    profundidadMaxima_ = 0;
}

void Quadtree::reservar(std::size_t maxPuntos) {
    const std::size_t internosPorNivel = maxPuntos / (kCapacidadHoja + 1);
    const std::size_t total = 1 + 4 * static_cast<std::size_t>(kProfundidadMax) * internosPorNivel;
    while (nodos_.size() < total) nodos_.push_back(NodoQT{raiz_, -1, 0, {}});
    for (auto& n : nodos_) n.puntos.reserve(std::max<std::size_t>(maxPuntos, kCapacidadHoja));
}

std::int32_t Quadtree::nuevoNodo(Rect limites, std::int32_t nivel) {
    if (usados_ == nodos_.size()) nodos_.push_back(NodoQT{limites, -1, nivel, {}});
    NodoQT& n = nodos_[usados_];
    n.limites = limites;
    n.primerHijo = -1;
    n.nivel = nivel;
    n.puntos.clear();
    return static_cast<std::int32_t>(usados_++);
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
    const std::int32_t primero = nuevoNodo({r.x, r.y, w, h}, nivel);
    nuevoNodo({r.x + w, r.y, w, h}, nivel);
    nuevoNodo({r.x, r.y + h, w, h}, nivel);
    nuevoNodo({r.x + w, r.y + h, w, h}, nivel);
    profundidadMaxima_ = std::max(profundidadMaxima_, static_cast<int>(nivel));
    // Ojo: nuevoNodo puede reubicar nodos_, por eso se vuelve a indexar aquí. El padre tenía
    // exactamente kCapacidadHoja puntos, así que cada hijo recibe a lo sumo esa cantidad y el
    // reparto no necesita recursión.
    auto& padre = nodos_[static_cast<std::size_t>(indice)];
    padre.primerHijo = primero;
    for (const Punto& pt : padre.puntos) {
        const bool derecha = pt.p.x >= r.x + w;
        const bool abajo = pt.p.y >= r.y + h;
        nodos_[static_cast<std::size_t>(primero + (derecha ? 1 : 0) + (abajo ? 2 : 0))].puntos.push_back(pt);
    }
    padre.puntos.clear();  // conserva la capacidad
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
