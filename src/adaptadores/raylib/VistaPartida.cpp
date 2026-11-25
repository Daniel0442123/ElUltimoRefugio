#include "adaptadores/raylib/VistaPartida.hpp"

#include <raylib.h>

#include <array>
#include <cmath>
#include <cstdio>

namespace refugio::adaptadores {
namespace {
using agentes::EstadoInfectado;

Color colorEstado(EstadoInfectado e) {
    switch (e) {
        case EstadoInfectado::Explorar: return {110, 150, 100, 255};   // verde apagado: deambula
        case EstadoInfectado::Investigar: return {230, 200, 60, 255};  // amarillo: oyó algo
        case EstadoInfectado::Perseguir: return {240, 130, 30, 255};   // naranja: la vio
        case EstadoInfectado::Buscar: return {200, 90, 200, 255};      // magenta: la perdió de vista
        case EstadoInfectado::Atacar: return {220, 30, 30, 255};       // rojo
        case EstadoInfectado::Interceptar: return {60, 200, 210, 255};  // turquesa: le cortan el paso
    }
    return WHITE;
}

Vector2 aPantalla(navegacion::Vec2 p, const EstiloVista& e) {
    return {static_cast<float>(e.origenX) + p.x * static_cast<float>(e.tamCelda),
            static_cast<float>(e.origenY) + p.y * static_cast<float>(e.tamCelda)};
}

Vector2 centroCelda(mundo::Celda c, const EstiloVista& e) { return aPantalla(agentes::centroDe(c), e); }

void dibujarObjeto(juego::TipoObjeto t, Vector2 c, float lado) {
    const Rectangle r{c.x - lado / 2, c.y - lado / 2, lado, lado};
    switch (t) {
        case juego::TipoObjeto::Medicamento:  // botiquín: blanco con cruz roja
            DrawRectangleRounded(r, 0.25f, 4, RAYWHITE);
            DrawRectangle(static_cast<int>(c.x - lado * 0.1f), static_cast<int>(c.y - lado * 0.35f),
                          static_cast<int>(lado * 0.2f), static_cast<int>(lado * 0.7f), RED);
            DrawRectangle(static_cast<int>(c.x - lado * 0.35f), static_cast<int>(c.y - lado * 0.1f),
                          static_cast<int>(lado * 0.7f), static_cast<int>(lado * 0.2f), RED);
            break;
        case juego::TipoObjeto::Combustible:  // bidón amarillo
            DrawRectangleRounded({r.x + lado * 0.15f, r.y, lado * 0.7f, lado}, 0.3f, 4, {240, 190, 20, 255});
            DrawRectangleLinesEx({r.x + lado * 0.15f, r.y, lado * 0.7f, lado}, 2.0f, {120, 80, 0, 255});
            break;
        case juego::TipoObjeto::Bateria:  // batería verde con borne
            DrawRectangleRec({r.x, r.y + lado * 0.2f, lado, lado * 0.6f}, {40, 180, 90, 255});
            DrawRectangleRec({r.x + lado * 0.35f, r.y + lado * 0.05f, lado * 0.3f, lado * 0.15f}, DARKGRAY);
            break;
    }
}

const char* nombreObjeto(juego::TipoObjeto t) {
    switch (t) {
        case juego::TipoObjeto::Medicamento: return "Medicamentos";
        case juego::TipoObjeto::Combustible: return "Combustible";
        case juego::TipoObjeto::Bateria: return "Bateria";
    }
    return "?";
}

void barra(int x, int y, int ancho, int alto, float fraccion, Color relleno) {
    DrawRectangle(x, y, ancho, alto, {50, 52, 58, 255});
    DrawRectangle(x, y, static_cast<int>(static_cast<float>(ancho) * std::fmax(0.0f, std::fmin(1.0f, fraccion))), alto,
                  relleno);
    DrawRectangleLines(x, y, ancho, alto, {90, 92, 100, 255});
}
}  // namespace

void dibujarPartida(const juego::Partida& p, const EstiloVista& e, bool depuracion) {
    dibujarGrafo(p.mapa(), e);
    const float tc = static_cast<float>(e.tamCelda);

    // Radio: se ilumina cuando Mara ya tiene los tres objetos.
    const Vector2 cr = centroCelda(p.radio(), e);
    const bool lista = p.tieneTodosLosObjetos();
    if (lista) DrawCircleV(cr, tc * (0.7f + 0.1f * std::sin(static_cast<float>(p.tick()) * 0.1f)), Fade(SKYBLUE, 0.35f));
    DrawRectangleRec({cr.x - tc * 0.35f, cr.y - tc * 0.2f, tc * 0.7f, tc * 0.5f}, {60, 70, 90, 255});
    DrawRectangleLinesEx({cr.x - tc * 0.35f, cr.y - tc * 0.2f, tc * 0.7f, tc * 0.5f}, 2.0f, SKYBLUE);
    DrawLineEx({cr.x + tc * 0.2f, cr.y - tc * 0.2f}, {cr.x + tc * 0.3f, cr.y - tc * 0.45f}, 2.0f, SKYBLUE);

    for (const auto& o : p.objetos())
        if (!o.recogido) dibujarObjeto(o.tipo, centroCelda(o.celda, e), tc * 0.55f);

    if (depuracion && p.campoActivo()) {  // RF-028: flecha de cada celda hacia su siguiente paso
        const auto& g = p.mapa();
        for (mundo::Nodo n = 0; n < g.numCeldas(); ++n) {
            if (!g.transitable(n) || !p.campo().alcanzable(n) || p.campo().siguiente(n) == n) continue;
            const Vector2 a = centroCelda(g.celda(n), e);
            const Vector2 b = centroCelda(g.celda(p.campo().siguiente(n)), e);
            DrawLineEx(a, {a.x + (b.x - a.x) * 0.4f, a.y + (b.y - a.y) * 0.4f}, 2.0f, Fade(DARKBLUE, 0.35f));
            DrawCircleV(a, 1.5f, Fade(DARKBLUE, 0.35f));
        }
    }

    // RF-073: los mensajes de la última alerta se ven 1 s como líneas emisor -> receptor.
    if (p.huboAlerta() && p.tick() - p.tickUltimaAlerta() < 60) {
        const float vida = 1.0f - static_cast<float>(p.tick() - p.tickUltimaAlerta()) / 60.0f;
        for (const auto& m : p.ultimaPropagacion().mensajes) {
            const Vector2 a = aPantalla(p.infectados()[m.emisor].pos, e);
            const Vector2 b = aPantalla(p.infectados()[m.receptor].pos, e);
            DrawLineEx(a, b, 3.0f, Fade(YELLOW, vida * m.confianza));
            DrawCircleLinesV(b, tc * 0.5f, Fade(YELLOW, vida));
        }
    }

    if (depuracion) {
        const auto& cfg = p.config();
        for (const auto& z : p.infectados()) {
            const float angulo = std::atan2(z.orientacion.y, z.orientacion.x) * RAD2DEG;
            DrawCircleSector(aPantalla(z.pos, e), cfg.visionRadio * tc, angulo - cfg.visionAngulo / 2,
                             angulo + cfg.visionAngulo / 2, 24, Fade(colorEstado(z.estado), 0.12f));
            for (std::size_t k = z.indiceRuta; k < z.ruta.size(); ++k) {
                const Vector2 a = k == z.indiceRuta ? aPantalla(z.pos, e) : centroCelda(p.mapa().celda(z.ruta[k - 1]), e);
                DrawLineEx(a, centroCelda(p.mapa().celda(z.ruta[k]), e), 2.0f, Fade(colorEstado(z.estado), 0.8f));
            }
        }
        // RF-049 / RF-051: destino de cada tarea adjudicada y celda de llegada alrededor de Mara.
        for (const auto& z : p.infectados()) {
            if (z.estado == EstadoInfectado::Interceptar && z.tieneTarea) {
                const Vector2 d = centroCelda(z.destinoTarea, e);
                DrawLineEx(aPantalla(z.pos, e), d, 1.5f, Fade(colorEstado(z.estado), 0.6f));
                DrawRectangleLinesEx({d.x - tc * 0.35f, d.y - tc * 0.35f, tc * 0.7f, tc * 0.7f}, 2.0f,
                                     colorEstado(z.estado));
            }
            if (z.tieneLlegada) {
                const Vector2 l = centroCelda(z.llegada, e);
                DrawRectangleLinesEx({l.x - tc * 0.5f, l.y - tc * 0.5f, tc, tc}, 1.0f, Fade(ORANGE, 0.5f));
            }
        }
        if (p.huboRuidoUltimoTickIa())
            DrawCircleLinesV(aPantalla(p.origenUltimoRuido(), e), p.radioUltimoRuido() * tc, Fade(YELLOW, 0.8f));
    }

    for (const auto& z : p.infectados()) {
        const Vector2 c = aPantalla(z.pos, e);
        DrawCircleV(c, tc * 0.32f, colorEstado(z.estado));
        DrawCircleLinesV(c, tc * 0.32f, {30, 30, 30, 255});
        DrawLineEx(c, {c.x + z.orientacion.x * tc * 0.42f, c.y + z.orientacion.y * tc * 0.42f}, 3.0f, {30, 30, 30, 255});
    }

    const auto& m = p.mara();
    const Vector2 cm = aPantalla(m.pos, e);
    DrawCircleV(cm, tc * 0.3f, {60, 140, 255, 255});
    DrawCircleLinesV(cm, tc * 0.3f, RAYWHITE);
    DrawLineEx(cm, {cm.x + m.orientacion.x * tc * 0.4f, cm.y + m.orientacion.y * tc * 0.4f}, 3.0f, RAYWHITE);
    if (m.recibioDanio) DrawCircleLinesV(cm, tc * 0.45f, RED);
}

void dibujarPanelPartida(const juego::Partida& p, int x, int y, int alto, int fps) {
    char buf[128];
    const auto& m = p.mara();
    const auto& cfg = p.config();
    DrawText("El ultimo refugio", x, y, 20, GOLD);
    y += 30;

    std::snprintf(buf, sizeof(buf), "Salud %.0f / %.0f", static_cast<double>(m.salud), static_cast<double>(cfg.saludMax));
    DrawText(buf, x, y, 16, RAYWHITE);
    y += 20;
    barra(x, y, 220, 12, m.salud / cfg.saludMax, m.salud > 30.0f ? Color{60, 190, 90, 255} : RED);
    y += 22;

    const char* modo = m.modo == juego::ModoMovimiento::Sigilo   ? "sigilo (ruido 1)"
                       : m.modo == juego::ModoMovimiento::Correr ? "correr (ruido 6)"
                                                                 : "caminar (ruido 3)";
    std::snprintf(buf, sizeof(buf), "Modo: %s", modo);
    DrawText(buf, x, y, 14, LIGHTGRAY);
    y += 24;

    DrawText("Objetivos", x, y, 18, GOLD);
    y += 22;
    for (int t = 0; t < juego::kNumTiposObjeto; ++t) {
        const bool tiene = m.inventario[static_cast<std::size_t>(t)];
        std::snprintf(buf, sizeof(buf), "[%c] %s", tiene ? 'x' : ' ', nombreObjeto(static_cast<juego::TipoObjeto>(t)));
        DrawText(buf, x, y, 14, tiene ? Color{120, 220, 130, 255} : LIGHTGRAY);
        y += 18;
    }
    DrawText(p.tieneTodosLosObjetos() ? "Ve a la radio y manten E" : "Despues: activar la radio", x, y, 14,
             p.tieneTodosLosObjetos() ? SKYBLUE : GRAY);
    y += 18;
    if (m.progresoInstalacion > 0.0f) {
        barra(x, y, 220, 10, m.progresoInstalacion / cfg.tInstalacion, SKYBLUE);
        y += 14;
    }
    y += 10;

    DrawText("Infectados", x, y, 18, GOLD);
    y += 22;
    std::array<int, agentes::kNumEstadosInfectado> cuenta{};
    for (const auto& z : p.infectados()) ++cuenta[static_cast<std::size_t>(z.estado)];
    for (int s = 0; s < agentes::kNumEstadosInfectado; ++s) {
        const auto est = static_cast<agentes::EstadoInfectado>(s);
        const auto nombre = agentes::nombreEstado(est);
        DrawCircle(x + 6, y + 7, 5, colorEstado(est));
        std::snprintf(buf, sizeof(buf), "%.*s: %d", static_cast<int>(nombre.size()), nombre.data(),
                      cuenta[static_cast<std::size_t>(s)]);
        DrawText(buf, x + 16, y, 14, RAYWHITE);
        y += 17;
    }
    y += 6;
    std::snprintf(buf, sizeof(buf), "A* tick IA: %u rutas, %u nodos", p.busquedasUltimoTickIa(), p.nodosUltimoTickIa());
    DrawText(buf, x, y, 12, LIGHTGRAY);
    y += 15;
    std::snprintf(buf, sizeof(buf), "tiempo A*: %.1f us   FPS: %d", p.tiempoBusquedaUltimoTickIaUs(), fps);
    DrawText(buf, x, y, 12, LIGHTGRAY);
    y += 15;
    if (p.campoActivo())
        std::snprintf(buf, sizeof(buf), "Flow field: %u agentes, %.0f us", p.agentesEnCampo(), p.campo().tiempoUs());
    else
        std::snprintf(buf, sizeof(buf), "Flow field: inactivo (umbral %d)", cfg.flowfieldUmbral);
    DrawText(buf, x, y, 12, p.campoActivo() ? SKYBLUE : LIGHTGRAY);
    y += 15;
    std::snprintf(buf, sizeof(buf), "Coordinado (C): %s  alertas %llu / msj %llu", cfg.coordinado ? "si" : "no",
                  static_cast<unsigned long long>(p.alertasEmitidas()),
                  static_cast<unsigned long long>(p.mensajesAlerta()));
    DrawText(buf, x, y, 12, cfg.coordinado ? YELLOW : LIGHTGRAY);
    y += 15;
    std::snprintf(buf, sizeof(buf), "Contract Net: %llu subastas, %llu tareas",
                  static_cast<unsigned long long>(p.subastas()), static_cast<unsigned long long>(p.adjudicaciones()));
    DrawText(buf, x, y, 12, cfg.coordinado && cfg.contractNet ? Color{60, 200, 210, 255} : LIGHTGRAY);
    y += 20;

    DrawText("Transiciones FSM", x, y, 16, GOLD);
    y += 20;
    const std::size_t n = p.numTransiciones() < 6 ? p.numTransiciones() : 6;
    for (std::size_t i = 0; i < n; ++i) {
        const auto& t = p.ultimaTransicion(i);
        const auto a = agentes::nombreEstado(t.desde);
        const auto b = agentes::nombreEstado(t.hacia);
        const auto mo = agentes::nombreMotivo(t.motivo);
        std::snprintf(buf, sizeof(buf), "%5.1fs Z%u %.*s>%.*s: %.*s",
                      static_cast<double>(t.tick) * static_cast<double>(p.dt()), static_cast<unsigned>(t.agente),
                      static_cast<int>(a.size()), a.data(), static_cast<int>(b.size()), b.data(),
                      static_cast<int>(mo.size()), mo.data());
        DrawText(buf, x, y, 11, i == 0 ? RAYWHITE : GRAY);
        y += 14;
    }

    int yc = alto - 96;
    DrawText("WASD/flechas: mover", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("Shift: correr  Ctrl: sigilo", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("E: recoger / activar radio", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("F1: depuracion  R: reiniciar  C: coord.", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("Tab: laboratorio Dijkstra/A*", x, yc, 13, LIGHTGRAY);
}

void dibujarFinPartida(const juego::Partida& p, int anchoMapa, int altoMapa) {
    if (p.estado() == juego::EstadoPartida::EnCurso) return;
    DrawRectangle(0, 0, anchoMapa, altoMapa, Fade(BLACK, 0.55f));
    const bool gano = p.estado() == juego::EstadoPartida::Victoria;
    const char* titulo = gano ? "La radio funciona: llega la extraccion" : "Mara ha caido";
    const int w = MeasureText(titulo, 32);
    DrawText(titulo, (anchoMapa - w) / 2, altoMapa / 2 - 30, 32, gano ? SKYBLUE : RED);
    const char* sub = "R: jugar otra vez (nuevo mapa)";
    DrawText(sub, (anchoMapa - MeasureText(sub, 18)) / 2, altoMapa / 2 + 14, 18, RAYWHITE);
}

}  // namespace refugio::adaptadores
