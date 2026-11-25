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
      rng_(semilla, 7u),
      propagador_(cfg.alertas),
      quadtree_({0.0f, 0.0f, static_cast<float>(mapa_.ancho()), static_cast<float>(mapa_.alto())}) {
    mara_.pos = esc.inicioMara;
    mara_.salud = cfg_.saludMax;
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
    desdeUltimoRuido_ = cfg_.periodoRuido;
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

void Partida::paso(const EntradaJugador& entrada) {
    if (estado_ != EstadoPartida::EnCurso) return;
    mara_.recibioDanio = false;
    moverMara(entrada);
    if (tick_ % static_cast<std::uint64_t>(pasosPorTickIa_) == 0) tickIa();
    for (std::size_t i = 0; i < infectados_.size(); ++i) ejecutarInfectado(i);
    moverHorda();
    interactuar(entrada);
    if (mara_.salud <= 0.0f) {
        mara_.salud = 0.0f;
        estado_ = EstadoPartida::Derrota;  // RF-011
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
        ruidoPendiente_ = true;
        origenRuido_ = mara_.pos;
        radioRuido_ = e.modo == ModoMovimiento::Sigilo   ? cfg_.ruidoSigilo
                      : e.modo == ModoMovimiento::Correr ? cfg_.ruidoCorrer
                                                         : cfg_.ruidoCaminar;
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
    const bool enRadio = distancia(mara_.pos, centroDe(radio_)) <= cfg_.alcanceInteraccion;
    if (!enRadio || !tieneTodosLosObjetos() || mara_.recibioDanio) {
        mara_.progresoInstalacion = 0.0f;
        return;
    }
    mara_.progresoInstalacion += dt_;
    if (mara_.progresoInstalacion >= cfg_.tInstalacion) estado_ = EstadoPartida::Victoria;  // RF-010
}

// -------------------------------------------------------------------------------- Ciclo de IA
// RF-040: percepción -> memoria -> decisión -> planificación, para cada agente y en ese orden.
void Partida::tickIa() {
    ruidoUltimoTickIa_ = ruidoPendiente_;
    ruidoPendiente_ = false;
    for (std::size_t i = 0; i < infectados_.size(); ++i) percibirYDecidir(i);
    repartirLlegadas();
    actualizarCampo();
    planificarRutas();
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
    p.oyeRuido = ruidoUltimoTickIa_ && agentes::oye(mapa_, z.pos, origenRuido_, radioRuido_);
    // Mensajes recibidos (RF-041 los cuenta como percepción): solo en modo coordinado.
    p.recibeAlerta = cfg_.coordinado && z.alertaPendiente;
    const Celda celdaAlerta = z.celdaAlerta;
    z.alertaPendiente = false;
    // RF-049: tarea de interceptar o flanquear adjudicada por Contract Net.
    p.tareaInterceptar = cfg_.coordinado && z.tareaPendiente;
    const coordinacion::TipoTarea tipoTarea = z.tipoTareaPendiente;
    const Celda destinoTarea = z.destinoTarea;
    z.tareaPendiente = false;

    // Memoria (primer corte de RF-043): última posición vista de Mara.
    if (p.veAMara) {
        z.recuerdaMara = true;
        z.ultimaPosMara = celdaMara;
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

    // Decisión (RF-044).
    agentes::ContextoFsm c;
    c.tiempoEnEstado = z.tiempoEnEstado;
    c.llegoAlObjetivo = !z.tieneObjetivo || (aqui == z.objetivo && z.indiceRuta >= z.ruta.size());
    c.sinRuta = z.sinRuta;
    const auto t = agentes::decidirTransicion(z.estado, p, c, cfg_.fsm);
    const bool entra = t.motivo != MotivoTransicion::Ninguno;
    if (entra) {
        registrar(i, z.estado, t.hacia, t.motivo);
        z.estado = t.hacia;
        z.tiempoEnEstado = 0.0f;
        z.usaCampo = false;  // actualizarCampo() decide de nuevo en este tick
        if (t.hacia != EstadoInfectado::Interceptar) z.tieneTarea = false;
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
            if (entra || c.llegoAlObjetivo || sinRuta) elegirObjetivo(i);
            break;
        case E::Investigar:
            if (p.recibeAlerta)
                fijarObjetivo(z, celdaAlerta);
            else if (p.oyeRuido)
                fijarObjetivo(z, celdaDe(origenRuido_));
            break;
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
            }
            break;
    }
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
        buscador_.buscarEn(mapa_, mapa_.nodo(c.x, c.y), mapa_.nodo(z.objetivo.x, z.objetivo.y), resultado_);
        ++busquedasTickIa_;
        ++busquedasTotales_;
        nodosTickIa_ += resultado_.expandidos;
        tiempoTickIaUs_ += resultado_.tiempoUs;
        z.rutaPendiente = false;
        z.desdeUltimaRuta = 0.0f;
        z.ruta.clear();
        z.indiceRuta = 0;
        if (resultado_.encontrada) {
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
    if (!campoActivo_ || campo_.destino() != destino) {  // solo se recalcula si Mara cambió de celda
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
}

}  // namespace refugio::juego
