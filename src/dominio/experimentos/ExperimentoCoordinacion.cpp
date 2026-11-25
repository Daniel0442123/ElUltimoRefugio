#include "dominio/experimentos/ExperimentoCoordinacion.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <iomanip>

namespace refugio::experimentos {

using agentes::celdaDe;
using agentes::centroDe;
using juego::EstadoPartida;
using juego::Partida;
using mundo::Celda;
using navegacion::Vec2;

namespace {

constexpr float kLlegada = 0.2f;     ///< Distancia al centro de una celda para darla por alcanzada.
constexpr float kContacto = 1.0f;    ///< RF-055: a 1 celda o menos hay contacto.
constexpr float kCerca = 2.0f;       ///< Radio para contar cuántos rodean a Mara a la vez.

float distancia(Vec2 a, Vec2 b) noexcept { return std::hypot(a.x - b.x, a.y - b.y); }

/// Celda transitable más cercana a @p c (anillos de Chebyshev crecientes); la propia si lo es.
Celda transitableCerca(const mundo::GrafoRejilla& g, Celda c) noexcept {
    if (g.transitable(c.x, c.y)) return c;
    for (int r = 1; r <= 4; ++r)
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (std::max(std::abs(dx), std::abs(dy)) == r && g.transitable(c.x + dx, c.y + dy))
                    return {c.x + dx, c.y + dy};
    return c;
}

/// FNV-1a de 64 bits.
struct Fnv {
    std::uint64_t h{1469598103934665603ULL};
    void mezclar(std::uint64_t v) noexcept {
        for (int i = 0; i < 8; ++i) {
            h ^= (v >> (8 * i)) & 0xFFU;
            h *= 1099511628211ULL;
        }
    }
    void mezclar(float f) noexcept { mezclar(static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(f))); }
    void mezclar(Celda c) noexcept {
        mezclar(static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.x)) << 32 | static_cast<std::uint32_t>(c.y));
    }
};

void acumular(ResumenModo& r, const MetricasSimulacion& m) noexcept {
    ++r.simulaciones;
    if (m.tPrimeraDeteccion >= 0.0) {
        ++r.detecciones;
        r.mediaDeteccion += m.tPrimeraDeteccion;
    }
    if (m.tContacto >= 0.0) {
        ++r.contactos;
        r.mediaContacto += m.tContacto;
    }
    if (m.estado == EstadoPartida::Victoria) ++r.victorias;
    if (m.estado == EstadoPartida::Derrota) ++r.derrotas;
    r.mediaCobertura += m.cobertura;
    r.mediaMensajes += static_cast<double>(m.mensajes);
    r.mediaSobrecargadas += static_cast<double>(m.celdasSobrecargadas);
    r.mediaDuracion += m.duracion;
    r.mediaMaxCerca += m.maxCerca;
}

void cerrar(ResumenModo& r) noexcept {
    if (r.detecciones > 0) r.mediaDeteccion /= r.detecciones;
    if (r.contactos > 0) r.mediaContacto /= r.contactos;
    if (r.simulaciones > 0) {
        r.mediaCobertura /= r.simulaciones;
        r.mediaMensajes /= r.simulaciones;
        r.mediaSobrecargadas /= r.simulaciones;
        r.mediaDuracion /= r.simulaciones;
        r.mediaMaxCerca /= r.simulaciones;
    }
}

std::string_view nombreEstado(EstadoPartida e) noexcept {
    switch (e) {
        case EstadoPartida::Victoria: return "victoria";
        case EstadoPartida::Derrota: return "derrota";
        default: return "en_curso";
    }
}

std::string_view nombreMotivo(juego::MotivoFin m) noexcept {
    switch (m) {
        case juego::MotivoFin::Extraccion: return "extraccion";
        case juego::MotivoFin::Salud: return "salud";
        case juego::MotivoFin::Refugio: return "refugio";
        default: return "tiempo";
    }
}

void escribirFila(const MetricasSimulacion& m, std::ostream& csv) {
    csv << nombreModo(m.modo) << ',' << m.semilla << ',' << m.repeticion << ',' << m.ticks << ',' << m.duracion << ','
        << nombreEstado(m.estado) << ',' << nombreMotivo(m.motivo) << ',' << m.tPrimeraDeteccion << ','
        << m.tContacto << ',' << m.cobertura << ',' << m.mensajes << ',' << m.mensajesAlerta << ','
        << m.mensajesContractNet << ',' << m.alertas << ',' << m.subastas << ',' << m.celdasSobrecargadas << ','
        << m.maxSobrecargadas << ',' << m.maxCerca << ',' << m.saludFinal << ',' << m.objetos << ',' << std::hex << m.hashPrevioDeteccion
        << ',' << m.hashFinal << std::dec << '\n';
}

}  // namespace

std::string_view nombreModo(ModoHorda m) noexcept { return m == ModoHorda::Aislado ? "aislado" : "coordinado"; }

// ------------------------------------------------------------------------------ GuionJugadora
GuionJugadora::GuionJugadora(const Partida& p) {
    buscador_.reservar(p.mapa().numCeldas());
    ruta_.ruta.reserve(p.mapa().numCeldas());
}

Celda GuionJugadora::celdaMeta(const Partida& p) const noexcept {
    const auto& objs = p.objetos();
    if (meta_ < objs.size()) return transitableCerca(p.mapa(), objs[meta_].celda);
    if (meta_ == objs.size()) return transitableCerca(p.mapa(), p.radio());
    return transitableCerca(p.mapa(), p.extraccion());
}

void GuionJugadora::planificar(const Partida& p, Celda destino) {
    const auto& g = p.mapa();
    const Celda aqui = celdaDe(p.mara().pos);
    buscador_.buscarEn(g, g.nodo(aqui.x, aqui.y), g.nodo(destino.x, destino.y), ruta_);
    siguiente_ = 0;
    destino_ = destino;
    tickPlan_ = p.tick();
    planificado_ = true;
}

juego::EntradaJugador GuionJugadora::entrada(const Partida& p) {
    const auto& objs = p.objetos();
    // Avanza de meta: objetos ya recogidos, radio activada.
    while (meta_ < objs.size() && objs[meta_].recogido) ++meta_;
    if (meta_ == objs.size() && p.activacionesRadio() > 0) ++meta_;

    juego::EntradaJugador e;
    e.modo = juego::ModoMovimiento::Caminar;
    e.interactuar = true;  // recoge lo que tenga al alcance y mantiene la instalación de la radio
    const Celda destino = celdaMeta(p);
    const auto hz = static_cast<std::uint64_t>(std::max(1, p.config().tickSimHz));
    if (!planificado_ || !(destino == destino_) || p.tick() >= tickPlan_ + hz) planificar(p, destino);
    if (!ruta_.encontrada) return e;

    const Vec2 pos = p.mara().pos;
    const auto& g = p.mapa();
    while (siguiente_ < ruta_.ruta.size() && distancia(pos, centroDe(g.celda(ruta_.ruta[siguiente_]))) <= kLlegada)
        ++siguiente_;
    if (siguiente_ >= ruta_.ruta.size()) return e;  // en la meta: quieta
    const Vec2 objetivo = centroDe(g.celda(ruta_.ruta[siguiente_]));
    const float d = distancia(pos, objetivo);
    e.ejeX = (objetivo.x - pos.x) / d;
    e.ejeY = (objetivo.y - pos.y) / d;
    return e;
}

// ---------------------------------------------------------------------------------- Huella
std::uint64_t hashEstado(const Partida& p) noexcept {
    Fnv f;
    f.mezclar(p.tick());
    f.mezclar(p.mara().pos.x);
    f.mezclar(p.mara().pos.y);
    f.mezclar(p.mara().salud);
    f.mezclar(p.integridad());
    for (const auto& z : p.infectados()) {
        f.mezclar(z.pos.x);
        f.mezclar(z.pos.y);
        f.mezclar(z.orientacion.x);
        f.mezclar(z.orientacion.y);
        f.mezclar(static_cast<std::uint64_t>(z.estado));
        f.mezclar(z.objetivo);
        f.mezclar(static_cast<std::uint64_t>(z.ruta.size()));
    }
    return f.h;
}

// ------------------------------------------------------------------------------- Simulación
MetricasSimulacion simularHorda(const ConfigCoordinacion& cfg, ModoHorda modo, int repeticion) {
    const std::uint64_t semilla = cfg.semilla + static_cast<std::uint64_t>(repeticion);
    juego::ConfigPartida cp = cfg.partida;
    cp.coordinado = modo == ModoHorda::Coordinado;  // RF-056: lo único que cambia
    Partida p(juego::crearEscenarioAleatorio(cfg.ancho, cfg.alto, cfg.infectados, semilla), cp, semilla);
    GuionJugadora guion(p);

    MetricasSimulacion m;
    m.modo = modo;
    m.semilla = semilla;
    m.repeticion = repeticion;

    const auto& g = p.mapa();
    std::vector<std::uint8_t> pisada(g.numCeldas(), 0);
    std::vector<std::uint8_t> ocupacion(g.numCeldas(), 0);
    std::uint32_t transitables = 0;
    for (mundo::Nodo n = 0; n < g.numCeldas(); ++n)
        if (g.transitable(n)) ++transitables;
    std::uint32_t pisadas = 0;
    const auto marcar = [&] {
        for (const auto& z : p.infectados()) {
            const Celda c = celdaDe(z.pos);
            if (!g.dentro(c.x, c.y)) continue;
            auto& v = pisada[g.nodo(c.x, c.y)];
            if (v == 0 && g.transitable(c.x, c.y)) ++pisadas;
            v = 1;
        }
    };
    marcar();

    const int pasosIa = std::max(1, cp.tickSimHz / std::max(1, cp.tickIaHz));
    const auto maxTicks = static_cast<std::uint64_t>(cfg.segundos * cp.tickSimHz);
    std::uint64_t hashPrevio = hashEstado(p);
    while (p.estado() == EstadoPartida::EnCurso && p.tick() < maxTicks) {
        p.paso(guion.entrada(p));
        marcar();
        if (m.tPrimeraDeteccion < 0.0) {
            const bool visto = std::any_of(p.infectados().begin(), p.infectados().end(),
                                           [](const juego::Infectado& z) { return z.percibido.veAMara; });
            if (visto) {
                m.tPrimeraDeteccion = p.tiempo();
                m.hashPrevioDeteccion = hashPrevio;
            } else {
                hashPrevio = hashEstado(p);
            }
        }
        if (m.tContacto < 0.0 &&
            std::any_of(p.infectados().begin(), p.infectados().end(),
                        [&](const juego::Infectado& z) { return distancia(z.pos, p.mara().pos) <= kContacto; }))
            m.tContacto = p.tiempo();
        const auto cerca = std::count_if(p.infectados().begin(), p.infectados().end(), [&](const juego::Infectado& z) {
            return distancia(z.pos, p.mara().pos) <= kCerca;
        });
        m.maxCerca = std::max(m.maxCerca, static_cast<int>(cerca));
        // RF-051: celdas con más infectados de los permitidos, contadas en cada tick de IA.
        if (p.tick() % static_cast<std::uint64_t>(pasosIa) == 0) {
            int sobre = 0;
            for (const auto& z : p.infectados()) {
                const Celda c = celdaDe(z.pos);
                if (!g.dentro(c.x, c.y)) continue;
                if (++ocupacion[g.nodo(c.x, c.y)] == cp.maxAgentesPorCelda + 1) ++sobre;
            }
            for (const auto& z : p.infectados()) {
                const Celda c = celdaDe(z.pos);
                if (g.dentro(c.x, c.y)) ocupacion[g.nodo(c.x, c.y)] = 0;
            }
            m.celdasSobrecargadas += static_cast<std::uint64_t>(sobre);
            m.maxSobrecargadas = std::max(m.maxSobrecargadas, sobre);
        }
    }
    if (m.tPrimeraDeteccion < 0.0) m.hashPrevioDeteccion = hashPrevio;

    m.ticks = p.tick();
    m.duracion = p.tiempo();
    m.estado = p.estado();
    m.motivo = p.motivoFin();
    m.cobertura = transitables > 0 ? static_cast<double>(pisadas) / transitables : 0.0;
    m.mensajesAlerta = p.mensajesAlerta();
    m.mensajesContractNet = p.mensajesContractNet();
    m.mensajes = p.totalMensajes();
    m.alertas = p.alertasEmitidas();
    m.subastas = p.subastas();
    m.saludFinal = p.mara().salud;
    m.objetos = static_cast<int>(std::count_if(p.objetos().begin(), p.objetos().end(),
                                               [](const juego::ObjetoMision& o) { return o.recogido; }));
    m.hashFinal = hashEstado(p);
    return m;
}

const std::vector<std::string_view>& columnasCoordinacion() {
    static const std::vector<std::string_view> cols{
        "modo",           "semilla",          "repeticion",       "ticks",            "duracion_s",
        "resultado",      "motivo_fin",       "t_primera_deteccion_s", "t_contacto_s",
        "cobertura",      "mensajes",         "mensajes_alerta",  "mensajes_contract_net",
        "alertas",        "subastas",         "celdas_sobrecargadas", "max_celdas_sobrecargadas", "max_infectados_cerca",
        "salud_final",    "objetos_recogidos", "hash_previo_deteccion", "hash_final"};
    return cols;
}

ResumenCoordinacion ejecutarExperimentoCoordinacion(const ConfigCoordinacion& cfg,
                                                    const std::vector<std::string>& metadatos, std::ostream& csv) {
    csv << std::setprecision(10);
    for (const auto& linea : metadatos) csv << "# " << linea << '\n';
    csv << "# experimento=aislados_vs_coordinados semilla_maestra=" << cfg.semilla << " repeticiones=" << cfg.repeticiones
        << " mapa=ciudad_" << cfg.ancho << 'x' << cfg.alto << " infectados=" << cfg.infectados
        << " segundos=" << cfg.segundos << " guion=objetos-radio-extraccion\n";
    const auto& cols = columnasCoordinacion();
    for (std::size_t i = 0; i < cols.size(); ++i) csv << (i ? "," : "") << cols[i];
    csv << '\n';

    ResumenCoordinacion r;
    r.simulaciones.reserve(static_cast<std::size_t>(std::max(0, cfg.repeticiones)) * 2);
    for (int rep = 0; rep < cfg.repeticiones; ++rep) {
        const auto a = simularHorda(cfg, ModoHorda::Aislado, rep);
        const auto c = simularHorda(cfg, ModoHorda::Coordinado, rep);
        escribirFila(a, csv);
        escribirFila(c, csv);
        acumular(r.aislado, a);
        acumular(r.coordinado, c);
        // CP-056: hasta la primera detección las dos simulaciones son la misma.
        if (a.tPrimeraDeteccion != c.tPrimeraDeteccion || a.hashPrevioDeteccion != c.hashPrevioDeteccion)
            ++r.paresDivergentes;
        if (c.tContacto >= 0.0 && (a.tContacto < 0.0 || c.tContacto < a.tContacto)) ++r.contactoAntesCoordinado;
        r.simulaciones.push_back(a);
        r.simulaciones.push_back(c);
    }
    cerrar(r.aislado);
    cerrar(r.coordinado);
    return r;
}

}  // namespace refugio::experimentos
