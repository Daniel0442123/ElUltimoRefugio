#include "dominio/coordinacion/PropagadorAlertas.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

namespace refugio::coordinacion {
namespace {
// Se usa la dirección del marco real y no la de una variable local, porque con ASan las
// variables locales pueden vivir en una «pila falsa» y la medición dejaría de ser válida.
#if defined(_MSC_VER) && !defined(__clang__)
__declspec(noinline) std::uintptr_t direccionMarcoActual() {
    return reinterpret_cast<std::uintptr_t>(_AddressOfReturnAddress());
}
#else
__attribute__((noinline)) std::uintptr_t direccionMarcoActual() {
    return reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
}
#endif
}  // namespace

ResultadoPropagacion PropagadorAlertas::propagar(const std::vector<Vec2>& posiciones, const Quadtree& indice,
                                                 std::uint32_t origen, float confianza) {
    ResultadoPropagacion r;
    propagarEn(posiciones, indice, origen, confianza, r);
    return r;
}

void PropagadorAlertas::reservar(std::size_t maxAgentes, ResultadoPropagacion& r) {
    const auto niveles = static_cast<std::size_t>(std::max(parametros_.profundidadMax, 0) + 1);
    if (bufferPorNivel_.size() < niveles) bufferPorNivel_.resize(niveles);
    for (auto& b : bufferPorNivel_) b.reserve(maxAgentes);
    visitado_.reserve(maxAgentes);
    r.mensajes.reserve(maxAgentes);  // cada agente recibe a lo sumo un mensaje
    r.saltoPorAgente.reserve(maxAgentes);
}

void PropagadorAlertas::propagarEn(const std::vector<Vec2>& posiciones, const Quadtree& indice, std::uint32_t origen,
                                   float confianza, ResultadoPropagacion& r) {
    r.mensajes.clear();
    r.saltoPorAgente.assign(posiciones.size(), -1);
    r.profundidadAlcanzada = 0;
    r.llamadas = 0;
    r.bytesPilaEstimados = 0;
    if (origen >= posiciones.size()) return;

    posiciones_ = &posiciones;
    indice_ = &indice;
    resultado_ = &r;
    visitado_.assign(posiciones.size(), 0);
    const auto niveles = static_cast<std::size_t>(std::max(parametros_.profundidadMax, 0) + 1);
    if (bufferPorNivel_.size() < niveles) bufferPorNivel_.resize(niveles);

    visitado_[origen] = 1;
    r.saltoPorAgente[origen] = 0;
    propagarRec(origen, 0, confianza);

    posiciones_ = nullptr;
    indice_ = nullptr;
    resultado_ = nullptr;
}

void PropagadorAlertas::propagarRec(std::uint32_t agente, int profundidad, float confianza) {
    // Medición real del uso de pila: dirección del marco actual en cada nivel.
    const std::uintptr_t direccion = direccionMarcoActual();
    if (profundidad == 0) direccionPilaBase_ = direccion;
    const std::uintptr_t usado = direccionPilaBase_ > direccion ? direccionPilaBase_ - direccion
                                                                  : direccion - direccionPilaBase_;
    resultado_->bytesPilaEstimados = std::max(resultado_->bytesPilaEstimados, static_cast<std::size_t>(usado));

    ++resultado_->llamadas;
    resultado_->profundidadAlcanzada = std::max(resultado_->profundidadAlcanzada, profundidad);
    assert(profundidad <= parametros_.profundidadMax);

    if (profundidad >= parametros_.profundidadMax) return;  // caso base 1: límite de profundidad

    auto& vecinos = bufferPorNivel_[static_cast<std::size_t>(profundidad)];
    vecinos.clear();
    indice_->consultarRadio((*posiciones_)[agente], parametros_.radioComunicacion, vecinos);
    std::sort(vecinos.begin(), vecinos.end());  // orden determinista (RNF-020)

    const int salto = profundidad + 1;
    const float confianzaSalto = confianza * parametros_.atenuacion;
    // Primera pasada: marcar y entregar a todos los vecinos directos no visitados.
    std::size_t nuevos = 0;
    for (const std::uint32_t b : vecinos) {
        if (visitado_[b] != 0) continue;  // caso base 2: ya alertado
        visitado_[b] = 1;
        resultado_->saltoPorAgente[b] = salto;
        resultado_->mensajes.push_back({agente, b, salto, confianzaSalto});
        vecinos[nuevos++] = b;
    }
    vecinos.resize(nuevos);
    // Segunda pasada: recursión. Se copia el índice porque el buffer del nivel siguiente es otro.
    for (std::size_t i = 0; i < vecinos.size(); ++i) {
        propagarRec(bufferPorNivel_[static_cast<std::size_t>(profundidad)][i], salto, confianzaSalto);
    }
}

}  // namespace refugio::coordinacion
