#include "adaptadores/raylib/VistaPartida.hpp"

#include <raylib.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string_view>

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
        case EstadoInfectado::AtacarBarricada: return {150, 90, 40, 255};  // marrón: golpea una barricada
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

/// Nombre corto de cada acción del superviviente para que el plan quepa en una línea del panel.
const char* abreviada(superviviente::Accion a) {
    switch (a) {
        case superviviente::Accion::IrAlmacen: return "Alm";
        case superviviente::Accion::RecogerTablas: return "Tablas";
        case superviviente::Accion::ArrancarTablas: return "Arrancar";
        case superviviente::Accion::IrPuerta: return "Puerta";
        case superviviente::Accion::RepararPuerta: return "Reparar";
        case superviviente::Accion::AyudarInstalacion: return "Ayudar";
        case superviviente::Accion::Vigilar: return "Vigilar";
        case superviviente::Accion::Huir: return "Huir";
    }
    return "?";
}
constexpr Color kColorSuperviviente{40, 200, 170, 255};
constexpr Color kColorExpandido{220, 60, 220, 255};  // el mismo magenta que A* en el laboratorio
constexpr Color kColorSeleccion{255, 255, 255, 255};

void barra(int x, int y, int ancho, int alto, float fraccion, Color relleno) {
    DrawRectangle(x, y, ancho, alto, {50, 52, 58, 255});
    DrawRectangle(x, y, static_cast<int>(static_cast<float>(ancho) * std::fmax(0.0f, std::fmin(1.0f, fraccion))), alto,
                  relleno);
    DrawRectangleLines(x, y, ancho, alto, {90, 92, 100, 255});
}
void dibujarAyuda(int x, int alto) {
    int yc = alto - 128;
    DrawText("WASD/flechas: mover", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("Shift: correr  Ctrl: sigilo", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("E: recoger / activar radio", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("Q: senuelo al cursor  B: barricada", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("F1: depuracion  R: reiniciar  C: coord.", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("Clic: monitor  N: nodos  L: registro", x, yc, 13, LIGHTGRAY);
    yc += 16;
    DrawText("Tab: laboratorio Dijkstra/A*", x, yc, 13, LIGHTGRAY);
}

/// Copia @p t en @p buf cambiando cada tabulador por dos espacios (raylib no dibuja tabuladores).
void sinTabs(std::string_view t, char* buf, std::size_t tam) {
    std::size_t n = 0;
    for (const char ch : t) {
        if (n + 3 >= tam) break;
        if (ch == '\t') {
            buf[n++] = ' ';
            buf[n++] = ' ';
        } else {
            buf[n++] = ch;
        }
    }
    buf[n] = '\0';
}

/// Una entrada del historial del agente: una transición de la FSM o un cambio de plan.
struct EntradaHistorial {
    std::uint64_t tick{};
    bool esPlan{false};
    std::size_t indice{};
};

/// RF-070 + RF-072: monitor del agente seleccionado en el panel lateral, hasta @p limite en y.
void dibujarMonitorAgente(const juego::Partida& p, int x, int y, int limite) {
    char buf[128];
    const auto id = static_cast<std::size_t>(p.seleccionado());
    const auto& z = p.infectados()[id];
    const auto& m = z.busqueda;
    const auto& pc = z.percibido;
    std::snprintf(buf, sizeof(buf), "Agente Z%zu", id);
    DrawText(buf, x, y, 18, GOLD);
    DrawText("clic fuera: soltar", x + 130, y + 5, 10, GRAY);
    y += 22;
    const auto est = agentes::nombreEstado(z.estado);
    DrawCircle(x + 5, y + 6, 5, colorEstado(z.estado));
    std::snprintf(buf, sizeof(buf), "Estado: %.*s (%.1f s)", static_cast<int>(est.size()), est.data(),
                  static_cast<double>(z.tiempoEnEstado));
    DrawText(buf, x + 14, y, 13, RAYWHITE);
    y += 16;
    // Objetivo: la celda a la que va y por qué (tarea de Contract Net o táctica propia).
    const auto tac = agentes::nombreTactica(z.tactica);
    if (z.tieneTarea) {
        const auto ta = coordinacion::nombreTarea(z.tarea);
        std::snprintf(buf, sizeof(buf), "Objetivo: (%d,%d) tarea %.*s", z.objetivo.x, z.objetivo.y,
                      static_cast<int>(ta.size()), ta.data());
    } else if (z.tieneObjetivo) {
        std::snprintf(buf, sizeof(buf), "Objetivo: (%d,%d) tactica %.*s", z.objetivo.x, z.objetivo.y,
                      static_cast<int>(tac.size()), tac.data());
    } else {
        std::snprintf(buf, sizeof(buf), "Objetivo: ninguno");
    }
    DrawText(buf, x, y, 12, RAYWHITE);
    y += 15;
    // Percepción relevante del último ciclo de IA: vista, oído y mensajes.
    std::snprintf(buf, sizeof(buf), "Ve a Mara: %s (a %.1f)  ruidos: %u", pc.veAMara ? "si" : "no",
                  static_cast<double>(pc.distanciaMara), static_cast<unsigned>(pc.ruidosOidos));
    DrawText(buf, x, y, 12, pc.veAMara ? ORANGE : LIGHTGRAY);
    y += 15;
    if (pc.recibioAlerta)
        std::snprintf(buf, sizeof(buf), "Mensajes: alerta %.2f%s  memoria: %u", static_cast<double>(pc.confianzaAlerta),
                      pc.recibioTarea ? " + tarea" : "", static_cast<unsigned>(pc.evidencias));
    else
        std::snprintf(buf, sizeof(buf), "Mensajes: %s  memoria: %u", pc.recibioTarea ? "tarea" : "ninguno",
                      static_cast<unsigned>(pc.evidencias));
    DrawText(buf, x, y, 12, pc.recibioAlerta ? YELLOW : LIGHTGRAY);
    y += 18;

    DrawText("Ultima busqueda", x, y, 15, GOLD);
    y += 18;
    if (z.usaCampo) {
        std::snprintf(buf, sizeof(buf), "Ahora sigue el flow field (%.0f us)", p.campo().tiempoUs());
        DrawText(buf, x, y, 12, SKYBLUE);
        y += 15;
    }
    if (!m.valida) {
        DrawText("Aun no ha planificado", x, y, 12, LIGHTGRAY);
        y += 15;
    } else {
        const auto alg = navegacion::nombreAlgoritmo(m.algoritmo);
        std::snprintf(buf, sizeof(buf), "%.*s  heuristica: %s", static_cast<int>(alg.size()), alg.data(),
                      m.algoritmo == navegacion::Algoritmo::AEstrella ? "octil" : "h = 0");
        DrawText(buf, x, y, 13, kColorExpandido);
        y += 16;
        std::snprintf(buf, sizeof(buf), "(%d,%d) > (%d,%d) hace %.1f s", m.origen.x, m.origen.y, m.destino.x, m.destino.y,
                      static_cast<double>(p.tick() - m.tick) * static_cast<double>(p.dt()));
        DrawText(buf, x, y, 12, LIGHTGRAY);
        y += 15;
        const std::size_t restantes = z.ruta.size() > z.indiceRuta ? z.ruta.size() - z.indiceRuta : 0;
        if (m.encontrada)
            std::snprintf(buf, sizeof(buf), "Costo: %.2f  ruta: %u celdas, faltan %zu", m.costo,
                          static_cast<unsigned>(m.largoRuta), restantes);
        else
            std::snprintf(buf, sizeof(buf), "Sin ruta al objetivo");
        DrawText(buf, x, y, 12, RAYWHITE);
        y += 15;
        std::snprintf(buf, sizeof(buf), "Nodos expandidos: %u  generados: %u", m.expandidos, m.generados);
        DrawText(buf, x, y, 12, RAYWHITE);
        y += 15;
        std::snprintf(buf, sizeof(buf), "Tiempo: %.1f us  memoria: %.1f KB", m.tiempoUs,
                      static_cast<double>(m.memoriaBytes) / 1024.0);
        DrawText(buf, x, y, 12, RAYWHITE);
        y += 15;
        std::snprintf(buf, sizeof(buf), "Abierta max: %u   pintados: %zu", m.maxAbierta, p.expansionSeleccionado().size());
        DrawText(buf, x, y, 12, LIGHTGRAY);
        y += 15;
    }
    y += 4;

    // RF-072: historial del agente, mezclando transiciones y cambios de plan, el más reciente arriba.
    DrawText("Historial del agente", x, y, 15, GOLD);
    y += 18;
    std::array<EntradaHistorial, 12> h{};
    std::size_t nh = 0;
    for (std::size_t i = 0; i < p.numTransiciones() && nh < 6; ++i)
        if (p.ultimaTransicion(i).agente == id) h[nh++] = {p.ultimaTransicion(i).tick, false, i};
    const std::size_t nTrans = nh;
    for (std::size_t i = 0; i < p.numCambiosPlan() && nh - nTrans < 6; ++i)
        if (p.ultimoCambioPlan(i).agente == id) h[nh++] = {p.ultimoCambioPlan(i).tick, true, i};
    std::stable_sort(h.begin(), h.begin() + static_cast<std::ptrdiff_t>(nh),
                     [](const EntradaHistorial& a, const EntradaHistorial& b) { return a.tick > b.tick; });
    if (nh == 0) DrawText("Sin cambios todavia", x, y, 12, GRAY);
    for (std::size_t k = 0; k < nh && y + 13 <= limite; ++k) {
        const double t = static_cast<double>(h[k].tick) * static_cast<double>(p.dt());
        if (h[k].esPlan) {
            const auto& c = p.ultimoCambioPlan(h[k].indice);
            const auto mo = juego::nombreMotivoPlan(c.motivo);
            std::snprintf(buf, sizeof(buf), "%5.1fs plan %.*s >(%d,%d) %.2f>%.2f", t, static_cast<int>(mo.size()), mo.data(),
                          c.nuevo.x, c.nuevo.y, static_cast<double>(c.valorAnterior), static_cast<double>(c.valorNuevo));
            DrawText(buf, x, y, 11, k == 0 ? Color{255, 200, 120, 255} : Color{200, 160, 110, 255});
        } else {
            const auto& tr = p.ultimaTransicion(h[k].indice);
            const auto a = agentes::nombreEstado(tr.desde);
            const auto b = agentes::nombreEstado(tr.hacia);
            const auto mo = agentes::nombreMotivo(tr.motivo);
            std::snprintf(buf, sizeof(buf), "%5.1fs %.*s>%.*s: %.*s", t, static_cast<int>(a.size()), a.data(),
                          static_cast<int>(b.size()), b.data(), static_cast<int>(mo.size()), mo.data());
            DrawText(buf, x, y, 11, k == 0 ? RAYWHITE : GRAY);
        }
        y += 13;
    }
}
}  // namespace

ModoConsola siguienteModo(ModoConsola m) noexcept {
    switch (m) {
        case ModoConsola::Oculta: return ModoConsola::Planes;
        case ModoConsola::Planes: return ModoConsola::Mensajes;
        case ModoConsola::Mensajes: return ModoConsola::Todo;
        case ModoConsola::Todo: break;
    }
    return ModoConsola::Oculta;
}

void dibujarConsolaIa(const juego::Partida& p, ModoConsola modo, int x, int y, int ancho, int filas) {
    if (modo == ModoConsola::Oculta) return;
    using juego::TipoEventoIa;
    constexpr int kAltoLinea = 13;
    const int alto = (filas + 1) * kAltoLinea + 8;
    DrawRectangle(x, y, ancho, alto, Fade(BLACK, 0.72f));
    DrawRectangleLines(x, y, ancho, alto, {90, 92, 100, 255});
    const char* titulo = modo == ModoConsola::Planes     ? "Registro de IA: estados y planes (L: mensajes)"
                         : modo == ModoConsola::Mensajes ? "Registro de IA: mensajes (L: todo)"
                                                         : "Registro de IA: todo (L: ocultar)";
    const auto& reg = p.registroIa();
    char buf[200];
    std::snprintf(buf, sizeof(buf), "%s   %llu lineas en logs/ia_%llu.log", titulo,
                  static_cast<unsigned long long>(reg.total()), static_cast<unsigned long long>(p.semilla()));
    DrawText(buf, x + 6, y + 4, 11, GOLD);
    // Las líneas que tocan, de la más reciente hacia atrás; se dibujan con la más nueva abajo.
    std::array<std::size_t, 32> elegidas{};
    std::size_t n = 0;
    const auto max = static_cast<std::size_t>(filas) < elegidas.size() ? static_cast<std::size_t>(filas) : elegidas.size();
    for (std::size_t i = 0; i < reg.tamano() && n < max; ++i) {
        const auto t = reg.ultima(i).tipo;
        const bool esMensaje = t == TipoEventoIa::Mensaje;
        if ((modo == ModoConsola::Planes && esMensaje) || (modo == ModoConsola::Mensajes && !esMensaje)) continue;
        elegidas[n++] = i;
    }
    int yl = y + 4 + kAltoLinea + static_cast<int>(max - n) * kAltoLinea;
    for (std::size_t k = n; k-- > 0;) {
        const auto& l = reg.ultima(elegidas[k]);
        const auto tipo = juego::nombreEventoIa(l.tipo);
        char linea[200];
        sinTabs(l.vista(), linea, sizeof(linea));
        char salida[256];
        std::snprintf(salida, sizeof(salida), "%6.1fs %-13.*s %s", static_cast<double>(l.tick) * static_cast<double>(p.dt()),
                      static_cast<int>(tipo.size()), tipo.data(), linea);
        const Color c = l.tipo == TipoEventoIa::Plan            ? Color{255, 200, 120, 255}
                        : l.tipo == TipoEventoIa::Mensaje       ? Color{240, 230, 120, 255}
                        : l.tipo == TipoEventoIa::Superviviente ? kColorSuperviviente
                                                                : LIGHTGRAY;
        DrawText(salida, x + 6, yl, 11, k == 0 ? c : Fade(c, 0.8f));
        yl += kAltoLinea;
    }
}

void dibujarPartida(const juego::Partida& p, const EstiloVista& e, bool depuracion, bool nodosExpandidos) {
    dibujarGrafo(p.mapa(), e);
    const float tc = static_cast<float>(e.tamCelda);

    // RF-071: nodos cerrados por la última búsqueda del agente seleccionado, más oscuros cuanto
    // antes se expandieron. Hay exactamente tantos como «expandidos» en el panel (CP-071).
    if (nodosExpandidos && p.seleccionado() >= 0) {
        const auto& exp = p.expansionSeleccionado();
        const float total = static_cast<float>(exp.size() > 0 ? exp.size() : 1);
        for (std::size_t k = 0; k < exp.size(); ++k) {
            const Vector2 c = centroCelda(p.mapa().celda(exp[k]), e);
            const float f = 0.25f + 0.35f * (1.0f - static_cast<float>(k) / total);
            DrawRectangleRec({c.x - tc * 0.5f + 1, c.y - tc * 0.5f + 1, tc - 2, tc - 2}, Fade(kColorExpandido, f));
        }
    }

    // Radio: se ilumina cuando Mara ya tiene los tres objetos.
    const Vector2 cr = centroCelda(p.radio(), e);
    const bool lista = p.tieneTodosLosObjetos();
    if (lista) DrawCircleV(cr, tc * (0.7f + 0.1f * std::sin(static_cast<float>(p.tick()) * 0.1f)), Fade(SKYBLUE, 0.35f));
    DrawRectangleRec({cr.x - tc * 0.35f, cr.y - tc * 0.2f, tc * 0.7f, tc * 0.5f}, {60, 70, 90, 255});
    DrawRectangleLinesEx({cr.x - tc * 0.35f, cr.y - tc * 0.2f, tc * 0.7f, tc * 0.5f}, 2.0f, SKYBLUE);
    DrawLineEx({cr.x + tc * 0.2f, cr.y - tc * 0.2f}, {cr.x + tc * 0.3f, cr.y - tc * 0.45f}, 2.0f, SKYBLUE);
    // RF-063: la radio está en el refugio; si golpean su puerta parpadea en rojo.
    if (p.golpeandoPuerta() > 0 && (p.tick() / 6) % 2 == 0) DrawCircleLinesV(cr, tc * 0.6f, RED);

    // RF-009: zona de extracción, verde mientras está abierta, con su temporizador.
    const Vector2 cx = centroCelda(p.extraccion(), e);
    const float rx = p.config().radioExtraccion * tc;
    if (p.zonaAbierta()) {
        DrawCircleV(cx, rx, Fade(GREEN, 0.25f + 0.1f * std::sin(static_cast<float>(p.tick()) * 0.15f)));
        DrawCircleLinesV(cx, rx, {40, 160, 60, 255});
        char t[16];
        std::snprintf(t, sizeof(t), "%.0f s", static_cast<double>(p.restanteExtraccion()));
        DrawText(t, static_cast<int>(cx.x) - MeasureText(t, 16) / 2, static_cast<int>(cx.y - rx) - 18, 16, {20, 110, 40, 255});
    } else {
        DrawCircleLinesV(cx, rx, Fade(DARKGRAY, 0.7f));
    }
    DrawText("EXT", static_cast<int>(cx.x) - MeasureText("EXT", 10) / 2, static_cast<int>(cx.y) - 5, 10,
             p.zonaAbierta() ? Color{20, 90, 30, 255} : GRAY);

    for (const auto& o : p.objetos())
        if (!o.recogido) dibujarObjeto(o.tipo, centroCelda(o.celda, e), tc * 0.55f);

    // HU-13: almacén de tablas.
    if (p.almacen().x >= 0) {
        const Vector2 ca = centroCelda(p.almacen(), e);
        const Rectangle r{ca.x - tc * 0.4f, ca.y - tc * 0.3f, tc * 0.8f, tc * 0.6f};
        DrawRectangleRec(r, {165, 120, 70, 255});
        for (int k = 1; k < 3; ++k)
            DrawLineEx({r.x, r.y + r.height * static_cast<float>(k) / 3.0f}, {r.x + r.width, r.y + r.height * static_cast<float>(k) / 3.0f},
                       1.5f, {100, 65, 30, 255});
        DrawRectangleLinesEx(r, 1.5f, {100, 65, 30, 255});
        DrawText("ALM", static_cast<int>(ca.x) - MeasureText("ALM", 10) / 2, static_cast<int>(r.y) - 11, 10, {100, 65, 30, 255});
    }

    // RF-014: barricada de tablas con su durabilidad.
    const auto& cfgP = p.config();
    for (const auto& b : p.barricadas()) {
        if (!b.activa) continue;
        const Vector2 c = centroCelda(b.celda, e);
        const Rectangle r{c.x - tc * 0.45f, c.y - tc * 0.45f, tc * 0.9f, tc * 0.9f};
        DrawRectangleRec(r, {120, 80, 45, 255});
        DrawLineEx({r.x, r.y}, {r.x + r.width, r.y + r.height}, 3.0f, {80, 50, 25, 255});
        DrawLineEx({r.x + r.width, r.y}, {r.x, r.y + r.height}, 3.0f, {80, 50, 25, 255});
        DrawRectangleLinesEx(r, 2.0f, {80, 50, 25, 255});
        const float f = b.durabilidad / cfgP.durabilidadBarricada;
        DrawRectangleRec({r.x, r.y - 5.0f, r.width, 3.0f}, {50, 52, 58, 255});
        DrawRectangleRec({r.x, r.y - 5.0f, r.width * f, 3.0f}, f > 0.3f ? Color{230, 170, 60, 255} : RED);
    }
    // RF-013: señuelo con la onda de su último ruido.
    for (const auto& sn : p.senuelos()) {
        if (!sn.activo) continue;
        const Vector2 c = centroCelda(sn.celda, e);
        const float fase = sn.desdeRuido / cfgP.senueloPeriodo;
        if (fase < 1.0f)
            DrawCircleLinesV(c, cfgP.senueloRadio * tc * std::fmin(1.0f, fase * 1.5f), Fade(GOLD, 0.6f * (1.0f - fase)));
        DrawCircleV(c, tc * 0.22f, {250, 210, 60, 255});
        DrawCircleLinesV(c, tc * 0.22f, {90, 70, 10, 255});
        DrawCircleLinesV(c, tc * 0.32f, Fade(GOLD, 0.8f));
    }

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
            if (z.estado == EstadoInfectado::Interceptar) {  // tarea de Contract Net o táctica propia
                const Vector2 d = centroCelda(z.tieneTarea ? z.destinoTarea : z.objetivo, e);
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

    // RF-070: agente seleccionado. Su ruta se ve siempre (no solo con F1), con su objetivo y un aro.
    if (p.seleccionado() >= 0) {
        const auto& z = p.infectados()[static_cast<std::size_t>(p.seleccionado())];
        const Vector2 c = aPantalla(z.pos, e);
        for (std::size_t k = z.indiceRuta; k < z.ruta.size(); ++k) {
            const Vector2 a = k == z.indiceRuta ? c : centroCelda(p.mapa().celda(z.ruta[k - 1]), e);
            DrawLineEx(a, centroCelda(p.mapa().celda(z.ruta[k]), e), 4.0f, Fade(kColorSeleccion, 0.9f));
        }
        if (z.tieneObjetivo) {
            const Vector2 o = centroCelda(z.objetivo, e);
            DrawRectangleLinesEx({o.x - tc * 0.45f, o.y - tc * 0.45f, tc * 0.9f, tc * 0.9f}, 3.0f, kColorSeleccion);
            DrawLineEx({o.x - tc * 0.3f, o.y}, {o.x + tc * 0.3f, o.y}, 2.0f, kColorSeleccion);
            DrawLineEx({o.x, o.y - tc * 0.3f}, {o.x, o.y + tc * 0.3f}, 2.0f, kColorSeleccion);
        }
        const float pulso = 0.5f + 0.06f * std::sin(static_cast<float>(p.tick()) * 0.2f);
        DrawCircleLinesV(c, tc * pulso, kColorSeleccion);
        DrawCircleLinesV(c, tc * pulso + 1.0f, kColorSeleccion);
        char id[8];
        std::snprintf(id, sizeof(id), "Z%d", p.seleccionado());
        const float anchoMapa = static_cast<float>(e.origenX) + static_cast<float>(p.mapa().ancho()) * tc;
        const float xId = c.x + tc * 0.45f + 24.0f > anchoMapa ? c.x - tc * 0.45f - 22.0f : c.x + tc * 0.45f;
        DrawText(id, static_cast<int>(xId), static_cast<int>(c.y - tc * 0.8f), 14, kColorSeleccion);
    }

    // HU-13: superviviente de la radio (ruta en depuración, chispas al reparar, aro al ayudar).
    if (const auto& s = p.superviviente(); s.activo) {
        const Vector2 cs = aPantalla(s.pos, e);
        if (depuracion && s.actividad == juego::Actividad::Mover)
            for (std::size_t k = s.indiceRuta; k < s.ruta.size(); ++k) {
                const Vector2 a = k == s.indiceRuta ? cs : centroCelda(p.mapa().celda(s.ruta[k - 1]), e);
                DrawLineEx(a, centroCelda(p.mapa().celda(s.ruta[k]), e), 2.0f, Fade(kColorSuperviviente, 0.8f));
            }
        if (s.actividad == juego::Actividad::Reparar && (p.tick() / 8) % 2 == 0)
            DrawCircleV({cs.x + s.orientacion.x * tc * 0.5f, cs.y + s.orientacion.y * tc * 0.5f}, tc * 0.12f, GOLD);
        if (s.ayudando) DrawCircleLinesV(cs, tc * 0.45f, SKYBLUE);
        DrawCircleV(cs, tc * 0.28f, kColorSuperviviente);
        DrawCircleLinesV(cs, tc * 0.28f, RAYWHITE);
        DrawLineEx(cs, {cs.x + s.orientacion.x * tc * 0.38f, cs.y + s.orientacion.y * tc * 0.38f}, 2.5f, RAYWHITE);
        if (s.tablas > 0.0f) DrawRectangleRec({cs.x - tc * 0.3f, cs.y + tc * 0.3f, tc * 0.6f, tc * 0.12f}, {165, 120, 70, 255});
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
    y += 18;
    std::snprintf(buf, sizeof(buf), "Senuelos (Q): %d   Barricadas (B): %d", m.senuelos, m.barricadas);
    DrawText(buf, x, y, 14, LIGHTGRAY);
    y += 22;

    DrawText("Objetivos", x, y, 18, GOLD);
    y += 22;
    for (int t = 0; t < juego::kNumTiposObjeto; ++t) {
        const bool tiene = m.inventario[static_cast<std::size_t>(t)];
        std::snprintf(buf, sizeof(buf), "[%c] %s", tiene ? 'x' : ' ', nombreObjeto(static_cast<juego::TipoObjeto>(t)));
        DrawText(buf, x, y, 14, tiene ? Color{120, 220, 130, 255} : LIGHTGRAY);
        y += 18;
    }
    if (p.zonaAbierta()) {
        std::snprintf(buf, sizeof(buf), "Extraccion abierta: %.0f s", static_cast<double>(p.restanteExtraccion()));
        DrawText(buf, x, y, 14, GREEN);
    } else if (p.activacionesRadio() > 0 && p.tieneTodosLosObjetos()) {
        DrawText("Extraccion cerrada: activa otra vez", x, y, 14, ORANGE);
    } else {
        DrawText(p.tieneTodosLosObjetos() ? "Ve a la radio y manten E" : "Despues: activar la radio", x, y, 14,
                 p.tieneTodosLosObjetos() ? SKYBLUE : GRAY);
    }
    y += 18;
    std::snprintf(buf, sizeof(buf), "Refugio %.0f", static_cast<double>(p.integridad()));
    DrawText(buf, x, y, 14, p.golpeandoPuerta() > 0 ? RED : LIGHTGRAY);
    barra(x + 90, y + 2, 130, 10, p.integridad() / cfg.integridadMax,
          p.integridad() > 30.0f ? Color{200, 160, 70, 255} : RED);
    y += 18;
    if (m.progresoInstalacion > 0.0f) {
        barra(x, y, 220, 10, m.progresoInstalacion / cfg.tInstalacion, SKYBLUE);
        y += 14;
    }
    // HU-13: meta (RF-060), plan GOAP con la acción en curso (RF-061) y último motivo de replan (RF-064).
    if (const auto& s = p.superviviente(); s.activo) {
        const auto meta = superviviente::nombreMeta(s.meta);
        const auto act = juego::nombreActividad(s.actividad);
        std::snprintf(buf, sizeof(buf), "Superviviente: %.*s (%.*s)", static_cast<int>(meta.size()), meta.data(),
                      static_cast<int>(act.size()), act.data());
        DrawText(buf, x, y, 12, kColorSuperviviente);
        y += 14;
        int n = std::snprintf(buf, sizeof(buf), "Plan:");
        for (std::size_t i = 0; i < s.plan.longitud && n > 0 && static_cast<std::size_t>(n) < sizeof(buf); ++i) {
            const char* a = abreviada(static_cast<superviviente::Accion>(s.plan.acciones[i]));
            n += std::snprintf(buf + n, sizeof(buf) - static_cast<std::size_t>(n), i == s.paso ? "%s[%s]" : "%s%s",
                               i == 0 ? " " : " > ", a);
        }
        DrawText(buf, x, y, 11, LIGHTGRAY);
        y += 13;
        const auto& u = s.utilidades;
        const auto mo = p.numRegistrosSuperviviente() > 0 ? juego::nombreMotivoReplan(p.ultimoRegistroSuperviviente(0).motivo)
                                                          : std::string_view{"-"};
        std::snprintf(buf, sizeof(buf), "U R%.2f A%.2f S%.2f V%.2f  %.*s", static_cast<double>(u[0]), static_cast<double>(u[1]),
                      static_cast<double>(u[2]), static_cast<double>(u[3]), static_cast<int>(mo.size()), mo.data());
        DrawText(buf, x, y, 11, GRAY);
        y += 13;
    }
    y += 6;

    if (p.seleccionado() >= 0) {
        dibujarMonitorAgente(p, x, y, alto - 150);
        dibujarAyuda(x, alto);
        return;
    }

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
        y += 15;
    }
    y += 4;
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
    y += 15;
    // RF-054: cuántos infectados prefieren cada táctica ahora mismo.
    std::array<int, agentes::kNumTacticas> tacticas{};
    for (const auto& z : p.infectados()) ++tacticas[static_cast<std::size_t>(z.tactica)];
    std::snprintf(buf, sizeof(buf), "Tacticas P / I / E: %d / %d / %d", tacticas[0], tacticas[1], tacticas[2]);
    DrawText(buf, x, y, 12, cfg.tacticas ? LIGHTGRAY : GRAY);
    y += 16;

    DrawText("Transiciones FSM", x, y, 16, GOLD);
    y += 18;
    const std::size_t n = p.numTransiciones() < 2 ? p.numTransiciones() : 2;
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
    y += 4;
    // RF-072: cambios de objetivo con sus utilidades (o costos de ruta, si es por una barricada).
    DrawText("Cambios de plan", x, y, 16, GOLD);
    y += 18;
    const std::size_t np = p.numCambiosPlan() < 2 ? p.numCambiosPlan() : 2;
    for (std::size_t i = 0; i < np; ++i) {
        const auto& c = p.ultimoCambioPlan(i);
        const auto mo = juego::nombreMotivoPlan(c.motivo);
        std::snprintf(buf, sizeof(buf), "%5.1fs Z%u %.*s >(%d,%d) %.2f>%.2f",
                      static_cast<double>(c.tick) * static_cast<double>(p.dt()), static_cast<unsigned>(c.agente),
                      static_cast<int>(mo.size()), mo.data(), c.nuevo.x, c.nuevo.y,
                      static_cast<double>(c.valorAnterior), static_cast<double>(c.valorNuevo));
        DrawText(buf, x, y, 11, i == 0 ? RAYWHITE : GRAY);
        y += 14;
    }

    dibujarAyuda(x, alto);
}

void dibujarFinPartida(const juego::Partida& p, int anchoMapa, int altoMapa) {
    if (p.estado() == juego::EstadoPartida::EnCurso) return;
    DrawRectangle(0, 0, anchoMapa, altoMapa, Fade(BLACK, 0.55f));
    const bool gano = p.estado() == juego::EstadoPartida::Victoria;
    const char* titulo = gano                                               ? "Mara llega a la extraccion: victoria"
                         : p.motivoFin() == juego::MotivoFin::Refugio ? "Derrota: refugio inutilizado"
                                                                      : "Derrota: Mara ha caido";
    const int w = MeasureText(titulo, 32);
    DrawText(titulo, (anchoMapa - w) / 2, altoMapa / 2 - 30, 32, gano ? SKYBLUE : RED);
    const char* sub = "R: jugar otra vez (nuevo mapa)";
    DrawText(sub, (anchoMapa - MeasureText(sub, 18)) / 2, altoMapa / 2 + 14, 18, RAYWHITE);
}

}  // namespace refugio::adaptadores
