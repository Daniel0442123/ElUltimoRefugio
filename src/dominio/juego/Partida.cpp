#include "dominio/juego/Partida.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

#include "dominio/experimentos/GeneradorMapas.hpp"

namespace refugio::juego {

using agentes::celdaDe;
using agentes::centroDe;
using mundo::Celda;

namespace {
constexpr std::size_t kBloqueVisitasU = 4;
constexpr int kBloqueVisitas = 4;        // RF-045: mapa de visitas en bloques de 4 x 4 celdas
constexpr float kAvanceWaypoint = 0.35f;  // Con steering se pasa al siguiente punto de ruta a esta distancia
constexpr float kLlegadaFinal = 0.15f;    // y el último se da por alcanzado a esta otra
constexpr float kDistanciaAtaque = 0.6f;  // Sin celda de llegada, se detiene a esta distancia de Mara
constexpr float kRadioLlegada = 0.75f;    // RF-051: distancia a Mara del punto de llegada de cada acceso
constexpr float kFrenado = 0.5f;          // Radio de frenado suave al llegar
constexpr std::size_t kMaxTareas = 4;     // Interceptar + 2 x Flanquear + Perseguir
constexpr std::size_t kMaxRuidosTick = 32;
constexpr float kAlcanceBarricada = 1.6f;  // Distancia al centro de la barricada para golpearla
constexpr std::uint32_t kFuenteRuido = 0xFFFFFFFFu;  // Emisor de una evidencia de ruido (desconocido)
constexpr float kConfianzaAvistamiento = 1.0f;  // ver a Mara directamente es evidencia segura

float longitud(Vec2 v) noexcept { return std::sqrt(v.x * v.x + v.y * v.y); }
float distancia(Vec2 a, Vec2 b) noexcept { return longitud({b.x - a.x, b.y - a.y}); }
Vec2 normalizar(Vec2 v) noexcept {
    const float l = longitud(v);
    return l > 1e-6f ? Vec2{v.x / l, v.y / l} : Vec2{0.0f, 0.0f};
}
bool operator==(Vec2 a, Vec2 b) noexcept { return a.x == b.x && a.y == b.y; }

/// Celdas alcanzables desde @p inicio (BFS 8-conexo, mismas reglas que el grafo). Solo al crear.
std::vector<mundo::Nodo> alcanzables(const mundo::GrafoRejilla& g, mundo::Nodo inicio) {
    std::vector<mundo::Nodo> orden;
    std::vector<bool> visto(g.numCeldas(), false);
    orden.push_back(inicio);
    visto[inicio] = true;
    for (std::size_t i = 0; i < orden.size(); ++i) {
        g.paraCadaVecino(orden[i], [&](mundo::Nodo v, double) {
            if (!visto[v]) {
                visto[v] = true;
                orden.push_back(v);
            }
        });
    }
    return orden;
}
}  // namespace

// --------------------------------------------------------------------------------- Escenario
Escenario crearEscenarioAleatorio(int ancho, int alto, int numInfectados, std::uint64_t semilla) {
    for (int intento = 0; intento < 16; ++intento) {
        nucleo::Pcg32 rng(semilla + static_cast<std::uint64_t>(intento) * 7919u, 1u);
        Escenario e;
        e.mapa = experimentos::generarMapa({ancho, alto, 0.22, 0.10}, rng);
        e.mapa.fijarMultiplicador(1, 1, mundo::kCostoMin);
        e.mapa.fijarMultiplicador(0, 1, mundo::kCostoMin);
        e.mapa.fijarMultiplicador(1, 0, mundo::kCostoMin);
        const mundo::Nodo inicio = e.mapa.nodo(1, 1);
        const auto region = alcanzables(e.mapa, inicio);
        if (region.size() < static_cast<std::size_t>(ancho * alto) / 3) continue;  // ciudad demasiado partida

        e.inicioMara = centroDe(e.mapa.celda(inicio));
        auto lejos = [&](mundo::Nodo n, float minimo) {
            return distancia(centroDe(e.mapa.celda(n)), e.inicioMara) >= minimo;
        };
        auto elegir = [&](float minimo) -> Celda {
            for (int k = 0; k < 4096; ++k) {
                const mundo::Nodo n = region[rng.enteroMenorQue(static_cast<std::uint32_t>(region.size()))];
                if (lejos(n, minimo)) return e.mapa.celda(n);
            }
            return e.mapa.celda(region.back());
        };
        const float diag = std::sqrt(static_cast<float>(ancho * ancho + alto * alto));
        for (int t = 0; t < kNumTiposObjeto; ++t) e.objetos.push_back({static_cast<TipoObjeto>(t), elegir(diag * 0.35f), false});
        e.radio = elegir(diag * 0.5f);
        for (int z = 0; z < numInfectados; ++z) e.infectados.push_back(centroDe(elegir(12.0f)));
        // RF-009: zona de extracción lejos de la radio, para que haya que cruzar la ciudad.
        e.extraccion = e.mapa.celda(inicio);
        float mejor = -1.0f;
        for (int k = 0; k < 64; ++k) {
            const Celda c = e.mapa.celda(region[rng.enteroMenorQue(static_cast<std::uint32_t>(region.size()))]);
            const float d = distancia(centroDe(c), centroDe(e.radio));
            if (d > mejor && distancia(centroDe(c), e.inicioMara) >= diag * 0.25f) {
                mejor = d;
                e.extraccion = c;
            }
        }
        // HU-13: el superviviente empieza en el refugio y el almacén de tablas queda a 5-10 celdas.
        e.inicioSuperviviente = centroDe(e.radio);
        for (int k = 0; k < 256; ++k) {
            const Celda c = e.mapa.celda(region[rng.enteroMenorQue(static_cast<std::uint32_t>(region.size()))]);
            const float d = distancia(centroDe(c), centroDe(e.radio));
            if (d < 5.0f || d > 10.0f) continue;
            if (std::any_of(e.objetos.begin(), e.objetos.end(), [&](const ObjetoMision& o) { return o.celda == c; })) continue;
            e.almacen = c;
            break;
        }
        return e;
    }
    throw std::runtime_error("crearEscenarioAleatorio: no se encontró un mapa jugable");
}

// ----------------------------------------------------------------------------------- Partida
Partida::Partida(Escenario esc, const ConfigPartida& cfg, std::uint64_t semilla)
    : cfg_(cfg),
      dt_(1.0f / static_cast<float>(cfg.tickSimHz)),
      pasosPorTickIa_(std::max(1, cfg.tickSimHz / std::max(1, cfg.tickIaHz))),
      mapa_(std::move(esc.mapa)),
      objetos_(std::move(esc.objetos)),
      radio_(esc.radio),
      semilla_(semilla),
      rng_(semilla, 7u),
      propagador_(cfg.alertas),
      quadtree_({0.0f, 0.0f, static_cast<float>(mapa_.ancho()), static_cast<float>(mapa_.alto())}) {
    mara_.pos = esc.inicioMara;
    mara_.salud = cfg_.saludMax;
    mara_.senuelos = cfg_.senuelosIniciales;
    extraccion_ = esc.extraccion.x >= 0 ? esc.extraccion : celdaDe(esc.inicioMara);
    integridad_ = std::min(cfg_.integridadInicial, cfg_.integridadMax);
    mara_.barricadas = cfg_.barricadasIniciales;
    // Superviviente de la radio (HU-13)
    sup_.activo = esc.inicioSuperviviente.x >= 0.0f;
    sup_.pos = esc.inicioSuperviviente;
    almacen_ = esc.almacen.x >= 0 && mapa_.transitable(esc.almacen.x, esc.almacen.y) ? esc.almacen : Celda{-1, -1};
    sup_.ruta.reserve(mapa_.numCeldas());
    construirArbolSuperviviente();
    const auto ancho = static_cast<std::size_t>(mapa_.ancho());
    const auto alto = static_cast<std::size_t>(mapa_.alto());
    const std::size_t bloques = ((ancho + kBloqueVisitasU - 1) / kBloqueVisitasU) * ((alto + kBloqueVisitasU - 1) / kBloqueVisitasU);
    infectados_.resize(esc.infectados.size());
    for (std::size_t i = 0; i < infectados_.size(); ++i) {
        auto& z = infectados_[i];
        z.pos = esc.infectados[i];
        // Orientación inicial variada y determinista.
        const float a = static_cast<float>(rng_.real01()) * 6.2831853f;
        z.orientacion = {std::cos(a), std::sin(a)};
        z.ruta.reserve(mapa_.numCeldas());
        z.visitas = std::vector<std::uint8_t>(bloques, std::uint8_t{0});
    }
    vecinos_.reserve(infectados_.size() + 8);
    // Calentamiento: la primera búsqueda dimensiona los arreglos del buscador y del resultado.
    // Toda la memoria de trabajo se reserva aquí, en la carga (RNF-006).
    resultado_.ruta.reserve(mapa_.numCeldas());
    resultado_.ordenExpansion.reserve(mapa_.numCeldas());  // RF-071: expansión del agente seleccionado
    expansionSel_.reserve(mapa_.numCeldas());
    buscador_.reservar(mapa_.numCeldas());
    quadtree_.reservar(infectados_.size());
    propagador_.reservar(infectados_.size(), propagacion_);
    posiciones_.reserve(infectados_.size());
    campo_.reservar(mapa_.numCeldas());
    tareas_.reserve(kMaxTareas);
    postores_.reserve(infectados_.size());
    contrato_.adjudicaciones.reserve(kMaxTareas);
    asignador_.reservar(infectados_.size());
    idsLlegada_.reserve(infectados_.size());
    celdasLlegada_.reserve(infectados_.size());
    asignaciones_.reserve(infectados_.size());
    moviles_.resize(infectados_.size());
    ruidosPendientes_.reserve(kMaxRuidosTick);
    ruidosTick_.reserve(kMaxRuidosTick);
    desdeUltimoRuido_ = cfg_.periodoRuido;
}

void Partida::terminar(EstadoPartida e, MotivoFin m) noexcept {
    if (estado_ != EstadoPartida::EnCurso) return;  // el primero que ocurre decide
    estado_ = e;
    motivoFin_ = m;
}

std::string_view Partida::textoResultado() const noexcept {
    switch (motivoFin_) {
        case MotivoFin::Extraccion: return "Victoria";
        case MotivoFin::Salud: return "Derrota: Mara ha caído";
        case MotivoFin::Refugio: return "Derrota: refugio inutilizado";
        case MotivoFin::Ninguno: break;
    }
    return "En curso";
}

bool Partida::tieneTodosLosObjetos() const noexcept {
    return std::all_of(mara_.inventario.begin(), mara_.inventario.end(), [](bool b) { return b; });
}

std::size_t Partida::numTransiciones() const noexcept {
    return static_cast<std::size_t>(std::min<std::uint64_t>(totalTransiciones_, kCapacidadRegistro));
}

const RegistroTransicion& Partida::ultimaTransicion(std::size_t haceN) const noexcept {
    const std::uint64_t idx = (totalTransiciones_ - 1 - haceN) % kCapacidadRegistro;
    return registro_[static_cast<std::size_t>(idx)];
}

std::size_t Partida::numCambiosPlan() const noexcept {
    return static_cast<std::size_t>(std::min<std::uint64_t>(totalCambiosPlan_, kCapacidadRegistro));
}

const RegistroPlan& Partida::ultimoCambioPlan(std::size_t haceN) const noexcept {
    const std::uint64_t idx = (totalCambiosPlan_ - 1 - haceN) % kCapacidadRegistro;
    return planes_[static_cast<std::size_t>(idx)];
}

std::string_view nombreMotivoPlan(MotivoPlan m) noexcept {
    switch (m) {
        case MotivoPlan::Ruido: return "ruido";
        case MotivoPlan::Senuelo: return "senuelo";
        case MotivoPlan::Alerta: return "alerta";
        case MotivoPlan::Rodeo: return "rodeo";
        case MotivoPlan::AtacarBarricada: return "atacar barricada";
        case MotivoPlan::TacticaInterceptar: return "tactica interceptar";
        case MotivoPlan::TacticaExplorar: return "tactica explorar";
    }
    return "?";
}

void Partida::registrarPlan(std::size_t i, Celda anterior, Celda nuevo, float vAnt, float vNuevo, MotivoPlan m) {
    planes_[static_cast<std::size_t>(totalCambiosPlan_ % kCapacidadRegistro)] =
        RegistroPlan{tick_, static_cast<std::uint16_t>(i), anterior, nuevo, vAnt, vNuevo, m};
    ++totalCambiosPlan_;
    const auto mo = nombreMotivoPlan(m);
    registroIa_.anotar(tick_, TipoEventoIa::Plan, static_cast<std::uint16_t>(i),
                       "Z%u\tanterior=(%d,%d)\tnuevo=(%d,%d)\tmotivo=%.*s\tvalores=%.2f>%.2f", static_cast<unsigned>(i),
                       anterior.x, anterior.y, nuevo.x, nuevo.y, static_cast<int>(mo.size()), mo.data(),
                       static_cast<double>(vAnt), static_cast<double>(vNuevo));
}

std::string_view nombreTipoMensaje(TipoMensaje t) noexcept {
    switch (t) {
        case TipoMensaje::Alerta: return "alerta";
        case TipoMensaje::Anuncio: return "anuncio";
        case TipoMensaje::Puja: return "puja";
        case TipoMensaje::Adjudicacion: return "adjudicacion";
    }
    return "?";
}

std::size_t Partida::numMensajes() const noexcept {
    return static_cast<std::size_t>(std::min<std::uint64_t>(totalMensajes_, kCapacidadMensajes));
}

const RegistroMensaje& Partida::ultimoMensaje(std::size_t haceN) const noexcept {
    return mensajes_[static_cast<std::size_t>((totalMensajes_ - 1 - haceN) % kCapacidadMensajes)];
}

void Partida::registrarMensaje(std::size_t emisor, std::size_t receptor, TipoMensaje t, int salto, float confianza,
                               float costo) {
    mensajes_[static_cast<std::size_t>(totalMensajes_ % kCapacidadMensajes)] =
        RegistroMensaje{tick_,      static_cast<std::uint16_t>(emisor), static_cast<std::uint16_t>(receptor), t,
                        static_cast<std::uint8_t>(salto), confianza, costo};
    ++totalMensajes_;
    const auto tipo = nombreTipoMensaje(t);
    if (costo >= 0.0f)
        registroIa_.anotar(tick_, TipoEventoIa::Mensaje, static_cast<std::uint16_t>(emisor),
                           "Z%u>Z%u\ttipo=%.*s\tsalto=%d\tconfianza=%.2f\tcosto=%.2f", static_cast<unsigned>(emisor),
                           static_cast<unsigned>(receptor), static_cast<int>(tipo.size()), tipo.data(), salto,
                           static_cast<double>(confianza), static_cast<double>(costo));
    else
        registroIa_.anotar(tick_, TipoEventoIa::Mensaje, static_cast<std::uint16_t>(emisor),
                           "Z%u>Z%u\ttipo=%.*s\tsalto=%d\tconfianza=%.2f", static_cast<unsigned>(emisor),
                           static_cast<unsigned>(receptor), static_cast<int>(tipo.size()), tipo.data(), salto,
                           static_cast<double>(confianza));
}

// ------------------------------------------------------------------- Monitoreo (RF-070, RF-071)
int Partida::agenteEn(Vec2 punto, float radio) const noexcept {
    int mejor = -1;
    float dMejor = radio;
    for (std::size_t i = 0; i < infectados_.size(); ++i) {
        const float d = distancia(infectados_[i].pos, punto);
        if (d <= dMejor) {
            dMejor = d;
            mejor = static_cast<int>(i);
        }
    }
    return mejor;
}

void Partida::seleccionar(int agente) {
    seleccionado_ = agente >= 0 && static_cast<std::size_t>(agente) < infectados_.size() ? agente : -1;
    expansionSel_.clear();
    if (seleccionado_ < 0) return;
    // Repite su última consulta en el mapa actual registrando la expansión, para pintarla ya. El
    // resultado es el mismo que obtuvo si el mapa no cambió (A* es determinista); solo el tiempo varía.
    auto& z = infectados_[static_cast<std::size_t>(seleccionado_)];
    if (!z.busqueda.valida) return;
    const Celda o = z.busqueda.origen;
    const Celda d = z.busqueda.destino;
    buscador_.buscarEn(mapa_, mapa_.nodo(o.x, o.y), mapa_.nodo(d.x, d.y), resultado_, true);
    anotarMetricas(static_cast<std::size_t>(seleccionado_), o, d);
}

void Partida::anotarMetricas(std::size_t i, Celda origen, Celda destino) noexcept {
    auto& m = infectados_[i].busqueda;
    m.valida = true;
    m.algoritmo = resultado_.algoritmo;
    m.encontrada = resultado_.encontrada;
    m.costo = resultado_.costo;
    m.expandidos = resultado_.expandidos;
    m.generados = resultado_.generados;
    m.maxAbierta = resultado_.maxAbierta;
    m.largoRuta = static_cast<std::uint32_t>(resultado_.ruta.size());
    m.tiempoUs = resultado_.tiempoUs;
    m.memoriaBytes = resultado_.memoriaBytes;
    m.tick = tick_;
    m.origen = origen;
    m.destino = destino;
    if (static_cast<int>(i) == seleccionado_) {
        expansionSel_.clear();
        expansionSel_.insert(expansionSel_.end(), resultado_.ordenExpansion.begin(),
                             resultado_.ordenExpansion.end());  // cabe: capacidad reservada
    }
}

void Partida::paso(const EntradaJugador& entrada) {
    if (estado_ != EstadoPartida::EnCurso) return;
    mara_.recibioDanio = false;
    moverMara(entrada);
    accionesMara(entrada);
    actualizarSenuelos();
    if (tick_ % static_cast<std::uint64_t>(pasosPorTickIa_) == 0) tickIa();
    golpeandoPuerta_ = 0;
    for (std::size_t i = 0; i < infectados_.size(); ++i) ejecutarInfectado(i);
    moverHorda();
    ejecutarSuperviviente();
    interactuar(entrada);
    // RF-009 / RF-010: con la zona abierta, entrar en ella es la victoria; si no, se va cerrando.
    if (zonaAbierta()) {
        if (distancia(mara_.pos, centroDe(extraccion_)) <= cfg_.radioExtraccion)
            terminar(EstadoPartida::Victoria, MotivoFin::Extraccion);
        restanteExtraccion_ = std::max(0.0f, restanteExtraccion_ - dt_);
    }
    if (mara_.salud <= 0.0f) {
        mara_.salud = 0.0f;
        terminar(EstadoPartida::Derrota, MotivoFin::Salud);  // RF-011
    }
    if (integridad_ <= 0.0f) {
        integridad_ = 0.0f;
        terminar(EstadoPartida::Derrota, MotivoFin::Refugio);  // RF-012
    }
    ++tick_;
}

// ----------------------------------------------------------------------------- Movimiento
bool Partida::libre(Vec2 p) const noexcept {
    const float r = cfg_.radioCuerpo;
    const Celda a = celdaDe({p.x - r, p.y - r});
    const Celda b = celdaDe({p.x + r, p.y + r});
    for (int y = a.y; y <= b.y; ++y)
        for (int x = a.x; x <= b.x; ++x)
            if (!mapa_.transitable(x, y)) return false;
    return true;
}

// Ejes por separado: contra una pared se desliza en vez de quedarse pegado (RF-004).
Vec2 Partida::moverConColision(Vec2 pos, Vec2 delta) const noexcept {
    Vec2 p = pos;
    if (const Vec2 px{p.x + delta.x, p.y}; libre(px)) p = px;
    if (const Vec2 py{p.x, p.y + delta.y}; libre(py)) p = py;
    return p;
}

float Partida::factorTerreno(Vec2 p) const noexcept {
    const Celda c = celdaDe(p);
    if (!mapa_.dentro(c.x, c.y)) return 1.0f;
    const auto m = mapa_.multiplicador(mapa_.nodo(c.x, c.y));
    return m <= 1 ? 1.0f : 1.0f / (1.0f + 0.25f * static_cast<float>(m - 1));
}

void Partida::moverMara(const EntradaJugador& e) {
    Vec2 dir{e.ejeX, e.ejeY};
    if (longitud(dir) > 1.0f) dir = normalizar(dir);
    mara_.modo = e.modo;
    const float vel = e.modo == ModoMovimiento::Sigilo   ? cfg_.velSigilo
                      : e.modo == ModoMovimiento::Correr ? cfg_.velCorrer
                                                         : cfg_.velCaminar;
    const float paso = vel * factorTerreno(mara_.pos) * dt_;
    const Vec2 antes = mara_.pos;
    mara_.pos = moverConColision(mara_.pos, {dir.x * paso, dir.y * paso});
    const bool seMovio = !(mara_.pos == antes);
    if (!seMovio) {
        desdeUltimoRuido_ = cfg_.periodoRuido;  // al volver a moverse hace ruido enseguida
        return;
    }
    mara_.orientacion = normalizar(dir);
    // RF-003: mientras se mueve emite un Ruido cada periodoRuido con radio según el modo.
    if (desdeUltimoRuido_ >= cfg_.periodoRuido - 1e-5f) {
        origenRuido_ = mara_.pos;
        radioRuido_ = e.modo == ModoMovimiento::Sigilo   ? cfg_.ruidoSigilo
                      : e.modo == ModoMovimiento::Correr ? cfg_.ruidoCorrer
                                                         : cfg_.ruidoCaminar;
        if (ruidosPendientes_.size() < kMaxRuidosTick) ruidosPendientes_.push_back({origenRuido_, radioRuido_, false});
        desdeUltimoRuido_ = 0.0f;
    }
    desdeUltimoRuido_ += dt_;
}

void Partida::interactuar(const EntradaJugador& e) {
    if (!e.interactuar) {
        mara_.progresoInstalacion = 0.0f;
        return;
    }
    // RF-005: recoger a distancia <= alcance (centro de la celda del objeto).
    for (auto& o : objetos_) {
        if (o.recogido || distancia(mara_.pos, centroDe(o.celda)) > cfg_.alcanceInteraccion) continue;
        o.recogido = true;
        mara_.inventario[static_cast<std::size_t>(o.tipo)] = true;
        return;  // un objeto por pulsación
    }
    // RF-007 / RF-008: instalar la radio manteniendo interactuar; el daño reinicia el progreso.
    // Con la zona ya abierta no hace falta: se puede volver a activar cuando se cierre.
    const bool enRadio = distancia(mara_.pos, centroDe(radio_)) <= cfg_.alcanceInteraccion;
    if (!enRadio || !tieneTodosLosObjetos() || mara_.recibioDanio || zonaAbierta()) {
        mara_.progresoInstalacion = 0.0f;
        return;
    }
    // Instalar hace ruido cada segundo: la horda se acerca al refugio (STB Escena 4).
    if (mara_.progresoInstalacion == 0.0f) desdeRuidoInstalacion_ = 1.0f;
    desdeRuidoInstalacion_ += dt_;
    if (desdeRuidoInstalacion_ >= 1.0f && cfg_.ruidoInstalacion > 0.0f) {
        if (ruidosPendientes_.size() < kMaxRuidosTick) ruidosPendientes_.push_back({centroDe(radio_), cfg_.ruidoInstalacion, false});
        desdeRuidoInstalacion_ = 0.0f;
    }
    mara_.progresoInstalacion += dt_ * (sup_.ayudando ? cfg_.factorAyuda : 1.0f);  // HU-13: el superviviente ayuda
    if (mara_.progresoInstalacion >= cfg_.tInstalacion) {  // RF-007: RadioActivada -> RF-009
        mara_.progresoInstalacion = 0.0f;
        restanteExtraccion_ = cfg_.tExtraccion;
        ++activacionesRadio_;
    }
}

// -------------------------------------------------------------------------------- Ciclo de IA
// RF-040: percepción -> memoria -> decisión -> planificación, para cada agente y en ese orden.
void Partida::tickIa() {
    ruidosTick_.clear();
    ruidosTick_.insert(ruidosTick_.end(), ruidosPendientes_.begin(), ruidosPendientes_.end());  // cabe: misma capacidad
    ruidosPendientes_.clear();
    ruidoUltimoTickIa_ = std::any_of(ruidosTick_.begin(), ruidosTick_.end(), [](const EventoRuido& r) { return !r.senuelo; });
    for (std::size_t i = 0; i < infectados_.size(); ++i) percibirYDecidir(i);
    revisarBarricadas();
    repartirLlegadas();
    actualizarCampo();
    planificarRutas();
    decidirSuperviviente();
    // Mapa de visitas propio (RF-045): cada agente marca el bloque en el que está.
    const int bloquesX = (mapa_.ancho() + kBloqueVisitas - 1) / kBloqueVisitas;
    for (auto& z : infectados_) {
        const Celda c = celdaDe(z.pos);
        auto& v = z.visitas[static_cast<std::size_t>((c.y / kBloqueVisitas) * bloquesX + c.x / kBloqueVisitas)];
        if (v < 255) ++v;
    }
}

void Partida::fijarObjetivo(Infectado& z, Celda c) {
    z.objetivo = c;
    z.tieneObjetivo = true;
    z.rutaPendiente = true;
}

bool Partida::celdaLibreAleatoriaCerca(Celda centro, float radio, Celda& salida) {
    const int r = std::max(1, static_cast<int>(radio));
    const auto lado = static_cast<std::uint32_t>(2 * r + 1);
    for (int k = 0; k < 24; ++k) {
        const Celda c{centro.x - r + static_cast<int>(rng_.enteroMenorQue(lado)),
                      centro.y - r + static_cast<int>(rng_.enteroMenorQue(lado))};
        if (!(c == centro) && mapa_.transitable(c.x, c.y)) {
            salida = c;
            return true;
        }
    }
    return false;
}

// RF-045 (primer corte): entre varias celdas candidatas cercanas elige la de menos visitas
// propias y, a igualdad, la más cercana. Solo usa el mapa de visitas del propio agente.
void Partida::elegirObjetivo(std::size_t i) {
    auto& z = infectados_[i];
    const Celda aqui = celdaDe(z.pos);
    const int bloquesX = (mapa_.ancho() + kBloqueVisitas - 1) / kBloqueVisitas;
    float mejor = 1e30f;
    Celda elegida{};
    bool hay = false;
    for (int k = 0; k < 8; ++k) {
        Celda c{};
        if (!celdaLibreAleatoriaCerca(aqui, cfg_.radioExplorar, c)) continue;
        const auto visitas =
            z.visitas[static_cast<std::size_t>((c.y / kBloqueVisitas) * bloquesX + c.x / kBloqueVisitas)];
        const float puntos = 4.0f * static_cast<float>(visitas) + 0.15f * distancia(centroDe(aqui), centroDe(c));
        if (puntos < mejor) {
            mejor = puntos;
            elegida = c;
            hay = true;
        }
    }
    if (hay) fijarObjetivo(z, elegida);
}

void Partida::percibirYDecidir(std::size_t i) {
    auto& z = infectados_[i];
    const Celda aqui = celdaDe(z.pos);
    const Celda celdaMara = celdaDe(mara_.pos);

    // Percepción local (RF-041, RF-042): cono de visión + línea de visión, y oído.
    agentes::PercepcionInfectado p;
    const float dMara = distancia(z.pos, mara_.pos);
    p.veAMara = agentes::ve(mapa_, z.pos, z.orientacion, mara_.pos, cfg_.visionRadio, cfg_.visionAngulo) ||
                (dMara <= cfg_.sentidoCercano && agentes::lineaDeVisionLibre(mapa_, aqui, celdaMara));
    p.distanciaMara = dMara;
    const double ahora = tiempo();
    const float semivida = cfg_.semividaMemoria;
    auto utilidadDe = [&](const agentes::Evidencia& ev) {
        return agentes::utilidad(ev, ahora, distancia(z.pos, centroDe(ev.celda)), semivida);
    };
    // Oído: de todos los ruidos del tick (Mara y señuelos, que el infectado no distingue) se queda
    // con el de mayor utilidad; todos los que oye van a la memoria (RF-043).
    agentes::Evidencia evRuido{};
    float uRuido = -1.0f;
    int ruidosOidos = 0;
    for (const auto& r : ruidosTick_) {
        if (!agentes::oye(mapa_, z.pos, r.origen, r.radio)) continue;
        ++ruidosOidos;
        // El tipo Senuelo solo sirve al registro del docente (RF-072); la decisión es la misma.
        const agentes::Evidencia ev{r.senuelo ? agentes::TipoEvidencia::Senuelo : agentes::TipoEvidencia::Ruido,
                                    celdaDe(r.origen), ahora, cfg_.confianzaRuido, kFuenteRuido};
        z.memoria.insertar(ev, ahora, semivida);
        const float u = utilidadDe(ev);
        if (u > uRuido) {
            uRuido = u;
            evRuido = ev;
        }
    }
    p.oyeRuido = uRuido >= 0.0f;
    // Mensajes recibidos (RF-041 los cuenta como percepción): solo en modo coordinado.
    p.recibeAlerta = cfg_.coordinado && z.alertaPendiente;
    const Celda celdaAlerta = z.celdaAlerta;
    const agentes::Evidencia evAlerta{agentes::TipoEvidencia::Alerta, celdaAlerta, ahora, z.confianzaAlerta, 0};
    if (p.recibeAlerta) z.memoria.insertar(evAlerta, ahora, semivida);
    z.alertaPendiente = false;
    // RF-049: tarea de interceptar o flanquear adjudicada por Contract Net.
    p.tareaInterceptar = cfg_.coordinado && z.tareaPendiente;
    const coordinacion::TipoTarea tipoTarea = z.tipoTareaPendiente;
    const Celda destinoTarea = z.destinoTarea;
    z.tareaPendiente = false;
    // RF-070: lo que el panel enseña como percepción relevante de este ciclo.
    z.percibido = {p.veAMara, dMara, static_cast<std::uint8_t>(std::min(ruidosOidos, 255)), p.recibeAlerta,
                   z.confianzaAlerta, p.tareaInterceptar, static_cast<std::uint8_t>(std::min<std::size_t>(z.memoria.tamano(), 255))};

    // Memoria (primer corte de RF-043): última posición vista de Mara.
    if (p.veAMara) {
        z.recuerdaMara = true;
        z.ultimaPosMara = celdaMara;
        z.memoria.insertar({agentes::TipoEvidencia::Avistamiento, celdaMara, ahora, kConfianzaAvistamiento,
                            static_cast<std::uint32_t>(i)},
                           ahora, semivida);
        if (z.avistamientos == 0 || z.tickVista != tick_) {  // RF-053: dos avistamientos para el rumbo
            z.vistaPrevia = z.vista;
            z.tickVistaPrevia = z.tickVista;
            z.vista = mara_.pos;
            z.tickVista = tick_;
            ++z.avistamientos;
        }
    } else if (p.recibeAlerta) {
        z.recuerdaMara = true;  // evidencia indirecta: la posición que avisó otro infectado
        z.ultimaPosMara = celdaAlerta;
    }

    // RF-054: táctica según la mejor evidencia de su memoria.
    if (cfg_.tacticas) {
        evaluarTactica(z);
        p.tactica = z.tactica;
    }

    // Decisión (RF-044).
    agentes::ContextoFsm c;
    c.tiempoEnEstado = z.tiempoEnEstado;
    c.llegoAlObjetivo = !z.tieneObjetivo || (aqui == z.objetivo && z.indiceRuta >= z.ruta.size());
    // Golpeando la puerta del refugio da por alcanzado un objetivo que esté junto a ella.
    if (z.golpeaPuerta && distancia(centroDe(z.objetivo), centroDe(radio_)) <= 1.5f) c.llegoAlObjetivo = true;
    c.sinRuta = z.sinRuta;
    c.rutaBloqueada = z.rutaBloqueada;
    c.barricadaDestruida = z.barricadaDestruida;
    z.rutaBloqueada = false;
    z.barricadaDestruida = false;
    const auto t = agentes::decidirTransicion(z.estado, p, c, cfg_.fsm);
    const bool entra = t.motivo != MotivoTransicion::Ninguno;
    if (entra) {
        registrar(i, z.estado, t.hacia, t.motivo);
        z.estado = t.hacia;
        z.tiempoEnEstado = 0.0f;
        z.usaCampo = false;  // actualizarCampo() decide de nuevo en este tick
        if (t.hacia != EstadoInfectado::Interceptar) z.tieneTarea = false;
        if (t.hacia != EstadoInfectado::Investigar) z.tieneEvidencia = false;
    }
    // RF-047: quien ve a Mara (evidencia directa, confianza 1) avisa a los cercanos, con pausa.
    if (p.veAMara && cfg_.coordinado && z.desdeUltimaAlerta >= cfg_.periodoAlerta &&
        kConfianzaAvistamiento >= cfg_.confianzaMinimaAlerta)
        emitirAlerta(i);
    const bool sinRuta = z.sinRuta;
    z.sinRuta = false;

    // Planificación: qué objetivo necesita el estado actual.
    using E = EstadoInfectado;
    switch (z.estado) {
        case E::Explorar:
            if (entra || c.llegoAlObjetivo || sinRuta) {
                const Celda antes = z.objetivo;
                elegirObjetivo(i);
                if (entra && t.motivo == MotivoTransicion::EvidenciaDebil)
                    registrarPlan(i, antes, z.objetivo, z.puntuaciones[0], z.puntuaciones[2], MotivoPlan::TacticaExplorar);
            }
            break;
        case E::Investigar: {
            if (entra && t.motivo == MotivoTransicion::BarricadaDestruida) {  // vuelve a lo que hacía
                fijarObjetivo(z, z.objetivoPrevio);
                break;
            }
            // RF-050: candidato = la alerta o el ruido de mayor utilidad. Al entrar manda la regla
            // de la FSM (alerta antes que ruido); después solo cambia si supera al objetivo actual.
            const bool hayAlerta = p.recibeAlerta;
            const bool hayRuido = p.oyeRuido;
            if (!hayAlerta && !hayRuido) break;
            const float uAlerta = hayAlerta ? utilidadDe(evAlerta) : -1.0f;
            const bool eligeAlerta = entra ? hayAlerta : uAlerta >= uRuido;
            const agentes::Evidencia& cand = eligeAlerta ? evAlerta : evRuido;
            const float uCand = eligeAlerta ? uAlerta : uRuido;
            if (!entra) {
                const float uActual = z.tieneEvidencia ? utilidadDe(z.evidenciaObjetivo) : 0.0f;
                if (uCand <= uActual) break;
                if (!(cand.celda == z.objetivo)) {
                    const MotivoPlan m = eligeAlerta                                           ? MotivoPlan::Alerta
                                         : cand.tipo == agentes::TipoEvidencia::Senuelo ? MotivoPlan::Senuelo
                                                                                        : MotivoPlan::Ruido;
                    registrarPlan(i, z.objetivo, cand.celda, uActual, uCand, m);
                }
            }
            z.evidenciaObjetivo = cand;
            z.tieneEvidencia = true;
            fijarObjetivo(z, cand.celda);
            break;
        }
        case E::Perseguir:
            if (entra || (!(celdaMara == z.objetivo) && z.desdeUltimaRuta >= cfg_.periodoReplan))
                fijarObjetivo(z, celdaMara);
            break;
        case E::Buscar:
            if (entra) {
                fijarObjetivo(z, z.recuerdaMara ? z.ultimaPosMara : aqui);
            } else if (c.llegoAlObjetivo || sinRuta) {
                Celda alrededor{};
                if (celdaLibreAleatoriaCerca(z.recuerdaMara ? z.ultimaPosMara : aqui, 3.0f, alrededor))
                    fijarObjetivo(z, alrededor);
            }
            break;
        case E::Atacar:
            z.ruta.clear();
            z.indiceRuta = 0;
            z.rutaPendiente = false;
            break;
        case E::Interceptar:
            if (p.tareaInterceptar) {
                z.tieneTarea = true;
                z.tarea = tipoTarea;
                fijarObjetivo(z, destinoTarea);
            } else if (entra && t.motivo == MotivoTransicion::EligeTactica && z.hayCorte) {
                // RF-053 / RF-054: él mismo eligió cortarle el paso.
                z.tieneTarea = false;
                registrarPlan(i, z.objetivo, z.puntoCorte, z.puntuaciones[0], z.puntuaciones[1], MotivoPlan::TacticaInterceptar);
                fijarObjetivo(z, z.puntoCorte);
            } else if (!z.tieneTarea && z.hayCorte && z.tactica == agentes::Tactica::Interceptar &&
                       !(z.puntoCorte == z.objetivo) && z.desdeUltimaRuta >= cfg_.periodoReplan) {
                fijarObjetivo(z, z.puntoCorte);  // Mara cambió de rumbo o de ritmo
            }
            break;
        case E::AtacarBarricada:
            if (entra) {
                z.objetivoPrevio = z.objetivo;
                fijarObjetivo(z, z.puestoAtaque);
            }
            break;
    }
}

// RF-053 / RF-054: resume la mejor evidencia de la memoria (por utilidad), estima el rumbo de
// Mara con los dos últimos avistamientos propios y puntúa las tres tácticas. Para cambiar de
// táctica la nueva tiene que superar a la actual en histeresisTactica (evita el parpadeo).
void Partida::evaluarTactica(Infectado& z) {
    const double ahora = tiempo();
    const float semivida = cfg_.semividaMemoria;
    agentes::EntradaTactica et;
    float uMejor = -1.0f;
    for (std::size_t k = 0; k < z.memoria.tamano(); ++k) {
        const auto& ev = z.memoria[k];
        const float d = distancia(z.pos, centroDe(ev.celda));
        const float u = agentes::utilidad(ev, ahora, d, semivida);
        if (u <= uMejor) continue;
        uMejor = u;
        et.hayEvidencia = true;
        et.tipo = ev.tipo;
        et.confianza = agentes::confianzaActual(ev, ahora, semivida);
        et.edad = static_cast<float>(ahora - ev.tiempo);
        et.distancia = d;
    }
    z.hayCorte = false;
    if (et.hayEvidencia && et.tipo == agentes::TipoEvidencia::Avistamiento && z.avistamientos >= 2 &&
        z.tickVista > z.tickVistaPrevia) {
        const float entre = static_cast<float>(z.tickVista - z.tickVistaPrevia) * dt_;
        const float edadVista = static_cast<float>(tick_ - z.tickVista) * dt_;
        if (entre <= 1.5f && edadVista <= 3.0f) {
            const Vec2 v{(z.vista.x - z.vistaPrevia.x) / entre, (z.vista.y - z.vistaPrevia.y) / entre};
            const float rapidez = longitud(v);
            if (rapidez >= cfg_.tactica.velocidadMinimaRumbo) {
                et.tieneRumbo = true;
                et.velocidadMara = rapidez;
                const Vec2 estimada{z.vista.x + v.x * edadVista, z.vista.y + v.y * edadVista};
                if (const auto corte = agentes::puntoDeCorte(mapa_, estimada, v, z.pos, cfg_.velPerseguir, cfg_.tactica)) {
                    et.hayCorte = true;
                    et.distanciaCorte = corte->distancia;
                    z.hayCorte = true;
                    z.puntoCorte = corte->celda;
                }
            }
        }
    }
    const auto r = agentes::elegirTactica(et, cfg_.tactica);
    z.puntuaciones = r.puntuacion;
    const auto actual = static_cast<std::size_t>(z.tactica);
    const auto nueva = static_cast<std::size_t>(r.tactica);
    if (r.tactica != z.tactica && (r.puntuacion[actual] <= 0.0f || r.puntuacion[nueva] >= cfg_.histeresisTactica * r.puntuacion[actual]))
        z.tactica = r.tactica;
}

// ------------------------------------------------------------------- Señuelos y barricadas
// RF-013: celda transitable más lejana de la línea de lanzamiento a SENUELO_ALCANCE o menos.
// La línea se recorre con Bresenham desde la celda de Mara y se corta en la primera pared.
Celda Partida::destinoSenuelo(Vec2 apuntar) const noexcept {
    const Celda a = celdaDe(mara_.pos);
    const Celda b = celdaDe(apuntar);
    Celda mejor = a;
    int x = a.x;
    int y = a.y;
    const int dx = std::abs(b.x - a.x);
    const int dy = -std::abs(b.y - a.y);
    const int sx = a.x < b.x ? 1 : -1;
    const int sy = a.y < b.y ? 1 : -1;
    int err = dx + dy;
    while (!(x == b.x && y == b.y)) {
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y += sy;
        }
        if (!mapa_.transitable(x, y)) break;
        if (distancia(centroDe(a), centroDe({x, y})) > cfg_.senueloAlcance) break;
        mejor = {x, y};
    }
    return mejor;
}

void Partida::accionesMara(const EntradaJugador& e) {
    if (e.lanzarSenuelo && mara_.senuelos > 0) {
        for (auto& sn : senuelos_) {
            if (sn.activo) continue;
            sn = Senuelo{destinoSenuelo(e.apuntar), cfg_.senueloRuidos, cfg_.senueloPeriodo, true};
            --mara_.senuelos;
            break;
        }
    }
    if (e.colocarBarricada && mara_.barricadas > 0) {
        // RF-014: celda adyacente (8 direcciones) en el sentido del punto apuntado.
        Vec2 dir{e.apuntar.x - mara_.pos.x, e.apuntar.y - mara_.pos.y};
        if (longitud(dir) < 1e-3f) dir = mara_.orientacion;
        dir = normalizar(dir);
        constexpr float kUmbral = 0.3827f;  // sen 22,5°: reparte el círculo en 8 sectores
        const int dx = dir.x > kUmbral ? 1 : dir.x < -kUmbral ? -1 : 0;
        const int dy = dir.y > kUmbral ? 1 : dir.y < -kUmbral ? -1 : 0;
        const Celda cm = celdaDe(mara_.pos);
        if ((dx != 0 || dy != 0) && colocarBarricadaEn({cm.x + dx, cm.y + dy})) --mara_.barricadas;
    }
}

void Partida::actualizarSenuelos() {
    for (auto& sn : senuelos_) {
        if (!sn.activo) continue;
        sn.desdeRuido += dt_;
        if (sn.ruidosRestantes > 0 && sn.desdeRuido >= cfg_.senueloPeriodo - 1e-4f) {
            if (ruidosPendientes_.size() < kMaxRuidosTick) ruidosPendientes_.push_back({centroDe(sn.celda), cfg_.senueloRadio, true});
            ++ruidosSenuelo_;
            --sn.ruidosRestantes;
            sn.desdeRuido = 0.0f;
        } else if (sn.ruidosRestantes == 0 && sn.desdeRuido >= cfg_.senueloPeriodo - 1e-4f) {
            sn.activo = false;  // se apaga un periodo después del último ruido
        }
    }
}

void Partida::enviarAlerta(std::size_t agente, Celda celda, float confianza) {
    auto& z = infectados_[agente];
    z.alertaPendiente = true;
    z.celdaAlerta = celda;
    z.confianzaAlerta = confianza;
}

int Partida::barricadaEn(Celda c) const noexcept {
    for (std::size_t b = 0; b < barricadas_.size(); ++b)
        if (barricadas_[b].activa && barricadas_[b].celda == c) return static_cast<int>(b);
    return -1;
}

bool Partida::colocarBarricadaEn(Celda c) {
    if (!mapa_.transitable(c.x, c.y) || c == radio_) return false;
    for (const auto& o : objetos_)
        if (!o.recogido && o.celda == c) return false;
    // Libre de agentes: ningún cuerpo (círculo de radioCuerpo) toca la celda.
    auto toca = [&](Vec2 p) {
        const float qx = std::clamp(p.x, static_cast<float>(c.x), static_cast<float>(c.x + 1));
        const float qy = std::clamp(p.y, static_cast<float>(c.y), static_cast<float>(c.y + 1));
        return distancia(p, {qx, qy}) < cfg_.radioCuerpo;
    };
    if (toca(mara_.pos) || (sup_.activo && toca(sup_.pos))) return false;
    for (const auto& z : infectados_)
        if (toca(z.pos)) return false;
    for (auto& b : barricadas_) {
        if (b.activa) continue;
        b = Barricada{c, cfg_.durabilidadBarricada, mapa_.multiplicador(mapa_.nodo(c.x, c.y)), true};
        mapa_.bloquear(c.x, c.y);
        ++cambiosMapa_;  // evento CambioDeMapa (RF-014, RF-025)
        campoSucio_ = true;
        return true;
    }
    return false;
}

void Partida::destruirBarricada(std::size_t b) {
    auto& bar = barricadas_[b];
    bar.activa = false;
    bar.durabilidad = 0.0f;
    mapa_.fijarMultiplicador(bar.celda.x, bar.celda.y, bar.multiplicadorPrevio);
    ++cambiosMapa_;
    campoSucio_ = true;
    for (auto& z : infectados_) {
        if (z.barricada != static_cast<int>(b)) continue;
        z.barricada = -1;
        z.verificarDesvio = false;
        if (z.estado == EstadoInfectado::AtacarBarricada) z.barricadaDestruida = true;
    }
}

// RF-025 / RF-052: el agente solo se entera de una barricada en su ruta si la ve (a radio de
// visión y con línea de visión) o le llega por mensaje; si no, sigue hasta verla. Al verla
// replanifica, y planificarRutas() compara el rodeo con el costo que tenía la ruta sin ella.
void Partida::revisarBarricadas() {
    if (std::none_of(barricadas_.begin(), barricadas_.end(), [](const Barricada& b) { return b.activa; })) return;
    for (std::size_t i = 0; i < infectados_.size(); ++i) {
        auto& z = infectados_[i];
        if (z.verificarDesvio || z.rutaPendiente || z.estado == EstadoInfectado::AtacarBarricada) continue;
        const Celda aqui = celdaDe(z.pos);
        Celda previa = aqui;
        double costo = 0.0;
        for (std::size_t k = z.indiceRuta; k < z.ruta.size(); ++k) {
            const Celda c = mapa_.celda(z.ruta[k]);
            const int b = barricadaEn(c);
            const double paso = (c.x != previa.x && c.y != previa.y) ? 1.4142135623730951 : 1.0;
            costo += paso * static_cast<double>(b >= 0 ? barricadas_[static_cast<std::size_t>(b)].multiplicadorPrevio
                                                       : mapa_.multiplicador(z.ruta[k]));
            if (b >= 0 && (z.barricada != b)) {
                const bool laVe = distancia(z.pos, centroDe(c)) <= cfg_.visionRadio && agentes::lineaDeVisionLibre(mapa_, aqui, c);
                if (!laVe) break;
                // Costo restante de la ruta antigua como si la barricada no estuviera.
                for (std::size_t j = k + 1; j < z.ruta.size(); ++j) {
                    const Celda cj = mapa_.celda(z.ruta[j]);
                    const Celda ci = mapa_.celda(z.ruta[j - 1]);
                    costo += ((cj.x != ci.x && cj.y != ci.y) ? 1.4142135623730951 : 1.0) * static_cast<double>(mapa_.multiplicador(z.ruta[j]));
                }
                z.barricada = b;
                z.costoSinBarricada = costo;
                z.puestoAtaque = previa;
                z.verificarDesvio = true;
                z.rutaPendiente = true;  // RF-025: se encola una búsqueda nueva en este mismo tick
                break;
            }
            previa = c;
        }
    }
}

void Partida::atacarBarricada(Infectado& z) {
    if (z.barricada < 0) return;
    auto& b = barricadas_[static_cast<std::size_t>(z.barricada)];
    const Vec2 centro = centroDe(b.celda);
    z.orientacion = normalizar({centro.x - z.pos.x, centro.y - z.pos.y});
    if (distancia(z.pos, centro) > kAlcanceBarricada) return;
    b.durabilidad -= cfg_.danioBarricada * dt_;  // RF-052: 10 puntos/s por atacante
    if (b.durabilidad <= 0.0f) destruirBarricada(static_cast<std::size_t>(z.barricada));
}

// RF-026: se atienden solicitudes por turno rotativo hasta agotar el presupuesto de nodos.
void Partida::planificarRutas() {
    busquedasTickIa_ = 0;
    nodosTickIa_ = 0;
    tiempoTickIaUs_ = 0.0;
    const std::size_t n = infectados_.size();
    for (std::size_t k = 0; k < n; ++k) {
        const std::size_t i = (inicioCola_ + k) % n;
        auto& z = infectados_[i];
        if (!z.rutaPendiente) continue;
        if (nodosTickIa_ >= static_cast<std::uint32_t>(cfg_.presupuestoNodosTick)) {
            inicioCola_ = i;  // el siguiente tick empieza por quien quedó esperando
            return;
        }
        const Celda c = celdaDe(z.pos);
        const bool registrarExpansion = static_cast<int>(i) == seleccionado_;  // RF-071
        buscador_.buscarEn(mapa_, mapa_.nodo(c.x, c.y), mapa_.nodo(z.objetivo.x, z.objetivo.y), resultado_,
                           registrarExpansion);
        anotarMetricas(i, c, z.objetivo);
        ++busquedasTickIa_;
        ++busquedasTotales_;
        nodosTickIa_ += resultado_.expandidos;
        tiempoTickIaUs_ += resultado_.tiempoUs;
        z.rutaPendiente = false;
        z.desdeUltimaRuta = 0.0f;
        z.ruta.clear();
        z.indiceRuta = 0;
        if (z.verificarDesvio) {
            // RF-052: sin rodeo, o con uno que cuesta más de FACTOR_DESVIO_BARRICADA veces la ruta
            // que tenía, ataca la barricada; si no, sigue por el rodeo. Las dos se registran (RF-072).
            z.verificarDesvio = false;
            const double limite = static_cast<double>(cfg_.factorDesvioBarricada) * z.costoSinBarricada;
            const Celda celdaBarricada = barricadas_[static_cast<std::size_t>(z.barricada)].celda;
            if (!resultado_.encontrada || resultado_.costo > limite) {
                registrarPlan(i, z.objetivo, celdaBarricada, static_cast<float>(z.costoSinBarricada),
                              resultado_.encontrada ? static_cast<float>(resultado_.costo) : -1.0f, MotivoPlan::AtacarBarricada);
                z.rutaBloqueada = true;  // la FSM pasa a AtacarBarricada en el siguiente ciclo
                continue;
            }
            registrarPlan(i, celdaBarricada, z.objetivo, static_cast<float>(z.costoSinBarricada),
                          static_cast<float>(resultado_.costo), MotivoPlan::Rodeo);
        }
        if (resultado_.encontrada) {
            z.costoRuta = resultado_.costo;
            z.ruta.insert(z.ruta.end(), resultado_.ruta.begin(), resultado_.ruta.end());  // cabe: capacidad reservada
            z.indiceRuta = 1;  // la primera celda es donde ya está
        } else {
            z.sinRuta = true;  // RF-027: otro comportamiento en el siguiente tick de IA
        }
    }
    inicioCola_ = n == 0 ? 0 : (inicioCola_ + 1) % n;
}

// --------------------------------------------------------------------------------- Ejecución
// Cada paso: el agente calcula su velocidad deseada (ruta, campo o punto de llegada) y ataca
// si está en alcance. El movimiento lo integra moverHorda() con steering (RF-029).
void Partida::velocidadHacia(Infectado& z, Vec2 meta, float velocidad, float frenado) {
    const Vec2 d{meta.x - z.pos.x, meta.y - z.pos.y};
    const float l = longitud(d);
    if (l <= 1e-4f) {
        z.velDeseada = {};
        return;
    }
    const float v = frenado > 0.0f ? velocidad * std::min(1.0f, l / frenado) : velocidad;
    z.velDeseada = {d.x / l * v, d.y / l * v};
}

Vec2 Partida::puntoDeLlegada(const Infectado& z) const noexcept {
    const Celda cm = celdaDe(mara_.pos);
    const Vec2 dir = normalizar({static_cast<float>(z.llegada.x - cm.x), static_cast<float>(z.llegada.y - cm.y)});
    // Los dos que comparten acceso se colocan a un lado y otro de la dirección del acceso.
    const float lado = z.puesto == 0 ? -0.2f : 0.2f;
    return {mara_.pos.x + dir.x * kRadioLlegada - dir.y * lado, mara_.pos.y + dir.y * kRadioLlegada + dir.x * lado};
}

void Partida::ejecutarInfectado(std::size_t i) {
    auto& z = infectados_[i];
    z.tiempoEnEstado += dt_;
    z.desdeUltimaRuta += dt_;
    z.desdeUltimaAlerta += dt_;
    using E = EstadoInfectado;
    const float factor = factorTerreno(z.pos);
    const float dMara = distancia(z.pos, mara_.pos);
    // RF-063: quien investiga o busca junto a la puerta del refugio la golpea y se queda ahí.
    z.golpeaPuerta = (z.estado == E::Investigar || z.estado == E::Buscar) &&
                     distancia(z.pos, centroDe(radio_)) <= cfg_.alcancePuerta;
    if (z.golpeaPuerta) {
        integridad_ -= cfg_.danioPuerta * dt_;
        ++golpeandoPuerta_;
        const Vec2 puerta = centroDe(radio_);
        z.orientacion = normalizar({puerta.x - z.pos.x, puerta.y - z.pos.y});
        z.velDeseada = {};
        return;
    }
    const bool cerca = z.tieneLlegada && dMara <= cfg_.radioAproximacion;

    if (z.estado == E::Atacar) {
        if (z.tieneLlegada)
            velocidadHacia(z, puntoDeLlegada(z), cfg_.velPerseguir * factor, kFrenado);
        else if (dMara > kDistanciaAtaque)
            velocidadHacia(z, mara_.pos, cfg_.velPerseguir * factor, kFrenado);
        else
            z.velDeseada = {};
        z.orientacion = normalizar({mara_.pos.x - z.pos.x, mara_.pos.y - z.pos.y});
        if (dMara <= cfg_.fsm.alcanceAtaque) {
            mara_.salud -= cfg_.danioPorSegundo * dt_;
            mara_.recibioDanio = true;
        }
        return;
    }

    const float vel = (z.estado == E::Perseguir || z.estado == E::Interceptar) ? cfg_.velPerseguir
                      : z.estado == E::Investigar ? 0.5f * (cfg_.velExplorar + cfg_.velPerseguir)
                                                  : cfg_.velExplorar;
    const float velMax = vel * factor;
    // RF-052: junto a la barricada la golpea; si no, va a su puesto de ataque por la ruta.
    if (z.estado == E::AtacarBarricada && z.barricada >= 0) {
        atacarBarricada(z);
        if (z.barricada >= 0 && distancia(z.pos, centroDe(barricadas_[static_cast<std::size_t>(z.barricada)].celda)) <= kAlcanceBarricada) {
            z.velDeseada = {};
            return;
        }
    }
    // RF-051: cerca de Mara cada perseguidor va a su celda de llegada en vez de a su celda.
    if (z.estado == E::Perseguir && cerca) {
        velocidadHacia(z, puntoDeLlegada(z), velMax, kFrenado);
        return;
    }
    // RF-028: con el campo activo, el siguiente paso sale de la celda actual en O(1).
    if (z.estado == E::Perseguir && z.usaCampo && campoActivo_) {
        const Celda c = celdaDe(z.pos);
        const mundo::Nodo n = mapa_.nodo(c.x, c.y);
        const mundo::Nodo sig = campo_.siguiente(n);
        if (sig != n)
            velocidadHacia(z, centroDe(mapa_.celda(sig)), velMax, 0.0f);
        else if (dMara > kDistanciaAtaque)
            velocidadHacia(z, mara_.pos, velMax, kFrenado);
        else
            z.velDeseada = {};
        return;
    }
    // Seguir la ruta A*: se avanza de punto al acercarse, para que el steering no tenga que clavarlos.
    while (z.indiceRuta < z.ruta.size()) {
        const bool ultimo = z.indiceRuta + 1 == z.ruta.size();
        const Vec2 meta = centroDe(mapa_.celda(z.ruta[z.indiceRuta]));
        if (distancia(z.pos, meta) > (ultimo ? kLlegadaFinal : kAvanceWaypoint)) break;
        ++z.indiceRuta;
    }
    if (z.indiceRuta < z.ruta.size()) {
        const bool ultimo = z.indiceRuta + 1 == z.ruta.size();
        velocidadHacia(z, centroDe(mapa_.celda(z.ruta[z.indiceRuta])), velMax, ultimo ? kFrenado : 0.0f);
        return;
    }
    // Al final de la ruta y con Mara a la vista, el último tramo es directo.
    if (z.estado == E::Perseguir && dMara > kDistanciaAtaque) {
        velocidadHacia(z, mara_.pos, velMax, kFrenado);
        return;
    }
    z.velDeseada = {};
}

// RF-029: seguir + separación + alineación + cohesión + evitación de paredes, con vecinos del
// quadtree (RF-030). Las fuerzas se calculan con las posiciones de este paso y luego se integran.
void Partida::moverHorda() {
    const std::size_t n = infectados_.size();
    if (n == 0) return;
    quadtree_.limpiar();
    for (std::size_t i = 0; i < n; ++i) {
        quadtree_.insertar(static_cast<std::uint32_t>(i), infectados_[i].pos);
        moviles_[i] = {infectados_[i].pos, infectados_[i].vel};
    }
    const auto& sp = cfg_.steering;
    const float radio = std::max(sp.radioVecindad, sp.radioSeparacion);
    for (std::size_t i = 0; i < n; ++i) {
        auto& z = infectados_[i];
        vecinos_.clear();
        quadtree_.consultarRadio(z.pos, radio, vecinos_);
        const Vec2 f = navegacion::fuerzaSteering(static_cast<std::uint32_t>(i), moviles_.data(), vecinos_, z.velDeseada,
                                                  mapa_, cfg_.radioCuerpo, sp);
        const float velMax = longitud(z.velDeseada) > 1e-4f ? longitud(z.velDeseada) : cfg_.velPerseguir * factorTerreno(z.pos);
        z.vel = navegacion::integrarVelocidad(z.vel, f, std::max(velMax, 0.5f), dt_);
        const Vec2 antes = z.pos;
        z.pos = moverConColision(z.pos, {z.vel.x * dt_, z.vel.y * dt_});
        // Contra una pared se anula la componente bloqueada para no acumular velocidad.
        if (z.pos.x == antes.x) z.vel.x = 0.0f;
        if (z.pos.y == antes.y) z.vel.y = 0.0f;
        if (z.estado != EstadoInfectado::Atacar && longitud(z.vel) > 0.05f) z.orientacion = normalizar(z.vel);
    }
}

// RF-047: alerta recursiva acotada por el grafo de comunicación (vecinos a radioComunicacion
// o menos, pedidos al quadtree). Cada receptor la procesa en su siguiente ciclo de IA.
void Partida::emitirAlerta(std::size_t i) {
    auto& z = infectados_[i];
    z.desdeUltimaAlerta = 0.0f;
    posiciones_.clear();
    for (const auto& otro : infectados_) posiciones_.push_back(otro.pos);  // cabe: capacidad reservada
    quadtree_.limpiar();
    for (std::size_t k = 0; k < infectados_.size(); ++k)
        quadtree_.insertar(static_cast<std::uint32_t>(k), infectados_[k].pos);
    propagador_.propagarEn(posiciones_, quadtree_, static_cast<std::uint32_t>(i), kConfianzaAvistamiento, propagacion_);
    const Celda celdaMara = celdaDe(mara_.pos);
    for (const auto& m : propagacion_.mensajes) {
        registrarMensaje(m.emisor, m.receptor, TipoMensaje::Alerta, m.salto, m.confianza);  // RF-073
        auto& r = infectados_[m.receptor];
        if (r.alertaPendiente && r.confianzaAlerta >= m.confianza) continue;  // ya tiene una mejor
        r.alertaPendiente = true;
        r.celdaAlerta = celdaMara;
        r.confianzaAlerta = m.confianza;
    }
    ++alertasEmitidas_;
    mensajesAlerta_ += propagacion_.mensajes.size();
    tickUltimaAlerta_ = tick_;
    if (cfg_.contractNet) contratar(i);
}

Celda Partida::celdaLibreCercana(Celda c, int radioMax) const noexcept {
    c.x = std::clamp(c.x, 0, mapa_.ancho() - 1);
    c.y = std::clamp(c.y, 0, mapa_.alto() - 1);
    if (mapa_.transitable(c.x, c.y)) return c;
    for (int r = 1; r <= radioMax; ++r)
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != r) continue;  // solo el anillo r
                if (mapa_.transitable(c.x + dx, c.y + dy)) return {c.x + dx, c.y + dy};
            }
    return {-1, -1};
}

// RF-049: el que ve a Mara actúa de gestor. Anuncia Interceptar (punto previsto a partir de sus
// dos últimos avistamientos, RF-053), hasta dos Flanquear (accesos del lado opuesto al suyo) y
// Perseguir; los alertados que no persiguen ya pujan con su costo octil y gana la puja más baja.
void Partida::contratar(std::size_t gestor) {
    const auto& g = infectados_[gestor];
    const auto confianzaDe = [this](std::uint32_t agente) {
        for (const auto& m : propagacion_.mensajes)
            if (m.receptor == agente) return m.confianza;
        return 0.0f;
    };
    postores_.clear();
    for (const auto& m : propagacion_.mensajes) {
        const auto& r = infectados_[m.receptor];
        if (r.estado == EstadoInfectado::Perseguir || r.estado == EstadoInfectado::Atacar) continue;
        postores_.push_back({m.receptor, celdaDe(r.pos)});
    }
    if (postores_.empty()) return;

    const Celda cm = celdaDe(mara_.pos);
    tareas_.clear();
    // Interceptar: dónde estará Mara dentro de horizonteIntercepcion segundos si sigue igual.
    if (g.avistamientos >= 2 && g.tickVista > g.tickVistaPrevia) {
        const float t = static_cast<float>(g.tickVista - g.tickVistaPrevia) * dt_;
        const Vec2 v{(g.vista.x - g.vistaPrevia.x) / t, (g.vista.y - g.vistaPrevia.y) / t};
        const float h = cfg_.horizonteIntercepcion;
        const Celda prevista = celdaLibreCercana(celdaDe({mara_.pos.x + v.x * h, mara_.pos.y + v.y * h}), 3);
        if (prevista.x >= 0 && !(prevista == cm)) tareas_.push_back({coordinacion::TipoTarea::Interceptar, prevista});
    }
    // Flanquear: celdas del anillo radioFlanqueo más opuestas al gestor y separadas entre sí.
    const Vec2 haciaGestor = normalizar({g.pos.x - mara_.pos.x, g.pos.y - mara_.pos.y});
    const int r = std::max(1, cfg_.radioFlanqueo);
    for (int k = 0; k < 2; ++k) {
        float mejor = 0.2f;  // como mínimo algo por detrás de Mara respecto al gestor
        Celda elegida{-1, -1};
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
                const Celda c{cm.x + dx, cm.y + dy};
                if (!mapa_.transitable(c.x, c.y)) continue;
                if (k == 1 && tareas_.back().tipo == coordinacion::TipoTarea::Flanquear &&
                    std::max(std::abs(c.x - tareas_.back().destino.x), std::abs(c.y - tareas_.back().destino.y)) < r)
                    continue;
                const Vec2 u = normalizar({static_cast<float>(dx), static_cast<float>(dy)});
                const float puntos = -(u.x * haciaGestor.x + u.y * haciaGestor.y);
                if (puntos > mejor) {
                    mejor = puntos;
                    elegida = c;
                }
            }
        if (elegida.x < 0) break;
        tareas_.push_back({coordinacion::TipoTarea::Flanquear, elegida});
    }
    tareas_.push_back({coordinacion::TipoTarea::Perseguir, cm});

    coordinacion::ejecutarContractNet(tareas_, postores_, contrato_);
    // RF-073: anuncio gestor -> postor y puja postor -> gestor por cada tarea, y una adjudicación por
    // tarea ganada. Son mensajes directos (salto 1) con la confianza de la alerta que lo hizo postor.
    for (const auto& t : tareas_)
        for (const auto& po : postores_) {
            const float conf = confianzaDe(po.agente);
            registrarMensaje(gestor, po.agente, TipoMensaje::Anuncio, 1, conf);
            registrarMensaje(po.agente, gestor, TipoMensaje::Puja, 1, conf,
                             static_cast<float>(coordinacion::costoPuja(po.celda, t.destino)));
        }
    for (const auto& a : contrato_.adjudicaciones)
        registrarMensaje(gestor, a.agente, TipoMensaje::Adjudicacion, 1, confianzaDe(a.agente), static_cast<float>(a.costo));
    for (const auto& a : contrato_.adjudicaciones) {
        auto& z = infectados_[a.agente];
        const auto& t = tareas_[a.tarea];
        if (t.tipo == coordinacion::TipoTarea::Perseguir) {
            z.alertaPendiente = true;  // va a la celda de Mara como con una alerta
            z.celdaAlerta = t.destino;
            continue;
        }
        z.tareaPendiente = true;
        z.tipoTareaPendiente = t.tipo;
        z.destinoTarea = t.destino;
    }
    ++subastas_;
    adjudicaciones_ += contrato_.adjudicaciones.size();
    // Anuncios, pujas y un aviso de adjudicación por tarea adjudicada.
    mensajesContractNet_ += contrato_.anuncios + contrato_.pujas + contrato_.adjudicaciones.size();
}

// RF-051: con dos o más perseguidores se reparten los accesos alrededor de Mara (2 por celda
// como máximo) minimizando la suma de distancias; cada uno se acerca por el suyo.
void Partida::repartirLlegadas() {
    idsLlegada_.clear();
    celdasLlegada_.clear();
    for (std::size_t i = 0; i < infectados_.size(); ++i) {
        auto& z = infectados_[i];
        z.tieneLlegada = false;
        if (z.estado != EstadoInfectado::Perseguir && z.estado != EstadoInfectado::Atacar) continue;
        idsLlegada_.push_back(static_cast<std::uint32_t>(i));
        celdasLlegada_.push_back(celdaDe(z.pos));
    }
    if (idsLlegada_.size() < 2) return;
    asignador_.asignar(mapa_, celdaDe(mara_.pos), idsLlegada_, celdasLlegada_, cfg_.maxAgentesPorCelda, asignaciones_);
    for (std::size_t k = 0; k < asignaciones_.size(); ++k) {
        const auto& a = asignaciones_[k];
        if (!a.asignado) continue;
        auto& z = infectados_[a.agente];
        z.tieneLlegada = true;
        z.llegada = a.acceso;
        z.puesto = 0;
        for (std::size_t j = 0; j < k; ++j)  // el segundo en la misma celda ocupa el otro puesto
            if (asignaciones_[j].asignado && asignaciones_[j].acceso == a.acceso) z.puesto = 1;
    }
}

// RF-028: si FLOWFIELD_UMBRAL o más infectados persiguen a Mara (mismo destino: su celda), se
// calcula un único campo con Dijkstra inverso y esos agentes dejan de pedir rutas A*.
void Partida::actualizarCampo() {
    std::uint32_t perseguidores = 0;
    for (const auto& z : infectados_)
        if (z.estado == EstadoInfectado::Perseguir) ++perseguidores;
    const bool activar = perseguidores >= static_cast<std::uint32_t>(std::max(1, cfg_.flowfieldUmbral));
    if (!activar) {
        if (campoActivo_) {
            for (auto& z : infectados_) {
                if (!z.usaCampo) continue;
                z.usaCampo = false;
                if (z.estado == EstadoInfectado::Perseguir) fijarObjetivo(z, celdaDe(mara_.pos));  // vuelve a A*
            }
        }
        campoActivo_ = false;
        agentesEnCampo_ = 0;
        return;
    }
    const Celda cm = celdaDe(mara_.pos);
    const mundo::Nodo destino = mapa_.nodo(cm.x, cm.y);
    if (!campoActivo_ || campo_.destino() != destino || campoSucio_) {  // Mara cambió de celda o el mapa (RF-025)
        campoSucio_ = false;
        campoActivo_ = campo_.calcular(mapa_, destino);
        ++camposCalculados_;
    }
    agentesEnCampo_ = 0;
    for (auto& z : infectados_) {
        if (z.estado != EstadoInfectado::Perseguir) continue;
        const Celda c = celdaDe(z.pos);
        z.usaCampo = campoActivo_ && campo_.alcanzable(mapa_.nodo(c.x, c.y));
        if (z.usaCampo) {
            z.rutaPendiente = false;  // sin A* individual
            z.ruta.clear();
            z.indiceRuta = 0;
            ++agentesEnCampo_;
        }
    }
}

void Partida::registrar(std::size_t i, EstadoInfectado desde, EstadoInfectado hacia, MotivoTransicion m) {
    registro_[static_cast<std::size_t>(totalTransiciones_ % kCapacidadRegistro)] =
        RegistroTransicion{tick_, static_cast<std::uint16_t>(i), desde, hacia, m};
    ++totalTransiciones_;
    const auto a = agentes::nombreEstado(desde);
    const auto b = agentes::nombreEstado(hacia);
    const auto mo = agentes::nombreMotivo(m);
    registroIa_.anotar(tick_, TipoEventoIa::Transicion, static_cast<std::uint16_t>(i), "Z%u\testado=%.*s>%.*s\tmotivo=%.*s",
                       static_cast<unsigned>(i), static_cast<int>(a.size()), a.data(), static_cast<int>(b.size()), b.data(),
                       static_cast<int>(mo.size()), mo.data());
}

}  // namespace refugio::juego
