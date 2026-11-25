#include "dominio/superviviente/Superviviente.hpp"

#include <algorithm>
#include <cmath>

namespace refugio::superviviente {

std::string_view nombreMeta(Meta m) noexcept {
    switch (m) {
        case Meta::RefugioSeguro: return "RefugioSeguro";
        case Meta::AyudarInstalacion: return "AyudarInstalacion";
        case Meta::PonerseASalvo: return "PonerseASalvo";
        case Meta::Vigilar: return "Vigilar";
    }
    return "?";
}

Hechos hechoDeMeta(Meta m) noexcept {
    switch (m) {
        case Meta::RefugioSeguro: return hecho::PuertaReparada;
        case Meta::AyudarInstalacion: return hecho::Ayudando;
        case Meta::PonerseASalvo: return hecho::ASalvo;
        case Meta::Vigilar: return hecho::Vigilando;
    }
    return 0;
}

std::string_view nombreAccion(Accion a) noexcept {
    switch (a) {
        case Accion::IrAlmacen: return "IrAlmacen";
        case Accion::RecogerTablas: return "RecogerTablas";
        case Accion::ArrancarTablas: return "ArrancarTablas";
        case Accion::IrPuerta: return "IrPuerta";
        case Accion::RepararPuerta: return "RepararPuerta";
        case Accion::AyudarInstalacion: return "AyudarInstalacion";
        case Accion::Vigilar: return "Vigilar";
        case Accion::Huir: return "Huir";
    }
    return "?";
}

std::array<AccionGoap, kNumAcciones> catalogoAcciones(const CostosAcciones& c) noexcept {
    using namespace hecho;
    std::array<AccionGoap, kNumAcciones> a{};
    a[static_cast<std::size_t>(Accion::IrAlmacen)] = {0, EnAlmacen, EnPuerta, c.irAlmacen, c.almacenAccesible};
    a[static_cast<std::size_t>(Accion::RecogerTablas)] = {EnAlmacen, TieneTablas, 0, c.recoger, c.almacenAccesible};
    a[static_cast<std::size_t>(Accion::ArrancarTablas)] = {0, TieneTablas, 0, c.arrancar, true};
    a[static_cast<std::size_t>(Accion::IrPuerta)] = {0, EnPuerta, EnAlmacen, c.irPuerta, true};
    a[static_cast<std::size_t>(Accion::RepararPuerta)] = {EnPuerta | TieneTablas, PuertaReparada, TieneTablas, c.reparar, true};
    a[static_cast<std::size_t>(Accion::AyudarInstalacion)] = {EnPuerta, Ayudando, 0, c.ayudar, true};
    a[static_cast<std::size_t>(Accion::Vigilar)] = {EnPuerta, Vigilando, 0, c.vigilar, true};
    a[static_cast<std::size_t>(Accion::Huir)] = {0, ASalvo, EnPuerta | EnAlmacen, c.huir, true};
    return a;
}

std::array<float, kNumMetas> puntuarMetas(const EntradaUtilidad& e, const ParametrosUtilidad& p) noexcept {
    const float peligro = 1.0f / (1.0f + std::exp(p.pendientePeligro * (e.distanciaAmenaza - p.distanciaPeligro)));
    const float integridad = std::clamp(e.integridad01, 0.0f, 1.0f);
    std::array<float, kNumMetas> u{};
    u[static_cast<std::size_t>(Meta::RefugioSeguro)] = (1.0f - integridad) * (1.0f - peligro);
    u[static_cast<std::size_t>(Meta::AyudarInstalacion)] = e.maraInstalando ? p.ayuda * (1.0f - peligro) : 0.0f;
    u[static_cast<std::size_t>(Meta::PonerseASalvo)] = peligro;
    u[static_cast<std::size_t>(Meta::Vigilar)] = p.vigilar;
    return u;
}

Meta elegirMeta(const std::array<float, kNumMetas>& u, Meta actual, float histeresis) noexcept {
    std::size_t mejor = 0;
    for (std::size_t m = 1; m < kNumMetas; ++m)
        if (u[m] > u[mejor]) mejor = m;
    const auto a = static_cast<std::size_t>(actual);
    return u[mejor] > u[a] + histeresis ? static_cast<Meta>(mejor) : actual;
}

}  // namespace refugio::superviviente
