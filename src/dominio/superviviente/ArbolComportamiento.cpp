#include "dominio/superviviente/ArbolComportamiento.hpp"

#include <algorithm>

namespace refugio::superviviente {

ArbolComportamiento::Id ArbolComportamiento::agregar(TipoNodo t, std::initializer_list<Id> hijos, std::uint16_t clave) {
    Nodo n{t, static_cast<std::uint16_t>(hijos_.size()), static_cast<std::uint16_t>(hijos.size()), clave};
    hijos_.insert(hijos_.end(), hijos.begin(), hijos.end());
    nodos_.push_back(n);
    return static_cast<Id>(nodos_.size() - 1);
}

ArbolComportamiento::Id ArbolComportamiento::secuencia(std::initializer_list<Id> hijos) { return agregar(TipoNodo::Secuencia, hijos); }
ArbolComportamiento::Id ArbolComportamiento::selector(std::initializer_list<Id> hijos) { return agregar(TipoNodo::Selector, hijos); }
ArbolComportamiento::Id ArbolComportamiento::inversor(Id hijo) { return agregar(TipoNodo::Inversor, {hijo}); }
ArbolComportamiento::Id ArbolComportamiento::forzarExito(Id hijo) { return agregar(TipoNodo::ForzarExito, {hijo}); }
ArbolComportamiento::Id ArbolComportamiento::condicion(std::uint16_t clave) { return agregar(TipoNodo::Condicion, {}, clave); }
ArbolComportamiento::Id ArbolComportamiento::accion(std::uint16_t clave) { return agregar(TipoNodo::Accion, {}, clave); }

EstadoNodo ArbolComportamiento::tick(Id id, EjecutorHojas& hojas, int prof) const noexcept {
    if (prof > kProfundidadMaxArbol) return EstadoNodo::Fallo;
    const Nodo& n = nodos_[id];
    switch (n.tipo) {
        case TipoNodo::Secuencia:
            // Exito si todos lo tienen; se detiene en el primero que falla o sigue en curso.
            for (std::uint16_t k = 0; k < n.numHijos; ++k) {
                const EstadoNodo r = tick(hijos_[n.primerHijo + k], hojas, prof + 1);
                if (r != EstadoNodo::Exito) return r;
            }
            return EstadoNodo::Exito;
        case TipoNodo::Selector:
            // Fallo si todos fallan; se detiene en el primero que tiene éxito o sigue en curso.
            for (std::uint16_t k = 0; k < n.numHijos; ++k) {
                const EstadoNodo r = tick(hijos_[n.primerHijo + k], hojas, prof + 1);
                if (r != EstadoNodo::Fallo) return r;
            }
            return EstadoNodo::Fallo;
        case TipoNodo::Inversor: {
            const EstadoNodo r = tick(hijos_[n.primerHijo], hojas, prof + 1);
            return r == EstadoNodo::Exito ? EstadoNodo::Fallo : r == EstadoNodo::Fallo ? EstadoNodo::Exito : r;
        }
        case TipoNodo::ForzarExito: {
            const EstadoNodo r = tick(hijos_[n.primerHijo], hojas, prof + 1);
            return r == EstadoNodo::EnCurso ? r : EstadoNodo::Exito;
        }
        case TipoNodo::Condicion: return hojas.condicion(n.clave) ? EstadoNodo::Exito : EstadoNodo::Fallo;
        case TipoNodo::Accion: return hojas.accion(n.clave);
    }
    return EstadoNodo::Fallo;
}

int ArbolComportamiento::profundidad(Id id) const noexcept {
    const Nodo& n = nodos_[id];
    int maxHijo = 0;
    for (std::uint16_t k = 0; k < n.numHijos; ++k) maxHijo = std::max(maxHijo, profundidad(hijos_[n.primerHijo + k]));
    return 1 + maxHijo;
}

}  // namespace refugio::superviviente
