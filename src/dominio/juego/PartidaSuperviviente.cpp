/// @file PartidaSuperviviente.cpp
/// @brief Superviviente de la radio dentro de la partida (HU-13, RF-060 a RF-064).
///
/// Ciclo en cada tick de IA (RF-040 aplicado al superviviente):
///  1. Percepción: el infectado visible más cercano (360°, con línea de visión) y si Mara instala.
///  2. Decisión (RF-060): curvas de utilidad y meta con histéresis.
///  3. Planificación (RF-061): GOAP si cambió la meta, si terminó el plan sin cumplirla o si el
///     tick anterior falló una acción (RF-064: el plan nuevo llega en el tick siguiente).
///  4. Ejecución (RF-062): el subárbol de la acción en curso fija la actividad; cada paso de
///     simulación la lleva a cabo (moverse, recoger, reparar +8/s, ayudar, vigilar, huir).

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "dominio/juego/Partida.hpp"

namespace refugio::juego {

using agentes::celdaDe;
using agentes::centroDe;
using mundo::Celda;
using superviviente::Accion;
using superviviente::EstadoNodo;

namespace {
constexpr float kAvanceWaypoint = 0.35f;
constexpr float kLlegadaFinal = 0.15f;
constexpr float kEnLugar = 0.75f;  // A esta distancia del centro de la celda ya «está» en ella
constexpr float kGiroVigilar = 1.2f;  // rad/s mientras vigila

float distancia(Vec2 a, Vec2 b) noexcept { return std::hypot(b.x - a.x, b.y - a.y); }
Vec2 normalizar(Vec2 v) noexcept {
    const float l = std::hypot(v.x, v.y);
    return l > 1e-6f ? Vec2{v.x / l, v.y / l} : Vec2{0.0f, 0.0f};
}

/// Claves de las hojas del árbol.
enum class Hoja : std::uint16_t {
    EnAlmacen, EnPuerta, TieneTablas, ReparacionHecha, AmenazaCerca,  // condiciones
    AvanzarAlmacen, AvanzarPuerta, Recoger, Arrancar, Reparar, Ayudar, Vigilar, Alejarse  // acciones
};
constexpr std::uint16_t k(Hoja h) noexcept { return static_cast<std::uint16_t>(h); }
}  // namespace

std::string_view nombreActividad(Actividad a) noexcept {
    switch (a) {
        case Actividad::Ninguna: return "-";
        case Actividad::Mover: return "moverse";
        case Actividad::Recoger: return "recoger tablas";
        case Actividad::Arrancar: return "arrancar tablas";
        case Actividad::Reparar: return "reparar";
        case Actividad::Ayudar: return "ayudar a instalar";
        case Actividad::Vigilar: return "vigilar";
        case Actividad::Huir: return "huir";
    }
    return "?";
}

std::string_view nombreMotivoReplan(MotivoReplan m) noexcept {
    switch (m) {
        case MotivoReplan::Inicio: return "inicio";
        case MotivoReplan::CambioMeta: return "cambio de meta";
        case MotivoReplan::PlanCompletado: return "plan completado";
        case MotivoReplan::AccionFallida: return "accion fallida";
        case MotivoReplan::PrecondicionRota: return "precondicion rota";
        case MotivoReplan::SinPlan: return "sin plan";
    }
    return "?";
}

/// Puente entre el árbol (que solo conoce claves) y el estado de la partida.
class HojasSuperviviente final : public superviviente::EjecutorHojas {
public:
    explicit HojasSuperviviente(Partida& p) noexcept : p_(p) {}
    bool accionFallo{false};  ///< Para distinguir AccionFallida de PrecondicionRota (RF-064).

    bool condicion(std::uint16_t clave) override {
        auto& s = p_.sup_;
        switch (static_cast<Hoja>(clave)) {
            case Hoja::EnAlmacen: return p_.almacen_.x >= 0 && p_.supEn(p_.almacen_);
            case Hoja::EnPuerta: return p_.supEn(p_.radio_);
            case Hoja::TieneTablas: return s.tablas > 0.0f;
            case Hoja::ReparacionHecha: return p_.integridad_ >= p_.cfg_.integridadMax || (s.reparo && s.tablas <= 0.0f);
            case Hoja::AmenazaCerca: return s.distanciaAmenaza < p_.cfg_.distanciaSegura;
            default: return false;
        }
    }

    EstadoNodo accion(std::uint16_t clave) override {
        auto& s = p_.sup_;
        const auto& cfg = p_.cfg_;
        EstadoNodo r = EstadoNodo::EnCurso;
        switch (static_cast<Hoja>(clave)) {
            case Hoja::AvanzarAlmacen:
                r = p_.almacen_.x >= 0 ? p_.avanzarSuperviviente(p_.almacen_) : EstadoNodo::Fallo;
                if (r == EstadoNodo::Fallo) {  // bloqueado: no lo intenta hasta que cambie el mapa
                    s.almacenInaccesible = true;
                    s.cambiosMapaAlmacen = p_.cambiosMapa_;
                }
                break;
            case Hoja::AvanzarPuerta: r = p_.avanzarSuperviviente(p_.radio_); break;
            case Hoja::Recoger:
            case Hoja::Arrancar: {
                const bool recoger = static_cast<Hoja>(clave) == Hoja::Recoger;
                if (s.progreso >= (recoger ? cfg.tRecogerTablas : cfg.tArrancarTablas) - 1e-4f) {
                    s.tablas = cfg.puntosTablas;
                    s.progreso = 0.0f;
                    s.actividad = Actividad::Ninguna;
                    r = EstadoNodo::Exito;
                } else {
                    s.actividad = recoger ? Actividad::Recoger : Actividad::Arrancar;
                }
                break;
            }
            case Hoja::Reparar:
                s.actividad = Actividad::Reparar;
                s.reparo = true;
                break;
            case Hoja::Ayudar:
                if (p_.mara_.progresoInstalacion > 0.0f) s.actividad = Actividad::Ayudar;
                else r = EstadoNodo::Exito;
                break;
            case Hoja::Vigilar: s.actividad = Actividad::Vigilar; break;
            case Hoja::Alejarse: s.actividad = Actividad::Huir; break;
            default: r = EstadoNodo::Fallo; break;
        }
        if (r == EstadoNodo::Fallo) accionFallo = true;
        return r;
    }

private:
    Partida& p_;
};

// RF-062: un subárbol por acción del catálogo GOAP. Se construye una vez, al cargar.
void Partida::construirArbolSuperviviente() {
    auto& a = arbolSup_;
    auto raiz = [&](Accion ac) -> auto& { return raizAccion_[static_cast<std::size_t>(ac)]; };
    raiz(Accion::IrAlmacen) = a.selector({a.condicion(k(Hoja::EnAlmacen)), a.accion(k(Hoja::AvanzarAlmacen))});
    raiz(Accion::RecogerTablas) = a.selector(
        {a.condicion(k(Hoja::TieneTablas)), a.secuencia({a.condicion(k(Hoja::EnAlmacen)), a.accion(k(Hoja::Recoger))})});
    raiz(Accion::ArrancarTablas) = a.selector({a.condicion(k(Hoja::TieneTablas)), a.accion(k(Hoja::Arrancar))});
    raiz(Accion::IrPuerta) = a.selector({a.condicion(k(Hoja::EnPuerta)), a.accion(k(Hoja::AvanzarPuerta))});
    raiz(Accion::RepararPuerta) = a.selector(
        {a.condicion(k(Hoja::ReparacionHecha)),
         a.secuencia({a.condicion(k(Hoja::EnPuerta)), a.condicion(k(Hoja::TieneTablas)), a.accion(k(Hoja::Reparar))})});
    raiz(Accion::AyudarInstalacion) = a.secuencia({a.condicion(k(Hoja::EnPuerta)), a.accion(k(Hoja::Ayudar))});
    raiz(Accion::Vigilar) = a.secuencia({a.condicion(k(Hoja::EnPuerta)), a.accion(k(Hoja::Vigilar))});
    // Decorador: «a salvo» es no tener una amenaza cerca.
    raiz(Accion::Huir) = a.selector({a.inversor(a.condicion(k(Hoja::AmenazaCerca))), a.accion(k(Hoja::Alejarse))});
}

std::size_t Partida::numRegistrosSuperviviente() const noexcept {
    return static_cast<std::size_t>(std::min<std::uint64_t>(totalRegistrosSup_, kCapacidadRegistroSuperviviente));
}

const RegistroSuperviviente& Partida::ultimoRegistroSuperviviente(std::size_t haceN) const noexcept {
    const std::uint64_t idx = (totalRegistrosSup_ - 1 - haceN) % kCapacidadRegistroSuperviviente;
    return registroSup_[static_cast<std::size_t>(idx)];
}

bool Partida::supEn(Celda c) const noexcept { return distancia(sup_.pos, centroDe(c)) <= kEnLugar; }

superviviente::Hechos Partida::hechosSuperviviente() const noexcept {
    namespace h = superviviente::hecho;
    superviviente::Hechos e = 0;
    if (almacen_.x >= 0 && supEn(almacen_)) e |= h::EnAlmacen;
    if (supEn(radio_)) e |= h::EnPuerta;
    if (sup_.tablas > 0.0f) e |= h::TieneTablas;
    if (integridad_ >= cfg_.integridadMax) e |= h::PuertaReparada;
    if (sup_.distanciaAmenaza >= cfg_.distanciaSegura) e |= h::ASalvo;
    return e;
}

void Partida::decidirSuperviviente() {
    if (!sup_.activo) return;
    auto& s = sup_;
    // 1. Percepción local: solo los infectados que ve.
    s.distanciaAmenaza = 1e9f;
    const Celda aqui = celdaDe(s.pos);
    for (const auto& z : infectados_) {
        const float d = distancia(s.pos, z.pos);
        if (d > cfg_.visionSuperviviente || d >= s.distanciaAmenaza) continue;
        if (!agentes::lineaDeVisionLibre(mapa_, aqui, celdaDe(z.pos))) continue;
        s.distanciaAmenaza = d;
        s.amenaza = z.pos;
    }
    const bool maraInstalando =
        mara_.progresoInstalacion > 0.0f && distancia(s.pos, centroDe(radio_)) <= cfg_.visionSuperviviente;
    // 2. Decisión (RF-060)
    s.utilidades = superviviente::puntuarMetas({integridad_ / cfg_.integridadMax, s.distanciaAmenaza, maraInstalando}, cfg_.utilidad);
    const auto nueva = superviviente::elegirMeta(s.utilidades, s.meta, cfg_.utilidad.histeresis);
    if (nueva != s.meta) {
        s.meta = nueva;
        if (!s.replanificar) s.motivo = MotivoReplan::CambioMeta;  // un fallo pendiente conserva su motivo
        s.replanificar = true;
    }
    if (s.almacenInaccesible && cambiosMapa_ != s.cambiosMapaAlmacen) s.almacenInaccesible = false;  // lo vuelve a intentar
    // Plan terminado y la meta sigue sin cumplirse: otra vuelta (p. ej. otra carga de tablas).
    if (!s.replanificar && s.plan.encontrado && s.paso >= s.plan.longitud &&
        (hechosSuperviviente() & superviviente::hechoDeMeta(s.meta)) == 0) {
        s.replanificar = true;
        s.motivo = MotivoReplan::PlanCompletado;
    }
    // 3. Planificación (RF-061)
    if (s.replanificar) planificarSuperviviente();
    // 4. Ejecución (RF-062): las acciones que ya se cumplen se encadenan en el mismo tick.
    HojasSuperviviente hojas(*this);
    while (s.paso < s.plan.longitud) {
        const auto accion = static_cast<Accion>(s.plan.acciones[s.paso]);
        hojas.accionFallo = false;
        const EstadoNodo r = arbolSup_.tick(raizAccion_[static_cast<std::size_t>(accion)], hojas);
        if (r == EstadoNodo::EnCurso) return;
        if (r == EstadoNodo::Fallo) {
            fallarPlanSuperviviente(hojas.accionFallo ? MotivoReplan::AccionFallida : MotivoReplan::PrecondicionRota, accion);
            return;
        }
        ++s.paso;
        s.progreso = 0.0f;
        s.reparo = false;
    }
    s.actividad = Actividad::Ninguna;
}

// RF-064: se descarta el plan; el nuevo se genera en el siguiente tick de IA con el motivo.
void Partida::fallarPlanSuperviviente(MotivoReplan m, Accion a) noexcept {
    auto& s = sup_;
    s.paso = s.plan.longitud;
    s.replanificar = true;
    s.motivo = m;
    s.tickFallo = tick_;
    s.accionFallida = a;
    s.actividad = Actividad::Ninguna;
}

void Partida::planificarSuperviviente() {
    auto& s = sup_;
    superviviente::CostosAcciones c;
    const float vel = std::max(0.1f, cfg_.velSuperviviente);
    c.almacenAccesible = almacen_.x >= 0 && !s.almacenInaccesible;
    c.irAlmacen = almacen_.x >= 0 ? std::max(0.1f, distancia(s.pos, centroDe(almacen_)) / vel) : 1.0f;
    // Volver a la puerta puede ser desde donde está o, si pasa antes por el almacén, desde allí.
    const float vuelta = c.almacenAccesible ? distancia(centroDe(almacen_), centroDe(radio_)) : 0.0f;
    c.irPuerta = std::max(0.1f, std::max(distancia(s.pos, centroDe(radio_)), vuelta) / vel);
    c.recoger = cfg_.tRecogerTablas;
    c.arrancar = cfg_.tArrancarTablas;
    c.reparar = cfg_.puntosTablas / std::max(0.1f, cfg_.reparacionPorSegundo);
    const auto catalogo = superviviente::catalogoAcciones(c);
    s.plan = goap_.planificar(hechosSuperviviente(), superviviente::hechoDeMeta(s.meta), catalogo);
    s.paso = 0;
    s.progreso = 0.0f;
    s.reparo = false;
    s.actividad = Actividad::Ninguna;
    s.replanificar = false;
    ++s.planes;
    registroSup_[static_cast<std::size_t>(totalRegistrosSup_ % kCapacidadRegistroSuperviviente)] = RegistroSuperviviente{
        tick_, s.meta, s.plan.encontrado ? s.motivo : MotivoReplan::SinPlan, s.tickFallo, s.accionFallida, s.plan.longitud, s.plan.costo};
    ++totalRegistrosSup_;
    // RF-064 / RF-072: el plan nuevo también va al registro de IA, con sus acciones.
    const auto& r = registroSup_[static_cast<std::size_t>((totalRegistrosSup_ - 1) % kCapacidadRegistroSuperviviente)];
    char acciones[96];
    int n = 0;
    acciones[0] = '\0';
    for (std::size_t k = 0; k < s.plan.longitud && n >= 0 && static_cast<std::size_t>(n) < sizeof(acciones); ++k) {
        const auto a = superviviente::nombreAccion(static_cast<Accion>(s.plan.acciones[k]));
        n += std::snprintf(acciones + n, sizeof(acciones) - static_cast<std::size_t>(n), "%s%.*s", k == 0 ? "" : ">",
                           static_cast<int>(a.size()), a.data());
    }
    const auto meta = superviviente::nombreMeta(r.meta);
    const auto mo = nombreMotivoReplan(r.motivo);
    registroIa_.anotar(tick_, TipoEventoIa::Superviviente, kAgenteSuperviviente,
                       "S\tmeta=%.*s\tmotivo=%.*s\tplan=%s\tcosto=%.1f", static_cast<int>(meta.size()), meta.data(),
                       static_cast<int>(mo.size()), mo.data(), s.plan.longitud > 0 ? acciones : "-", static_cast<double>(r.costo));
}

// Ruta A* hasta @p destino; se recalcula si cambia el destino o el mapa (CambioDeMapa).
EstadoNodo Partida::avanzarSuperviviente(Celda destino) {
    auto& s = sup_;
    if (!s.hayRuta || !(s.destino == destino) || s.cambiosMapaRuta != cambiosMapa_) {
        const Celda c = celdaDe(s.pos);
        buscadorActivo().buscarEn(mapa_, mapa_.nodo(c.x, c.y), mapa_.nodo(destino.x, destino.y), resultado_);
        ++busquedasTotales_;
        s.destino = destino;
        s.cambiosMapaRuta = cambiosMapa_;
        s.ruta.clear();
        s.indiceRuta = 1;
        s.hayRuta = resultado_.encontrada;
        if (s.hayRuta) s.ruta.insert(s.ruta.end(), resultado_.ruta.begin(), resultado_.ruta.end());  // cabe: reservada
    }
    if (!s.hayRuta) {
        s.actividad = Actividad::Ninguna;
        return EstadoNodo::Fallo;
    }
    s.actividad = Actividad::Mover;
    return EstadoNodo::EnCurso;
}

void Partida::ejecutarSuperviviente() {
    if (!sup_.activo) return;
    auto& s = sup_;
    s.ayudando = false;
    switch (s.actividad) {
        case Actividad::Mover: {
            while (s.indiceRuta < s.ruta.size()) {
                const bool ultimo = s.indiceRuta + 1 == s.ruta.size();
                if (distancia(s.pos, centroDe(mapa_.celda(s.ruta[s.indiceRuta]))) > (ultimo ? kLlegadaFinal : kAvanceWaypoint)) break;
                ++s.indiceRuta;
            }
            if (s.indiceRuta >= s.ruta.size()) break;
            const Vec2 meta = centroDe(mapa_.celda(s.ruta[s.indiceRuta]));
            const Vec2 dir = normalizar({meta.x - s.pos.x, meta.y - s.pos.y});
            const float paso = std::min(cfg_.velSuperviviente * factorTerreno(s.pos) * dt_, distancia(s.pos, meta));
            s.pos = moverConColision(s.pos, {dir.x * paso, dir.y * paso});
            if (dir.x != 0.0f || dir.y != 0.0f) s.orientacion = dir;
            break;
        }
        case Actividad::Recoger:
        case Actividad::Arrancar: s.progreso += dt_; break;
        case Actividad::Reparar: {
            // RF-063: +8 puntos/s sin superar INTEGRIDAD_REFUGIO_MAX, mientras le queden tablas.
            const float d = std::min({cfg_.reparacionPorSegundo * dt_, s.tablas, cfg_.integridadMax - integridad_});
            if (d > 0.0f) {
                integridad_ += d;
                s.tablas -= d;
            }
            const Vec2 puerta = centroDe(radio_);
            if (distancia(s.pos, puerta) > 1e-3f) s.orientacion = normalizar({puerta.x - s.pos.x, puerta.y - s.pos.y});
            break;
        }
        case Actividad::Ayudar: s.ayudando = mara_.progresoInstalacion > 0.0f && supEn(radio_); break;
        case Actividad::Vigilar: {
            const float a = std::atan2(s.orientacion.y, s.orientacion.x) + kGiroVigilar * dt_;
            s.orientacion = {std::cos(a), std::sin(a)};
            break;
        }
        case Actividad::Huir: {
            const Vec2 dir = normalizar({s.pos.x - s.amenaza.x, s.pos.y - s.amenaza.y});
            const float paso = cfg_.velSuperviviente * factorTerreno(s.pos) * dt_;
            s.pos = moverConColision(s.pos, {dir.x * paso, dir.y * paso});
            if (dir.x != 0.0f || dir.y != 0.0f) s.orientacion = dir;
            break;
        }
        case Actividad::Ninguna: break;
    }
}

}  // namespace refugio::juego
