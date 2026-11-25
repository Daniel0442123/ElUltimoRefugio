#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>

#include "contador_asignaciones.hpp"
#include "dominio/coordinacion/AsignadorAccesos.hpp"
#include "dominio/coordinacion/ContractNet.hpp"
#include "dominio/juego/Partida.hpp"
#include "dominio/navegacion/Steering.hpp"

using namespace refugio;
using coordinacion::TipoTarea;
using juego::Partida;
using E = agentes::EstadoInfectado;
using mundo::Celda;

namespace {
juego::ConfigPartida configPrueba() {
    juego::ConfigPartida c;
    c.visionAngulo = 360.0f;
    c.danioPorSegundo = 0.0f;
    return c;
}
float largo(navegacion::Vec2 v) { return std::hypot(v.x, v.y); }
}  // namespace

// ------------------------------------------------------------------------------- RF-049
TEST_CASE("CP-049 Contract Net adjudica cada tarea a la puja mas baja y una por agente", "[RF-049][HU-10]") {
    // Mara en (20,10). Tareas: interceptar en (26,10), flanquear en (20,4) y (20,16), perseguir (20,10).
    const std::vector<coordinacion::Tarea> tareas{{TipoTarea::Interceptar, {26, 10}},
                                                  {TipoTarea::Flanquear, {20, 4}},
                                                  {TipoTarea::Flanquear, {20, 16}},
                                                  {TipoTarea::Perseguir, {20, 10}}};
    // Seis alertados con costos octiles conocidos hacia cada tarea.
    const std::vector<coordinacion::Postor> postores{{0, {27, 11}}, {1, {25, 10}}, {2, {20, 2}},
                                                     {3, {21, 17}}, {4, {14, 10}}, {5, {10, 10}}};
    coordinacion::ResultadoContractNet r;
    coordinacion::ejecutarContractNet(tareas, postores, r);

    REQUIRE(r.adjudicaciones.size() == 4);
    // Interceptar: 1 puja 1 y 0 puja 1+0,414 -> gana 1.
    CHECK(r.adjudicaciones[0].agente == 1);
    CHECK(r.adjudicaciones[0].costo == coordinacion::costoPuja({25, 10}, {26, 10}));
    CHECK(r.adjudicaciones[1].agente == 2);  // (20,2) está a 2 de (20,4)
    CHECK(r.adjudicaciones[2].agente == 3);  // (21,17) a 1,414 de (20,16)
    CHECK(r.adjudicaciones[3].agente == 4);  // de los libres, 4 está a 6 y 5 a 10

    // Cada adjudicación es la puja mínima entre los postores que aún estaban libres.
    std::set<std::uint32_t> ocupados;
    for (const auto& a : r.adjudicaciones) {
        for (const auto& p : postores)
            if (!ocupados.count(p.agente)) CHECK(a.costo <= coordinacion::costoPuja(p.celda, tareas[a.tarea].destino));
        CHECK(ocupados.insert(a.agente).second);  // una tarea por agente
    }
    // Mensajes: un anuncio y una puja por postor y tarea.
    CHECK(r.anuncios == 24);
    CHECK(r.pujas == 24);

    // Empate exacto: gana el agente de menor id; con menos postores que tareas quedan sin adjudicar.
    const std::vector<coordinacion::Postor> empatados{{7, {20, 12}}, {3, {20, 8}}};
    coordinacion::ejecutarContractNet({{TipoTarea::Perseguir, {20, 10}}, {TipoTarea::Interceptar, {0, 0}},
                                       {TipoTarea::Flanquear, {1, 1}}},
                                      empatados, r);
    REQUIRE(r.adjudicaciones.size() == 2);
    CHECK(r.adjudicaciones[0].agente == 3);
    CHECK(r.adjudicaciones[1].agente == 7);
}

TEST_CASE("CP-049 En la partida el que ve a Mara reparte interceptar, flanquear y perseguir", "[RF-049][RF-053][HU-12]") {
    // Mara corre hacia el este por una calle abierta; A la ve y los otros están lejos, sin verla.
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(40, 20);
    e.inicioMara = {14.5f, 10.5f};
    e.radio = {39, 19};
    e.infectados = {{10.5f, 10.5f}, {6.5f, 10.5f}, {10.5f, 6.5f}, {10.5f, 14.5f}, {6.5f, 6.5f}};
    auto cfg = configPrueba();
    cfg.visionRadio = 5.0f;  // solo A (a 4) la ve
    cfg.alertas.radioComunicacion = 6.0f;
    cfg.periodoAlerta = 0.5f;
    Partida p(std::move(e), cfg, 3);
    const juego::EntradaJugador este{1.0f, 0.0f, juego::ModoMovimiento::Sigilo, false};
    std::uint64_t vistas = 0;
    bool intercepta = false;
    bool alguienIntercepta = false;
    for (int i = 0; i < 3 * 60; ++i) {
        p.paso(este);
        for (const auto& z : p.infectados())
            if (z.estado == E::Interceptar && z.tieneTarea && z.tarea != TipoTarea::Perseguir &&
                z.objetivo == z.destinoTarea)
                alguienIntercepta = true;
        if (p.subastas() == vistas) continue;
        vistas = p.subastas();
        // Cada subasta: Perseguir siempre al final; cada adjudicación a un agente distinto.
        const auto& tareas = p.ultimasTareas();
        REQUIRE_FALSE(tareas.empty());
        CHECK(tareas.back().tipo == TipoTarea::Perseguir);
        const auto& r = p.ultimoContractNet();
        const std::size_t postores = r.anuncios / tareas.size();
        CHECK(r.adjudicaciones.size() == std::min(tareas.size(), postores));
        std::set<std::uint32_t> ganadores;
        for (const auto& a : r.adjudicaciones) CHECK(ganadores.insert(a.agente).second);
        // Con dos avistamientos el gestor prevé el rumbo: interceptar va por delante de Mara.
        if (tareas.front().tipo == TipoTarea::Interceptar) {
            intercepta = true;
            CHECK(tareas.front().destino.x > agentes::celdaDe(p.mara().pos).x);
        }
    }
    CHECK(vistas >= 2);
    CHECK(intercepta);
    CHECK(alguienIntercepta);  // el adjudicado entra en Interceptar con el destino de su tarea
    CHECK(p.mensajesContractNet() > 0);
}

// ------------------------------------------------------------------------------- RF-051
TEST_CASE("CP-051 El reparto de accesos respeta el maximo por celda y minimiza distancias", "[RF-051][HU-10]") {
    mundo::GrafoRejilla g(20, 20);
    g.bloquear(11, 10);  // un acceso menos: quedan 7
    const Celda objetivo{10, 10};
    std::vector<std::uint32_t> ids;
    std::vector<Celda> celdas;
    for (std::uint32_t k = 0; k < 16; ++k) {
        ids.push_back(100 + k);
        celdas.push_back({static_cast<int>(k % 4) * 6, static_cast<int>(k / 4) * 6});
    }
    coordinacion::AsignadorAccesos a;
    a.reservar(ids.size());
    std::vector<coordinacion::Asignacion> s;
    a.asignar(g, objetivo, ids, celdas, 2, s);
    REQUIRE(s.size() == ids.size());
    int asignados = 0;
    std::vector<int> cuenta(9, 0);
    for (std::size_t k = 0; k < s.size(); ++k) {
        CHECK(s[k].agente == ids[k]);
        if (!s[k].asignado) continue;
        ++asignados;
        const int dx = s[k].acceso.x - objetivo.x;
        const int dy = s[k].acceso.y - objetivo.y;
        CHECK(std::max(std::abs(dx), std::abs(dy)) == 1);
        CHECK(g.transitable(s[k].acceso.x, s[k].acceso.y));
        ++cuenta[static_cast<std::size_t>((dy + 1) * 3 + dx + 1)];
    }
    CHECK(asignados == 14);  // 7 accesos x 2
    for (const int c : cuenta) CHECK(c <= 2);

    // Sin competencia cada agente toma su acceso más cercano.
    a.asignar(g, objetivo, {1, 2}, {{0, 10}, {10, 0}}, 2, s);
    CHECK(s[0].acceso == Celda{9, 10});
    CHECK(s[1].acceso == Celda{10, 9});
}

TEST_CASE("CP-051 Doce perseguidores durante 10 s: max 2 por celda de llegada y 3 o mas accesos", "[RF-051][RNF-030][HU-10]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(30, 30);
    e.inicioMara = {15.5f, 15.5f};
    e.radio = {29, 29};
    for (int k = 0; k < 12; ++k) {
        const float a = static_cast<float>(k) * 0.5236f;  // alrededor de Mara, a 5 celdas
        e.infectados.push_back({15.5f + 5.0f * std::cos(a), 15.5f + 5.0f * std::sin(a)});
    }
    auto cfg = configPrueba();
    Partida p(std::move(e), cfg, 11);
    int maxPorCelda = 0;
    std::size_t maxAccesos = 0;
    for (int i = 0; i < 10 * 60; ++i) {
        p.paso({});
        std::vector<int> cuenta(9, 0);
        const Celda cm = agentes::celdaDe(p.mara().pos);
        for (const auto& z : p.infectados()) {
            if (!z.tieneLlegada) continue;
            const int dx = z.llegada.x - cm.x;
            const int dy = z.llegada.y - cm.y;
            REQUIRE(std::max(std::abs(dx), std::abs(dy)) == 1);
            ++cuenta[static_cast<std::size_t>((dy + 1) * 3 + dx + 1)];
        }
        maxPorCelda = std::max(maxPorCelda, *std::max_element(cuenta.begin(), cuenta.end()));
        maxAccesos = std::max(maxAccesos, static_cast<std::size_t>(std::count_if(cuenta.begin(), cuenta.end(), [](int c) { return c > 0; })));
    }
    CHECK(maxPorCelda <= 2);
    CHECK(maxAccesos >= 3);
    // Al final la rodean sin amontonarse: nadie a menos de 0,35 de otro y casi todos atacan.
    int atacando = 0;
    const auto& zs = p.infectados();
    for (std::size_t i = 0; i < zs.size(); ++i) {
        if (zs[i].estado == E::Atacar) ++atacando;
        for (std::size_t j = i + 1; j < zs.size(); ++j) CHECK(largo({zs[i].pos.x - zs[j].pos.x, zs[i].pos.y - zs[j].pos.y}) >= 0.35f);
    }
    CHECK(atacando >= 8);
}

// ------------------------------------------------------------------------------- RF-029
TEST_CASE("CP-029 Steering: dos agentes que se cruzan no se solapan y no pasan de la velocidad maxima", "[RF-029][HU-10]") {
    const mundo::GrafoRejilla g(12, 12);
    const navegacion::ParametrosSteering par{};
    const float velMax = 3.0f;
    const float dt = 1.0f / 60.0f;
    navegacion::Movil m[2] = {{{2.0f, 6.0f}, {velMax, 0.0f}}, {{6.0f, 2.0f}, {0.0f, velMax}}};
    const navegacion::Vec2 metas[2] = {{10.0f, 6.0f}, {6.0f, 10.0f}};
    std::vector<std::uint32_t> vecinos{0, 1};
    float minimo = 1e9f;
    float velMaxObservada = 0.0f;
    for (int i = 0; i < 4 * 60; ++i) {
        navegacion::Vec2 f[2];
        for (std::uint32_t k = 0; k < 2; ++k) {
            const navegacion::Vec2 d{metas[k].x - m[k].pos.x, metas[k].y - m[k].pos.y};
            const float l = largo(d);
            const navegacion::Vec2 deseada = l > 1e-4f ? navegacion::Vec2{d.x / l * velMax, d.y / l * velMax} : navegacion::Vec2{};
            f[k] = navegacion::fuerzaSteering(k, m, vecinos, deseada, g, 0.3f, par);
            CHECK(largo(f[k]) <= par.aceleracionMax + 1e-3f);
        }
        for (int k = 0; k < 2; ++k) {
            m[k].vel = navegacion::integrarVelocidad(m[k].vel, f[k], velMax, dt);
            m[k].pos.x += m[k].vel.x * dt;
            m[k].pos.y += m[k].vel.y * dt;
            velMaxObservada = std::max(velMaxObservada, largo(m[k].vel));
        }
        minimo = std::min(minimo, largo({m[0].pos.x - m[1].pos.x, m[0].pos.y - m[1].pos.y}));
    }
    CHECK(minimo >= 0.6f);  // dos radios de cuerpo: no se solapan
    CHECK(velMaxObservada <= velMax + 1e-4f);
    // Y los dos acaban llegando a su meta.
    CHECK(largo({m[0].pos.x - metas[0].x, m[0].pos.y - metas[0].y}) < 0.5f);
    CHECK(largo({m[1].pos.x - metas[1].x, m[1].pos.y - metas[1].y}) < 0.5f);
}

TEST_CASE("CP-029 La evitacion aleja de las paredes cercanas", "[RF-029]") {
    mundo::GrafoRejilla g(10, 10);
    g.bloquear(5, 5);
    const navegacion::ParametrosSteering par{};
    navegacion::Movil m[1] = {{{4.75f, 5.5f}, {0.0f, 0.0f}}};  // a 0,25 del bloque por su izquierda
    const auto f = navegacion::fuerzaSteering(0, m, {0}, {0.0f, 0.0f}, g, 0.3f, par);
    CHECK(f.x < 0.0f);
    CHECK(std::abs(f.y) < 1e-4f);
}

TEST_CASE("CP-106 Contract Net, reparto de accesos y steering no asignan memoria en el bucle", "[RNF-006][RF-049][RF-051][RF-029]") {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(40, 20);
    for (int y = 0; y < 14; ++y) e.mapa.bloquear(26, y);
    e.inicioMara = {33.5f, 8.5f};
    e.radio = {39, 19};
    for (int k = 0; k < 12; ++k) e.infectados.push_back({20.5f + static_cast<float>(k % 4) * 1.5f, 3.5f + static_cast<float>(k / 4) * 5.0f});
    auto cfg = configPrueba();
    cfg.visionRadio = 14.0f;
    cfg.periodoAlerta = 0.5f;
    Partida p(std::move(e), cfg, 5);
    auto entrada = [](int i) {
        const float a = static_cast<float>(i) * 0.03f;
        return juego::EntradaJugador{std::cos(a), std::sin(a), juego::ModoMovimiento::Correr, false};
    };
    for (int i = 0; i < 30; ++i) p.paso(entrada(i));
    const auto subastasAntes = p.subastas();
    std::uint64_t asignaciones = 0;
    {
        refugio::pruebas::VentanaAsignaciones ventana;
        for (int i = 30; i < 30 + 20 * 60; ++i) p.paso(entrada(i));
        asignaciones = ventana.asignaciones();
    }
    CHECK(p.subastas() > subastasAntes);
    CHECK(asignaciones == 0);
}
