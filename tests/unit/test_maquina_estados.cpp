#include <catch2/catch_test_macros.hpp>

#include "dominio/agentes/MaquinaEstados.hpp"

using namespace refugio::agentes;
using E = EstadoInfectado;
using M = MotivoTransicion;

namespace {
struct Fila {
    const char* caso;
    E desde;
    PercepcionInfectado p;
    ContextoFsm c;
    E hacia;
    M motivo;
};
}  // namespace

TEST_CASE("CP-044 Transiciones de la FSM del infectado: una fila por transicion", "[RF-044][HU-08]") {
    const ParametrosFsm par{};  // alcance 0.9, histeresis 0.5, buscar 8 s
    const PercepcionInfectado nada{};
    const PercepcionInfectado veLejos{true, 4.0f, false};
    const PercepcionInfectado veCerca{true, 0.5f, false};
    const PercepcionInfectado ruido{false, 0.0f, true};
    const ContextoFsm sinNovedad{};

    const Fila tabla[] = {
        {"Explorar + ve a Mara -> Perseguir", E::Explorar, veLejos, sinNovedad, E::Perseguir, M::VeAMara},
        {"Explorar + ruido -> Investigar", E::Explorar, ruido, sinNovedad, E::Investigar, M::OyeRuido},
        {"Explorar sin nada sigue", E::Explorar, nada, {0, true, false}, E::Explorar, M::Ninguno},
        {"Investigar + ve a Mara -> Perseguir", E::Investigar, veLejos, sinNovedad, E::Perseguir, M::VeAMara},
        {"Investigar llega sin evidencia -> Buscar", E::Investigar, nada, {0, true, false}, E::Buscar, M::LlegaAlOrigen},
        {"Investigar sin ruta -> Explorar (RF-027)", E::Investigar, nada, {0, false, true}, E::Explorar, M::SinRuta},
        {"Perseguir pierde la vision -> Buscar", E::Perseguir, nada, sinNovedad, E::Buscar, M::PierdeVision},
        {"Perseguir a distancia <= 1 -> Atacar", E::Perseguir, veCerca, sinNovedad, E::Atacar, M::EnAlcance},
        {"Perseguir sin ruta -> Explorar (RF-027)", E::Perseguir, veLejos, {0, false, true}, E::Explorar, M::SinRuta},
        {"Perseguir viendo lejos sigue", E::Perseguir, veLejos, sinNovedad, E::Perseguir, M::Ninguno},
        {"Buscar + ve a Mara -> Perseguir", E::Buscar, veLejos, sinNovedad, E::Perseguir, M::VeAMara},
        {"Buscar + ruido -> Investigar", E::Buscar, ruido, sinNovedad, E::Investigar, M::OyeRuido},
        {"Buscar 8 s sin nada -> Explorar", E::Buscar, nada, {8.0f, false, false}, E::Explorar, M::BusquedaAgotada},
        {"Buscar antes de 8 s sigue", E::Buscar, nada, {7.9f, false, false}, E::Buscar, M::Ninguno},
        {"Atacar en alcance sigue", E::Atacar, veCerca, sinNovedad, E::Atacar, M::Ninguno},
        {"Atacar dentro de la histeresis sigue", E::Atacar, {true, 1.3f, false}, sinNovedad, E::Atacar, M::Ninguno},
        {"Atacar y Mara se aleja -> Perseguir", E::Atacar, {true, 1.5f, false}, sinNovedad, E::Perseguir, M::FueraDeAlcance},
        {"Atacar y la pierde de vista -> Buscar", E::Atacar, nada, sinNovedad, E::Buscar, M::FueraDeAlcance},
    };
    for (const auto& f : tabla) {
        INFO(f.caso);
        const auto t = decidirTransicion(f.desde, f.p, f.c, par);
        CHECK(t.hacia == f.hacia);
        CHECK(t.motivo == f.motivo);
    }
}

TEST_CASE("CP-044 Cada estado y motivo tiene nombre para el registro", "[RF-044][RF-072]") {
    for (int e = 0; e <= static_cast<int>(E::Atacar); ++e) CHECK(nombreEstado(static_cast<E>(e)) != "?");
    for (int m = 0; m <= static_cast<int>(M::SinRuta); ++m) CHECK(nombreMotivo(static_cast<M>(m)) != "?");
}
