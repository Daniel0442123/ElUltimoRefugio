// Rutas para la horda sin congelar el juego (HU-23): RF-026, RF-027, RF-028, RNF-002, RNF-006, ADR-0007.
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "contador_asignaciones.hpp"
#include "dominio/experimentos/GeneradorMapas.hpp"
#include "dominio/juego/Partida.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"
#include "dominio/navegacion/PlanificadorRutas.hpp"
#include "dominio/nucleo/Pcg32.hpp"

using namespace refugio;
using mundo::Nodo;
using navegacion::Algoritmo;
using navegacion::EstadoBusqueda;
using navegacion::PlanificadorRutas;
using navegacion::ResultadoBusqueda;

// El límite de 2 ms por tick (HU-23) solo se comprueba en Release sin sanitizadores: en Debug o
// con ASan el mismo trabajo tarda varias veces más y la medida no dice nada del juego.
#if defined(NDEBUG) && !defined(__SANITIZE_ADDRESS__)
#if defined(__has_feature)
#if !__has_feature(address_sanitizer)
#define REFUGIO_MEDIR_TIEMPO 1
#endif
#else
#define REFUGIO_MEDIR_TIEMPO 1
#endif
#endif

namespace {

mundo::GrafoRejilla ciudad(int lado, std::uint64_t semilla) {
    nucleo::Pcg32 rng(semilla, 3u);
    return experimentos::generarMapa({lado, lado, 0.25, 0.10}, rng);
}

/// Una solicitud de la prueba: origen y destino fijos por agente.
struct Pedido {
    Nodo origen;
    Nodo destino;
};

/// @p n pedidos con el destino a @p radio celdas o menos del origen (en cada eje) y alcanzable:
/// los infectados que oyen un ruido o reciben una alerta van a un punto de la zona (STB Escena 3).
std::vector<Pedido> pedidosCercanos(const mundo::GrafoRejilla& g, int n, int radio, std::uint64_t semilla) {
    nucleo::Pcg32 rng(semilla, 5u);
    navegacion::BuscadorMejorPrimero b(Algoritmo::AEstrella);
    std::vector<Pedido> v;
    while (static_cast<int>(v.size()) < n) {
        const Nodo o = rng.enteroMenorQue(g.numCeldas());
        if (!g.transitable(o)) continue;
        const auto c = g.celda(o);
        const auto lado = static_cast<std::uint32_t>(2 * radio + 1);
        const int x = c.x + static_cast<int>(rng.enteroMenorQue(lado)) - radio;
        const int y = c.y + static_cast<int>(rng.enteroMenorQue(lado)) - radio;
        if (x < 0 || y < 0 || x >= g.ancho() || y >= g.alto() || !g.transitable(x, y)) continue;
        const Nodo d = g.nodo(x, y);
        if (d == o || !b.buscar(g, o, d).encontrada) continue;
        v.push_back({o, d});
    }
    return v;
}

}  // namespace

TEST_CASE("RF-026 La busqueda reanudable da el mismo resultado que la busqueda de una vez", "[RF-026][RF-021][HU-23]") {
    const auto g = ciudad(64, 11);
    nucleo::Pcg32 rng(17, 1u);
    const auto pares = experimentos::generarPares(g, 40, rng);
    for (const auto alg : {Algoritmo::AEstrella, Algoritmo::Dijkstra}) {
        navegacion::BuscadorMejorPrimero completo(alg);
        navegacion::BuscadorMejorPrimero partido(alg);
        for (const auto& [o, d] : pares) {
            const auto r = completo.buscar(g, o, d, true);
            for (const std::uint32_t trozo : {1u, 7u, 100u, 4000u}) {
                ResultadoBusqueda p;
                REQUIRE(partido.iniciar(g, o, d, p, true) == EstadoBusqueda::EnCurso);
                int llamadas = 0;
                EstadoBusqueda e = EstadoBusqueda::EnCurso;
                while (e == EstadoBusqueda::EnCurso) {
                    const auto antes = p.expandidos;
                    e = partido.avanzar(g, p, trozo);
                    REQUIRE(p.expandidos - antes <= trozo);  // nunca más del presupuesto
                    ++llamadas;
                }
                CHECK(e == (r.encontrada ? EstadoBusqueda::Encontrada : EstadoBusqueda::SinRuta));
                CHECK(p.encontrada == r.encontrada);
                CHECK(p.costo == r.costo);
                CHECK(p.ruta == r.ruta);
                CHECK(p.expandidos == r.expandidos);
                CHECK(p.generados == r.generados);
                CHECK(p.maxAbierta == r.maxAbierta);
                CHECK(p.ordenExpansion == r.ordenExpansion);
                CHECK(llamadas >= static_cast<int>((r.expandidos + trozo - 1) / trozo));
                CHECK(partido.avanzar(g, p, trozo) == e);  // terminada: no cambia nada más
                CHECK(p.expandidos == r.expandidos);
            }
        }
    }
}

TEST_CASE("CP-027 La busqueda reanudable termina en SinRuta sin bloquear", "[RF-027][HU-23]") {
    mundo::GrafoRejilla g(40, 40);
    for (int i = 18; i <= 22; ++i) {  // destino (20,20) encerrado
        g.bloquear(i, 18);
        g.bloquear(i, 22);
        g.bloquear(18, i);
        g.bloquear(22, i);
    }
    navegacion::BuscadorMejorPrimero b(Algoritmo::AEstrella);
    ResultadoBusqueda r;
    REQUIRE(b.iniciar(g, g.nodo(1, 1), g.nodo(20, 20), r) == EstadoBusqueda::EnCurso);
    int llamadas = 0;
    while (b.avanzar(g, r, 200) == EstadoBusqueda::EnCurso) ++llamadas;
    CHECK(b.estado() == EstadoBusqueda::SinRuta);
    CHECK_FALSE(r.encontrada);
    CHECK(r.ruta.empty());
    CHECK(llamadas >= 7);  // ~1575 celdas alcanzables: varios trozos de 200, ninguno mayor
    CHECK(r.expandidos == 40 * 40 - 16 - 9);

    // Origen o destino no transitables: SinRuta al iniciar, sin expandir nada.
    CHECK(b.iniciar(g, g.nodo(1, 1), g.nodo(18, 18), r) == EstadoBusqueda::SinRuta);
    CHECK(r.expandidos == 0);
    CHECK(b.avanzar(g, r, 100) == EstadoBusqueda::SinRuta);
}

TEST_CASE("CP-026 100 solicitudes en el mismo tick: 4000 nodos por tick y 10 ticks como maximo", "[RF-026][RNF-002][HU-23]") {
    const auto g = ciudad(128, 26);
    // Destinos a 40 celdas o menos (un tercio del mapa): el trabajo ocupa varios ticks.
    const auto pedidos = pedidosCercanos(g, 100, 40, 26);
    const std::uint32_t presupuesto = 4000;  // PRESUPUESTO_NODOS_TICK

    // Referencia: cada búsqueda hecha de una vez.
    navegacion::BuscadorMejorPrimero ref(Algoritmo::AEstrella);
    std::vector<ResultadoBusqueda> esperado(pedidos.size());
    std::uint64_t totalNodos = 0;
    for (std::size_t i = 0; i < pedidos.size(); ++i) {
        ref.buscarEn(g, pedidos[i].origen, pedidos[i].destino, esperado[i]);
        totalNodos += esperado[i].expandidos;
    }

    PlanificadorRutas plan;
    plan.reservar(static_cast<std::uint32_t>(pedidos.size()), g.numCeldas());
    const auto consulta = [&](std::uint32_t a) { return std::pair<Nodo, Nodo>{pedidos[a].origen, pedidos[a].destino}; };
    // Calentamiento: una primera ronda toca por primera vez la memoria reservada (fallos de página
    // del sistema), como pasa en los primeros segundos de partida. Se mide la segunda.
    for (std::uint32_t i = 0; i < pedidos.size(); ++i) plan.solicitar(i, 1);
    for (int t = 0; t < 50 && plan.pendientes() + (plan.agenteEnCurso() != PlanificadorRutas::kSinAgente) > 0; ++t)
        plan.procesar(g, presupuesto, consulta, [](std::uint32_t, const ResultadoBusqueda&) {});
    for (std::uint32_t i = 0; i < pedidos.size(); ++i) plan.solicitar(i, 1);
    REQUIRE(plan.pendientes() == 100);

    std::vector<int> tickEntrega(pedidos.size(), -1);
    std::vector<char> igual(pedidos.size(), 0);
    std::vector<std::uint32_t> nodosPorTick;
    nodosPorTick.reserve(64);
    int ticks = 0;
    double peorTiempoUs = 0.0;
    std::uint64_t nodos = 0;
    std::uint64_t asignaciones = 0;
    {
        const pruebas::VentanaAsignaciones ventana;  // RNF-006: la cola no asigna memoria al procesar
        while ((plan.pendientes() > 0 || plan.agenteEnCurso() != PlanificadorRutas::kSinAgente) && ticks < 50) {
            ++ticks;
            plan.procesar(
                g, presupuesto, consulta,
                [&](std::uint32_t a, const ResultadoBusqueda& r) {
                    tickEntrega[a] = ticks;
                    igual[a] = r.encontrada && r.costo == esperado[a].costo && r.ruta == esperado[a].ruta ? 1 : 0;
                });
            const auto& m = plan.metricas();
            nodosPorTick.push_back(m.expandidos);  // cabe: capacidad reservada
            nodos += m.expandidos;
            if (m.tiempoUs > peorTiempoUs) peorTiempoUs = m.tiempoUs;
        }
        asignaciones = ventana.asignaciones();
    }
    CHECK(asignaciones == 0);
    for (const auto n : nodosPorTick) CHECK(n <= presupuesto);
    INFO("nodos totales " << totalNodos << ", ticks " << ticks << ", peor tick " << peorTiempoUs << " us");
    CHECK(nodos == totalNodos);  // ningún nodo se expande dos veces
    CHECK(ticks >= 3);            // el trabajo no cabe en un tick: el presupuesto lo reparte
    CHECK(ticks <= 10);           // todas resueltas en 10 ticks de IA o menos
    CHECK(ticks == static_cast<int>((totalNodos + presupuesto - 1) / presupuesto));  // presupuesto aprovechado
    for (std::size_t i = 0; i < pedidos.size(); ++i) {
        CHECK(tickEntrega[i] >= 1);
        CHECK(igual[i] == 1);  // misma ruta óptima que sin presupuesto
    }
    CHECK(plan.metricas().esperaMaxima <= 10);
#ifdef REFUGIO_MEDIR_TIEMPO
    CHECK(peorTiempoUs <= 2000.0);  // HU-23: el tiempo de IA por tick no supera 2 ms
#endif
}

TEST_CASE("RF-026 La cola atiende por prioridad y antiguedad sin dejar a nadie esperando siempre", "[RF-026][HU-23]") {
    mundo::GrafoRejilla g(30, 30);
    const auto consulta = [&g](std::uint32_t a) {
        return std::pair<Nodo, Nodo>{g.nodo(0, static_cast<int>(a % 30)), g.nodo(29, static_cast<int>(a % 30))};
    };
    PlanificadorRutas plan;
    plan.reservar(64, g.numCeldas());
    std::vector<std::uint32_t> orden;
    const auto anotar = [&orden](std::uint32_t a, const ResultadoBusqueda& r) {
        CHECK(r.encontrada);
        orden.push_back(a);
    };
    // Una ruta recta de 30 celdas expande 30 nodos con A*: presupuesto para una por tick.
    plan.solicitar(0, 0);  // explorar, la más antigua
    plan.procesar(g, 0, consulta, anotar);  // tick sin presupuesto: nadie avanza
    CHECK(orden.empty());
    plan.solicitar(1, 2);  // perseguir
    plan.solicitar(2, 1);  // investigar
    plan.solicitar(3, 2);  // perseguir, después que la 1
    plan.procesar(g, 30, consulta, anotar);
    plan.procesar(g, 30, consulta, anotar);
    plan.procesar(g, 30, consulta, anotar);
    plan.procesar(g, 30, consulta, anotar);
    // 1 y 3 (prioridad 2; con la misma antigüedad, el agente menor), luego la 2 (prioridad 1) y
    // al final la 0: aunque esperó un tick más, en el tick 4 su clave es 0·5 + 4 = 4 frente a
    // 1·5 + 3 = 8 de la 2.
    CHECK(orden == std::vector<std::uint32_t>{1, 3, 2, 0});

    // Envejecimiento: una petición de prioridad 0 adelanta a las urgentes nuevas tras esperar.
    orden.clear();
    plan.solicitar(10, 0);
    for (int t = 0; t < 12; ++t) {
        plan.solicitar(static_cast<std::uint32_t>(20 + t), 2);  // llega una urgente nueva cada tick
        plan.procesar(g, 30, consulta, anotar);
    }
    bool atendida = false;
    std::size_t posicion = 0;
    for (std::size_t k = 0; k < orden.size(); ++k)
        if (orden[k] == 10) {
            atendida = true;
            posicion = k;
        }
    CHECK(atendida);
    CHECK(posicion <= 11);

    // Pedir otra vez no la duplica ni le quita antigüedad; cancelar la saca de la cola.
    plan.vaciar();
    orden.clear();
    plan.solicitar(5, 0);
    plan.solicitar(5, 0);
    CHECK(plan.pendientes() == 1);
    plan.solicitar(6, 0);
    plan.cancelar(5);
    CHECK_FALSE(plan.solicitado(5));
    CHECK(plan.solicitado(6));
    plan.procesar(g, 1000, consulta, anotar);
    CHECK(orden == std::vector<std::uint32_t>{6});
}

TEST_CASE("RF-025 Una busqueda a medias vuelve a empezar si cambia el mapa o el algoritmo", "[RF-026][RF-025][HU-23]") {
    mundo::GrafoRejilla g(60, 60);
    const auto consulta = [&g](std::uint32_t) { return std::pair<Nodo, Nodo>{g.nodo(0, 30), g.nodo(59, 30)}; };
    ResultadoBusqueda entregado;
    int entregas = 0;
    const auto guardar = [&](std::uint32_t, const ResultadoBusqueda& r) {
        entregado = r;
        ++entregas;
    };
    PlanificadorRutas plan;
    plan.reservar(4, g.numCeldas());
    plan.solicitar(0, 1);
    plan.procesar(g, 20, consulta, guardar);  // a medias: 20 de 60 nodos
    REQUIRE(plan.agenteEnCurso() == 0);
    CHECK(plan.metricas().enCurso);
    CHECK(plan.metricas().expandidos == 20);

    // Una barricada corta la recta: la búsqueda empieza de nuevo y rodea.
    g.bloquear(40, 30);
    plan.reiniciarEnCurso();
    while (plan.agenteEnCurso() != PlanificadorRutas::kSinAgente) plan.procesar(g, 20, consulta, guardar);
    REQUIRE(entregas == 1);
    CHECK(entregado.encontrada);
    const auto directo = navegacion::BuscadorMejorPrimero(Algoritmo::AEstrella).buscar(g, g.nodo(0, 30), g.nodo(59, 30));
    CHECK(entregado.costo == directo.costo);
    CHECK(entregado.ruta == directo.ruta);

    // F2 a mitad de una búsqueda: termina con Dijkstra (mismo costo, más nodos).
    plan.solicitar(1, 1);
    plan.procesar(g, 20, consulta, guardar);
    plan.fijarAlgoritmo(Algoritmo::Dijkstra);
    while (plan.agenteEnCurso() != PlanificadorRutas::kSinAgente) plan.procesar(g, 500, consulta, guardar);
    REQUIRE(entregas == 2);
    CHECK(entregado.algoritmo == Algoritmo::Dijkstra);
    CHECK(entregado.costo == directo.costo);
    CHECK(entregado.expandidos > directo.expandidos);

    // Cancelar la que está en curso: no se entrega nada.
    plan.solicitar(2, 1);
    plan.procesar(g, 10, consulta, guardar);
    plan.cancelar(2);
    CHECK(plan.agenteEnCurso() == PlanificadorRutas::kSinAgente);
    plan.procesar(g, 5000, consulta, guardar);
    CHECK(entregas == 2);
}

TEST_CASE("CP-026 Horda de 100 infectados en 128x128 sin superar el presupuesto", "[RF-026][RF-030][RNF-002][HU-23]") {
    juego::ConfigPartida cfg;
    cfg.presupuestoNodosTick = 4000;
    cfg.danioPorSegundo = 0.0f;  // Mara aguanta toda la prueba
    juego::Partida p(juego::crearEscenarioAleatorio(128, 128, 100, 33), cfg, 33);
    // Mara corre en zigzag: hace ruido, la ven y la horda pide rutas en masa.
    double peorTickUs = 0.0;
    std::uint64_t busquedas = 0;
    for (int t = 0; t < 60 * 60; ++t) {
        juego::EntradaJugador e;
        e.modo = juego::ModoMovimiento::Correr;
        e.ejeX = (t / 90) % 2 == 0 ? 1.0f : -1.0f;
        e.ejeY = (t / 150) % 2 == 0 ? 1.0f : -1.0f;
        p.paso(e);
        if (p.tiempoBusquedaUltimoTickIaUs() > peorTickUs) peorTickUs = p.tiempoBusquedaUltimoTickIaUs();
    }
    busquedas = p.busquedasTotales();
    INFO("busquedas " << busquedas << ", peor tick " << p.maxNodosTickIa() << " nodos / " << peorTickUs
                      << " us, espera maxima " << p.esperaMaximaRuta() << " ticks de IA");
    CHECK(busquedas >= 100);
    CHECK(p.maxNodosTickIa() <= 4000);
    CHECK(p.esperaMaximaRuta() <= 10);
#ifdef REFUGIO_MEDIR_TIEMPO
    CHECK(peorTickUs <= 2000.0);
#endif
}

TEST_CASE("CP-027 Un infectado con el destino encerrado cambia de comportamiento sin bloquear el juego", "[RF-027][HU-23]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(30, 30);
    // Mara dentro de un recinto cerrado: el infectado la oye a través de la pared, pero no hay ruta.
    for (int i = 10; i <= 16; ++i) {
        e.mapa.bloquear(i, 10);
        e.mapa.bloquear(i, 16);
        e.mapa.bloquear(10, i);
        e.mapa.bloquear(16, i);
    }
    e.inicioMara = {13.5f, 13.5f};
    e.radio = {29, 29};
    e.infectados = {{8.5f, 13.5f}};  // a 5 celdas de Mara, al otro lado de la pared
    juego::ConfigPartida cfg;
    cfg.danioPorSegundo = 0.0f;
    cfg.presupuestoNodosTick = 50;  // la búsqueda sin salida ocupa varios ticks
    juego::Partida p(std::move(e), cfg, 3);
    bool sinRuta = false;
    bool cambio = false;
    for (int t = 0; t < 20 * 60 && !(sinRuta && cambio); ++t) {
        juego::EntradaJugador en;
        en.modo = juego::ModoMovimiento::Correr;  // Mara corre en el sitio: ruido de radio 6
        en.ejeX = t % 2 == 0 ? 1.0f : -1.0f;
        p.paso(en);
        const auto& z = p.infectados()[0];
        if (z.busqueda.valida && !z.busqueda.encontrada) sinRuta = true;
        if (sinRuta && z.estado == agentes::EstadoInfectado::Explorar && !(z.objetivo == agentes::celdaDe(p.mara().pos)))
            cambio = true;
        CHECK(p.nodosUltimoTickIa() <= 50);
    }
    CHECK(sinRuta);
    CHECK(cambio);
}
