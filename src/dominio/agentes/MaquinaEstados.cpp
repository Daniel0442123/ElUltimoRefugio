#include "dominio/agentes/MaquinaEstados.hpp"

namespace refugio::agentes {

std::string_view nombreEstado(EstadoInfectado e) noexcept {
    switch (e) {
        case EstadoInfectado::Explorar: return "Explorar";
        case EstadoInfectado::Investigar: return "Investigar";
        case EstadoInfectado::Perseguir: return "Perseguir";
        case EstadoInfectado::Buscar: return "Buscar";
        case EstadoInfectado::Atacar: return "Atacar";
        case EstadoInfectado::Interceptar: return "Interceptar";
        case EstadoInfectado::AtacarBarricada: return "AtacarBarricada";
    }
    return "?";
}

std::string_view nombreMotivo(MotivoTransicion m) noexcept {
    switch (m) {
        case MotivoTransicion::Ninguno: return "-";
        case MotivoTransicion::VeAMara: return "ve a Mara";
        case MotivoTransicion::OyeRuido: return "oye ruido";
        case MotivoTransicion::LlegaAlOrigen: return "llega al origen";
        case MotivoTransicion::PierdeVision: return "pierde la vision";
        case MotivoTransicion::BusquedaAgotada: return "busqueda agotada";
        case MotivoTransicion::EnAlcance: return "en alcance";
        case MotivoTransicion::FueraDeAlcance: return "fuera de alcance";
        case MotivoTransicion::SinRuta: return "sin ruta";
        case MotivoTransicion::RecibeAlerta: return "recibe alerta";
        case MotivoTransicion::AsignaTarea: return "tarea asignada";
        case MotivoTransicion::RutaBloqueada: return "ruta bloqueada";
        case MotivoTransicion::BarricadaDestruida: return "barricada destruida";
    }
    return "?";
}

Transicion decidirTransicion(EstadoInfectado actual, const PercepcionInfectado& p, const ContextoFsm& c,
                             const ParametrosFsm& par) noexcept {
    using E = EstadoInfectado;
    using M = MotivoTransicion;
    const Transicion seguir{actual, M::Ninguno};
    // RF-052: desde cualquier estado de movimiento, una barricada sin rodeo razonable se ataca.
    if (c.rutaBloqueada && actual != E::Atacar && actual != E::AtacarBarricada) return {E::AtacarBarricada, M::RutaBloqueada};

    switch (actual) {
        case E::Explorar:
            if (p.veAMara) return {E::Perseguir, M::VeAMara};
            if (p.tareaInterceptar) return {E::Interceptar, M::AsignaTarea};
            if (p.recibeAlerta) return {E::Investigar, M::RecibeAlerta};  // posición más fiable que un ruido
            if (p.oyeRuido) return {E::Investigar, M::OyeRuido};
            return seguir;  // al llegar elige otra frontera, sin cambiar de estado (RF-045)

        case E::Investigar:
            if (p.veAMara) return {E::Perseguir, M::VeAMara};
            if (p.tareaInterceptar) return {E::Interceptar, M::AsignaTarea};
            if (c.sinRuta) return {E::Explorar, M::SinRuta};
            if (p.recibeAlerta || p.oyeRuido) return seguir;  // nuevo punto: se reorienta sin cambiar de estado
            if (c.llegoAlObjetivo) return {E::Buscar, M::LlegaAlOrigen};
            return seguir;

        case E::Perseguir:
            if (!p.veAMara) return {E::Buscar, M::PierdeVision};
            if (p.distanciaMara <= par.alcanceAtaque) return {E::Atacar, M::EnAlcance};
            if (c.sinRuta) return {E::Explorar, M::SinRuta};
            return seguir;

        case E::Buscar:
            if (p.veAMara) return {E::Perseguir, M::VeAMara};
            if (p.tareaInterceptar) return {E::Interceptar, M::AsignaTarea};
            if (p.recibeAlerta) return {E::Investigar, M::RecibeAlerta};
            if (p.oyeRuido) return {E::Investigar, M::OyeRuido};
            if (c.tiempoEnEstado >= par.tiempoBuscar) return {E::Explorar, M::BusquedaAgotada};
            return seguir;

        case E::Interceptar:
            if (p.veAMara) return {E::Perseguir, M::VeAMara};
            if (c.sinRuta) return {E::Explorar, M::SinRuta};
            if (p.tareaInterceptar) return seguir;  // nuevo punto: se reorienta
            if (c.llegoAlObjetivo) return {E::Buscar, M::LlegaAlOrigen};
            return seguir;

        case E::AtacarBarricada:
            if (p.veAMara && p.distanciaMara <= par.alcanceAtaque) return {E::Atacar, M::EnAlcance};
            if (c.barricadaDestruida) return p.veAMara ? Transicion{E::Perseguir, M::VeAMara} : Transicion{E::Investigar, M::BarricadaDestruida};
            return seguir;

        case E::Atacar:
            if (!p.veAMara) return {E::Buscar, M::FueraDeAlcance};
            if (p.distanciaMara > par.alcanceAtaque + par.histeresisAtaque) return {E::Perseguir, M::FueraDeAlcance};
            return seguir;
    }
    return seguir;
}

}  // namespace refugio::agentes
