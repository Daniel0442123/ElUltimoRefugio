#include "dominio/navegacion/BuscadorRuta.hpp"

#include <algorithm>
#include <chrono>

namespace refugio::navegacion {

std::string_view nombreAlgoritmo(Algoritmo a) noexcept {
    return a == Algoritmo::Dijkstra ? "Dijkstra" : "AEstrella";
}

// ----------------------------------------------------------------- MonticuloBinario
// Se mueve un «hueco» en lugar de intercambiar: una escritura por nivel en vez de tres.
void MonticuloBinario::insertar(Entrada e) {
    datos_.push_back(e);
    Entrada* d = datos_.data();
    std::size_t i = datos_.size() - 1;
    while (i > 0) {
        const std::size_t p = (i - 1) / 2;
        if (!menor(e, d[p])) break;
        d[i] = d[p];
        i = p;
    }
    d[i] = e;
}

// Extracción «de abajo arriba» (Floyd): el hueco de la cima baja siempre por el hijo menor hasta
// una hoja, sin compararlo con el último elemento; después el último sube desde esa hoja. El
// último casi siempre pertenece cerca del fondo, así que se hacen ~log n comparaciones en vez de
// ~2 log n, y la elección del hijo es sin saltos (el predictor de saltos no puede acertarla).
MonticuloBinario::Entrada MonticuloBinario::extraer() {
    const Entrada cima = datos_.front();
    const Entrada ultimo = datos_.back();
    datos_.pop_back();
    const std::size_t n = datos_.size();
    if (n == 0) return cima;
    Entrada* d = datos_.data();
    std::size_t i = 0;
    std::size_t hijo = 1;
    while (hijo + 1 < n) {
        hijo += static_cast<std::size_t>(menor(d[hijo + 1], d[hijo]));
        d[i] = d[hijo];
        i = hijo;
        hijo = 2 * i + 1;
    }
    if (hijo < n) {  // un solo hijo en el último nivel
        d[i] = d[hijo];
        i = hijo;
    }
    while (i > 0) {
        const std::size_t p = (i - 1) / 2;
        if (!menor(ultimo, d[p])) break;
        d[i] = d[p];
        i = p;
    }
    d[i] = ultimo;
    return cima;
}

// ------------------------------------------------------------- BuscadorMejorPrimero
void BuscadorMejorPrimero::prepararEspacio(std::uint32_t numNodos) {
    if (nodos_.size() != numNodos) {
        nodos_.assign(numNodos, EstadoNodo{0.0, 0, 0});
        generacion_ = 0;
    }
    ++generacion_;
    if (generacion_ >= 0x7FFFFFFFu) {  // 2·gen + 1 desbordaría: reinicio completo (tras 2^31 búsquedas)
        for (auto& e : nodos_) e.estado = 0;
        generacion_ = 1;
    }
    abierta_.limpiar();
}

void BuscadorMejorPrimero::reservar(std::uint32_t numNodos) {
    prepararEspacio(numNodos);
    abierta_.reservar(8u * static_cast<std::size_t>(numNodos) + 1u);
}

template <bool UsarH>
void BuscadorMejorPrimero::expandir(const GrafoRejilla& grafo, Nodo origen, Nodo destino, ResultadoBusqueda& r,
                                    bool registrar) {
    const int ancho = grafo.ancho();
    const int alto = grafo.alto();
    const std::uint8_t* costo = grafo.datosCosto();
    const Nodo w = static_cast<Nodo>(ancho);
    const std::uint32_t abierto = 2u * generacion_;
    const std::uint32_t cerrado = abierto + 1u;
    EstadoNodo* nodos = nodos_.data();
    const mundo::Celda cd = grafo.celda(destino);

    // Distancia octil a destino sin divisiones: las coordenadas del vecino se derivan de las de u.
    auto h = [&](int x, int y) noexcept -> double {
        if constexpr (UsarH) {
            const double dx = static_cast<double>(x > cd.x ? x - cd.x : cd.x - x);
            const double dy = static_cast<double>(y > cd.y ? y - cd.y : cd.y - y);
            return (dx + dy) + (mundo::kRaiz2 - 2.0) * (dx < dy ? dx : dy);
        } else {
            (void)x;
            (void)y;
            return 0.0;
        }
    };

    const mundo::Celda co = grafo.celda(origen);
    nodos[origen] = {0.0, origen, abierto};
    const double h0 = h(co.x, co.y);
    abierta_.insertar({h0, static_cast<float>(h0), origen});
    r.generados = 1;
    r.maxAbierta = 1;

    while (!abierta_.vacio()) {
        const auto actual = abierta_.extraer();
        const Nodo u = actual.nodo;
        EstadoNodo& eu = nodos[u];
        if (eu.estado == cerrado) continue;  // copia vieja (eliminación perezosa)
        eu.estado = cerrado;
        ++r.expandidos;
        if (registrar) r.ordenExpansion.push_back(u);
        if (u == destino) {
            r.encontrada = true;
            break;
        }
        const double gu = eu.g;
        const int x = static_cast<int>(u % w);
        const int y = static_cast<int>(u / w);

        auto relajar = [&](Nodo v, int vx, int vy, double peso) {
            EstadoNodo& ev = nodos[v];
            if (ev.estado == cerrado) return;  // sin reapertura: h consistente
            const double gv = gu + peso;
            if (ev.estado != abierto || gv < ev.g) {
                ev = {gv, u, abierto};
                const double hv = h(vx, vy);
                abierta_.insertar({gv + hv, static_cast<float>(hv), v});
                ++r.generados;
            }
        };

        // Mismo orden de vecinos que GrafoRejilla::paraCadaVecino: E, O, S, N, SE, NE, SO, NO.
        const std::uint8_t mE = x + 1 < ancho ? costo[u + 1] : mundo::kBloqueado;
        const std::uint8_t mO = x > 0 ? costo[u - 1] : mundo::kBloqueado;
        const std::uint8_t mS = y + 1 < alto ? costo[u + w] : mundo::kBloqueado;
        const std::uint8_t mN = y > 0 ? costo[u - w] : mundo::kBloqueado;
        if (mE) relajar(u + 1, x + 1, y, static_cast<double>(mE));
        if (mO) relajar(u - 1, x - 1, y, static_cast<double>(mO));
        if (mS) relajar(u + w, x, y + 1, static_cast<double>(mS));
        if (mN) relajar(u - w, x, y - 1, static_cast<double>(mN));
        // Una diagonal solo existe si las dos ortogonales que rodea son transitables (RF-020).
        if (mE && mS) {
            if (const auto m = costo[u + w + 1]) relajar(u + w + 1, x + 1, y + 1, mundo::kRaiz2 * m);
        }
        if (mE && mN) {
            if (const auto m = costo[u - w + 1]) relajar(u - w + 1, x + 1, y - 1, mundo::kRaiz2 * m);
        }
        if (mO && mS) {
            if (const auto m = costo[u + w - 1]) relajar(u + w - 1, x - 1, y + 1, mundo::kRaiz2 * m);
        }
        if (mO && mN) {
            if (const auto m = costo[u - w - 1]) relajar(u - w - 1, x - 1, y - 1, mundo::kRaiz2 * m);
        }
        const auto tam = static_cast<std::uint32_t>(abierta_.tamano());
        if (tam > r.maxAbierta) r.maxAbierta = tam;
    }
}

void BuscadorMejorPrimero::buscarEn(const GrafoRejilla& grafo, Nodo origen, Nodo destino, ResultadoBusqueda& r,
                                    bool registrarExpansion) {
    using Reloj = std::chrono::steady_clock;
    const auto inicio = Reloj::now();

    // Se vacían los vectores sin liberar su memoria (RNF-006).
    r.algoritmo = algoritmo_;
    r.encontrada = false;
    r.ruta.clear();
    r.costo = 0.0;
    r.expandidos = 0;
    r.generados = 0;
    r.maxAbierta = 0;
    r.ordenExpansion.clear();
    prepararEspacio(grafo.numCeldas());

    if (origen < grafo.numCeldas() && destino < grafo.numCeldas() && grafo.transitable(origen) &&
        grafo.transitable(destino)) {
        if (algoritmo_ == Algoritmo::AEstrella)
            expandir<true>(grafo, origen, destino, r, registrarExpansion);
        else
            expandir<false>(grafo, origen, destino, r, registrarExpansion);
    }

    if (r.encontrada) {
        r.costo = nodos_[destino].g;
        for (Nodo n = destino;; n = nodos_[n].padre) {
            r.ruta.push_back(n);
            if (n == origen) break;
        }
        std::reverse(r.ruta.begin(), r.ruta.end());
    }

    r.memoriaBytes = nodos_.capacity() * sizeof(EstadoNodo) +
                     static_cast<std::size_t>(r.maxAbierta) * sizeof(MonticuloBinario::Entrada);
    r.tiempoUs = std::chrono::duration<double, std::micro>(Reloj::now() - inicio).count();
}

std::unique_ptr<IBuscadorRuta> crearBuscador(Algoritmo a) { return std::make_unique<BuscadorMejorPrimero>(a); }

}  // namespace refugio::navegacion
