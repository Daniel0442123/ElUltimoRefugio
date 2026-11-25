#include "dominio/experimentos/Benchmark.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>

#include "dominio/experimentos/GeneradorMapas.hpp"
#include "dominio/navegacion/Comparacion.hpp"
#include "dominio/nucleo/Json.hpp"

namespace refugio::experimentos {

using navegacion::Algoritmo;
using navegacion::IBuscadorRuta;
using navegacion::ResultadoBusqueda;

namespace {

FabricaBuscador fabricaPorDefecto(const FabricaBuscador& f) {
    if (f) return f;
    return [](Algoritmo a) { return navegacion::crearBuscador(a); };
}

/// Semilla derivada por tamaño: cada mapa se puede regenerar de forma independiente (RF-086).
std::uint64_t semillaMapa(std::uint64_t maestra, int tam) noexcept {
    return maestra * 1000003ULL + static_cast<std::uint64_t>(tam);
}

void escribirCabecera(const ConfigBenchmark& cfg, const std::vector<std::string>& metadatos, std::string_view escenario,
                      std::ostream& csv) {
    csv << std::setprecision(10);
    for (const auto& linea : metadatos) csv << "# " << linea << '\n';
    csv << "# escenario=" << escenario << " config=" << (cfg.archivoConfig.empty() ? "linea_de_comandos" : cfg.archivoConfig)
        << '\n';
    csv << "# semilla_maestra=" << cfg.semilla << " pares=" << cfg.pares << " repeticiones=" << cfg.repeticiones
        << " densidad=" << cfg.densidad << " terreno_caro=" << cfg.terrenoCaro << '\n';
    if (escenario == "dinamico")
        for (const auto& e : cfg.dinamico.eventos) {
            csv << "# evento tick=" << e.tick;
            if (e.x >= 0 && e.y >= 0) csv << " celda=(" << e.x << ',' << e.y << ')';
            else csv << " adelante=" << e.adelante;
            csv << " multiplicador=" << static_cast<int>(e.multiplicador) << '\n';
        }
    const auto& cols = columnasBenchmark();
    for (std::size_t i = 0; i < cols.size(); ++i) csv << (i ? "," : "") << cols[i];
    csv << '\n';
}

struct Fila {
    std::string_view algoritmo;
    std::string_view mapa;
    int tam;
    double densidad;
    std::uint64_t semilla;
    std::size_t par;
    int repeticion;
    mundo::Celda origen;
    mundo::Celda destino;
    bool encontrada;
    double costo;
    std::size_t longitud;
    std::uint64_t expandidos;
    std::uint64_t generados;
    std::uint32_t maxAbierta;
    double tiempoUs;
    std::size_t memoria;
    int replanificaciones;
    double costoInicial;
    int eventos;
};

void escribirFila(const Fila& f, std::ostream& csv) {
    csv << f.algoritmo << ',' << f.mapa << ',' << f.tam << ',' << f.tam << ',' << f.densidad << ',' << f.semilla << ','
        << f.par << ',' << f.repeticion << ',' << f.origen.x << ',' << f.origen.y << ',' << f.destino.x << ','
        << f.destino.y << ',' << (f.encontrada ? 1 : 0) << ',' << f.costo << ',' << f.longitud << ',' << f.expandidos
        << ',' << f.generados << ',' << f.maxAbierta << ',' << f.tiempoUs << ',' << f.memoria << ','
        << f.replanificaciones << ',' << f.costoInicial << ',' << f.eventos << '\n';
}

/// Resultado de caminar de origen a destino con cambios del mapa (RF-083).
struct Recorrido {
    bool llego{false};
    double costo{0.0};
    double costoInicial{0.0};
    std::size_t celdas{0};
    std::uint64_t expandidos{0};
    std::uint64_t generados{0};
    std::uint32_t maxAbierta{0};
    double tiempoUs{0.0};
    std::size_t memoria{0};
    int replanificaciones{0};
    int eventos{0};
};

void acumular(Recorrido& rc, const ResultadoBusqueda& r) {
    rc.expandidos += r.expandidos;
    rc.generados += r.generados;
    rc.maxAbierta = std::max(rc.maxAbierta, r.maxAbierta);
    rc.tiempoUs += r.tiempoUs;
    rc.memoria = std::max(rc.memoria, r.memoriaBytes);
}

/// El agente avanza una celda de su ruta por tick. Antes de moverse aplica los eventos del tick;
/// si alguno toca la ruta que le queda, replanifica desde donde está (RF-025, RF-083).
Recorrido recorrer(mundo::GrafoRejilla grafo, mundo::Nodo origen, mundo::Nodo destino,
                   const std::vector<EventoDinamico>& eventos, IBuscadorRuta& buscador, ResultadoBusqueda& r,
                   std::vector<mundo::Nodo>& ruta) {
    Recorrido rc;
    buscador.buscarEn(grafo, origen, destino, r);
    acumular(rc, r);
    if (!r.encontrada) return rc;
    rc.costoInicial = r.costo;
    ruta.assign(r.ruta.begin(), r.ruta.end());
    std::size_t idx = 0;
    mundo::Nodo actual = origen;
    rc.celdas = 1;
    std::size_t ev = 0;
    for (int tick = 1; actual != destino; ++tick) {
        bool tocaRuta = false;
        for (; ev < eventos.size() && eventos[ev].tick <= tick; ++ev) {
            const auto& e = eventos[ev];
            mundo::Nodo celda = 0;
            if (e.x >= 0 && e.y >= 0) {
                if (!grafo.dentro(e.x, e.y)) continue;
                celda = grafo.nodo(e.x, e.y);
            } else {
                // Celda «adelante» pasos por delante, sin llegar al destino.
                if (ruta.size() < idx + 3) continue;
                celda = ruta[std::min(idx + static_cast<std::size_t>(e.adelante), ruta.size() - 2)];
            }
            if (celda == actual || celda == destino) continue;
            const auto c = grafo.celda(celda);
            grafo.fijarMultiplicador(c.x, c.y, e.multiplicador);
            ++rc.eventos;
            if (std::find(ruta.begin() + static_cast<std::ptrdiff_t>(idx) + 1, ruta.end(), celda) != ruta.end())
                tocaRuta = true;
        }
        if (tocaRuta) {
            buscador.buscarEn(grafo, actual, destino, r);
            acumular(rc, r);
            ++rc.replanificaciones;
            if (!r.encontrada) return rc;
            ruta.assign(r.ruta.begin(), r.ruta.end());
            idx = 0;
        }
        const mundo::Nodo siguiente = ruta[idx + 1];
        const auto a = grafo.celda(actual);
        const auto b = grafo.celda(siguiente);
        const double d = (a.x != b.x && a.y != b.y) ? std::sqrt(2.0) : 1.0;
        rc.costo += d * static_cast<double>(grafo.multiplicador(siguiente));
        actual = siguiente;
        ++idx;
        ++rc.celdas;
    }
    rc.llego = true;
    return rc;
}

// --- lectura de la configuración --------------------------------------------------------------

using nucleo::ErrorJson;
using nucleo::ValorJson;

void soloClaves(const ValorJson& v, std::string_view contexto, std::initializer_list<std::string_view> validas) {
    for (const auto& miembro : v.comoObjeto(contexto)) {
        if (std::find(validas.begin(), validas.end(), miembro.first) == validas.end())
            throw ErrorJson(std::string(contexto) + ": clave desconocida \"" + miembro.first + "\"");
    }
}

std::vector<int> listaTamanos(const ValorJson& v, const std::string& contexto) {
    std::vector<int> t;
    for (const auto& e : v.comoArreglo(contexto)) t.push_back(static_cast<int>(e.comoEntero(contexto, 4, 4096)));
    if (t.empty()) throw ErrorJson(contexto + ": debe tener al menos un tamano");
    return t;
}

double numeroEn(const ValorJson& v, const std::string& contexto, double minimo, double maximo) {
    const double d = v.comoNumero(contexto);
    if (d < minimo || d > maximo)
        throw ErrorJson(contexto + ": debe estar entre " + std::to_string(minimo) + " y " + std::to_string(maximo));
    return d;
}

}  // namespace

const std::vector<std::string_view>& columnasBenchmark() {
    static const std::vector<std::string_view> c{
        "algoritmo",        "mapa",           "ancho",           "alto",       "densidad_obstaculos",
        "semilla",          "par",            "repeticion",      "origen_x",   "origen_y",
        "destino_x",        "destino_y",      "encontrada",      "costo",      "longitud",
        "nodos_expandidos", "nodos_generados", "max_abierta",    "tiempo_us",  "memoria_pico_bytes",
        "replanificaciones", "costo_inicial", "eventos_aplicados"};
    return c;
}

ResumenBenchmark ejecutarBenchmark(const ConfigBenchmark& cfg, const std::vector<std::string>& metadatos,
                                   std::ostream& csv, const FabricaBuscador& fabrica) {
    ResumenBenchmark resumen;
    escribirCabecera(cfg, metadatos, "estatico", csv);

    const auto crear = fabricaPorDefecto(fabrica);
    auto dijkstra = crear(Algoritmo::Dijkstra);
    auto aestrella = crear(Algoritmo::AEstrella);
    navegacion::ComparacionBusqueda cmp;  // reutiliza sus dos resultados en cada consulta

    for (const int tam : cfg.tamanos) {
        const std::uint64_t semilla = semillaMapa(cfg.semilla, tam);
        nucleo::Pcg32 rng(semilla, 1u);
        const auto grafo = generarMapa({tam, tam, cfg.densidad, cfg.terrenoCaro}, rng);
        const auto pares = generarPares(grafo, cfg.pares, rng);
        const std::string mapa = cfg.mapa + "_" + std::to_string(tam);

        for (std::size_t i = 0; i < pares.size(); ++i) {
            const auto [o, d] = pares[i];
            for (int rep = 0; rep < cfg.repeticiones; ++rep) {
                // Se alterna el orden para no favorecer a ninguno por efectos de caché. Los
                // resultados se reutilizan, como en el juego: no se mide la asignación de memoria.
                if (rep % 2 == 0) {
                    navegacion::compararAlgoritmos(grafo, o, d, *dijkstra, *aestrella, cmp);
                } else {
                    aestrella->buscarEn(grafo, o, d, cmp.aEstrella);
                    dijkstra->buscarEn(grafo, o, d, cmp.dijkstra);
                }
                if (!cmp.mismoCosto()) ++resumen.discrepanciasOptimalidad;  // RF-082
                if (!cmp.aEstrellaNoExpandeMas()) ++resumen.aEstrellaExpandioMas;
                if (!cmp.dijkstra.encontrada) ++resumen.consultasSinRuta;

                for (const auto* r : {&cmp.dijkstra, &cmp.aEstrella}) {
                    escribirFila({navegacion::nombreAlgoritmo(r->algoritmo), mapa, tam, cfg.densidad, semilla, i, rep,
                                  grafo.celda(o), grafo.celda(d), r->encontrada, r->costo, r->ruta.size(),
                                  r->expandidos, r->generados, r->maxAbierta, r->tiempoUs, r->memoriaBytes, 0, r->costo,
                                  0},
                                 csv);
                    ++resumen.mediciones;
                }
            }
        }
    }
    return resumen;
}

ResumenDinamico ejecutarBenchmarkDinamico(const ConfigBenchmark& cfg, const std::vector<std::string>& metadatos,
                                          std::ostream& csv, const FabricaBuscador& fabrica) {
    ResumenDinamico resumen;
    escribirCabecera(cfg, metadatos, "dinamico", csv);

    auto eventos = cfg.dinamico.eventos;
    std::stable_sort(eventos.begin(), eventos.end(),
                     [](const EventoDinamico& a, const EventoDinamico& b) { return a.tick < b.tick; });
    // Solo se usan pares con ruta lo bastante larga para que el último evento ocurra antes de llegar.
    int distanciaMinima = 2;
    for (const auto& e : eventos) distanciaMinima = std::max(distanciaMinima, e.tick + e.adelante + 2);

    const auto crear = fabricaPorDefecto(fabrica);
    std::unique_ptr<IBuscadorRuta> buscadores[2] = {crear(Algoritmo::Dijkstra), crear(Algoritmo::AEstrella)};
    ResultadoBusqueda r;
    std::vector<mundo::Nodo> ruta;

    for (const int tam : cfg.dinamico.tamanos) {
        const std::uint64_t semilla = semillaMapa(cfg.semilla, tam) ^ 0xD1A0ULL;
        nucleo::Pcg32 rng(semilla, 2u);  // flujo propio del escenario dinámico (RF-086)
        const auto grafo = generarMapa({tam, tam, cfg.densidad, cfg.terrenoCaro}, rng);
        std::vector<std::pair<mundo::Nodo, mundo::Nodo>> pares;
        navegacion::BuscadorMejorPrimero filtro(Algoritmo::AEstrella);
        for (const auto& par : generarPares(grafo, cfg.pares * 20, rng)) {
            const auto a = grafo.celda(par.first);
            const auto b = grafo.celda(par.second);
            if (std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)) < distanciaMinima) continue;
            filtro.buscarEn(grafo, par.first, par.second, r);
            if (!r.encontrada) continue;  // sin ruta desde el principio: no mide nada del cambio
            pares.push_back(par);
            if (static_cast<int>(pares.size()) == cfg.pares) break;
        }
        const std::string mapa = cfg.mapa + "_" + std::to_string(tam) + "_dinamico";

        for (std::size_t i = 0; i < pares.size(); ++i) {
            const auto [o, d] = pares[i];
            for (int rep = 0; rep < cfg.repeticiones; ++rep) {
                for (int k = 0; k < 2; ++k) {
                    const int alg = rep % 2 == 0 ? k : 1 - k;  // orden alterno, como en el estático
                    auto& b = *buscadores[alg];
                    const Recorrido rc = recorrer(grafo, o, d, eventos, b, r, ruta);
                    escribirFila({navegacion::nombreAlgoritmo(b.algoritmo()), mapa, tam, cfg.densidad, semilla, i, rep,
                                  grafo.celda(o), grafo.celda(d), rc.llego, rc.costo, rc.celdas, rc.expandidos,
                                  rc.generados, rc.maxAbierta, rc.tiempoUs, rc.memoria, rc.replanificaciones,
                                  rc.costoInicial, rc.eventos},
                                 csv);
                    ++resumen.filas;
                    if (rep == 0) {
                        ++resumen.ejecuciones;
                        resumen.replanificaciones[alg] += static_cast<std::uint64_t>(rc.replanificaciones);
                        if (rc.replanificaciones == 0) ++resumen.sinReplanificar[alg];
                        if (!rc.llego) ++resumen.noLlegaron;
                    }
                }
            }
        }
    }
    return resumen;
}

ConfigBenchmark leerConfigBenchmark(std::string_view json) {
    const ValorJson raiz = nucleo::leerJson(json);
    soloClaves(raiz, "config",
               {"tamanos", "pares", "repeticiones", "densidad", "terreno_caro", "semilla", "mapa", "salida", "dinamico",
                "descripcion"});
    ConfigBenchmark c;
    if (const auto* v = raiz.buscar("tamanos")) c.tamanos = listaTamanos(*v, "tamanos");
    if (const auto* v = raiz.buscar("pares")) c.pares = static_cast<int>(v->comoEntero("pares", 1, 100000));
    if (const auto* v = raiz.buscar("repeticiones"))
        c.repeticiones = static_cast<int>(v->comoEntero("repeticiones", 1, 1000));
    if (const auto* v = raiz.buscar("densidad")) c.densidad = numeroEn(*v, "densidad", 0.0, 0.9);
    if (const auto* v = raiz.buscar("terreno_caro")) c.terrenoCaro = numeroEn(*v, "terreno_caro", 0.0, 0.9);
    if (const auto* v = raiz.buscar("semilla"))
        c.semilla = static_cast<std::uint64_t>(v->comoEntero("semilla", 0, 9007199254740991LL));
    if (const auto* v = raiz.buscar("mapa")) {
        c.mapa = v->comoCadena("mapa");
        if (c.mapa.empty() || c.mapa.find_first_of(",\n\r\"") != std::string::npos)
            throw ErrorJson("mapa: no puede estar vacio ni contener comas, comillas o saltos de linea");
    }
    if (const auto* v = raiz.buscar("salida")) c.salida = v->comoCadena("salida");
    if (const auto* din = raiz.buscar("dinamico")) {
        soloClaves(*din, "dinamico", {"activo", "tamanos", "eventos"});
        c.dinamico.activo = true;
        if (const auto* v = din->buscar("activo")) c.dinamico.activo = v->comoBool("dinamico.activo");
        if (const auto* v = din->buscar("tamanos")) c.dinamico.tamanos = listaTamanos(*v, "dinamico.tamanos");
        if (const auto* v = din->buscar("eventos")) {
            c.dinamico.eventos.clear();
            const auto& lista = v->comoArreglo("dinamico.eventos");
            for (std::size_t i = 0; i < lista.size(); ++i) {
                const std::string ctx = "dinamico.eventos[" + std::to_string(i) + "]";
                soloClaves(lista[i], ctx, {"tick", "x", "y", "adelante", "multiplicador"});
                EventoDinamico e;
                if (const auto* t = lista[i].buscar("tick"))
                    e.tick = static_cast<int>(t->comoEntero(ctx + ".tick", 1, 1000000));
                const auto* x = lista[i].buscar("x");
                const auto* y = lista[i].buscar("y");
                if ((x == nullptr) != (y == nullptr)) throw ErrorJson(ctx + ": x e y van juntos");
                if (x != nullptr) {
                    e.x = static_cast<int>(x->comoEntero(ctx + ".x", 0, 4095));
                    e.y = static_cast<int>(y->comoEntero(ctx + ".y", 0, 4095));
                }
                if (const auto* a = lista[i].buscar("adelante"))
                    e.adelante = static_cast<int>(a->comoEntero(ctx + ".adelante", 1, 1000));
                if (const auto* m = lista[i].buscar("multiplicador"))
                    e.multiplicador = static_cast<std::uint8_t>(m->comoEntero(ctx + ".multiplicador", 0, 255));
                c.dinamico.eventos.push_back(e);
            }
            if (c.dinamico.eventos.empty()) throw ErrorJson("dinamico.eventos: debe tener al menos un evento");
        }
    }
    return c;
}

}  // namespace refugio::experimentos
