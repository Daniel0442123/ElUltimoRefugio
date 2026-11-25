#include "app/MenuPausa.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace refugio::app {

using config::Accion;
using config::kNumAcciones;
namespace tecla = config::tecla;

namespace {
constexpr int kPrincipalContinuar = 0, kPrincipalGuardar = 1, kPrincipalCargar = 2, kPrincipalOpciones = 3,
              kPrincipalSalir = 4;
constexpr int kOpcionesVolumen = 0, kOpcionesTutorial = 1, kOpcionesControles = 2, kOpcionesVolver = 3;
constexpr int kControlesRestaurar = static_cast<int>(kNumAcciones);
constexpr int kControlesVolver = kControlesRestaurar + 1;
}  // namespace

MenuPausa::MenuPausa(const config::OpcionesJuego& opciones, const config::Controles& controles)
    : opciones_(opciones), controles_(controles) {}

void MenuPausa::abrir(const std::array<std::string, kRanuras>& ranuras) {
    ranuras_ = ranuras;
    mensaje_.clear();
    ir(PantallaMenu::Principal);
}

void MenuPausa::confirmar(PeticionMenu p, const std::array<std::string, kRanuras>& ranuras) {
    ranuras_ = ranuras;
    mensaje_.clear();
    pendiente_ = p;
    anterior_ = PantallaMenu::Cerrado;  // «No» vuelve al juego
    cursorAnterior_ = 0;
    pantalla_ = PantallaMenu::Confirmar;
    cursor_ = 1;  // por defecto «No»
}

void MenuPausa::cerrar() noexcept { pantalla_ = PantallaMenu::Cerrado; }

void MenuPausa::ir(PantallaMenu p, int cursor) {
    if (p != pantalla_) mensaje_.clear();  // el mensaje es de la pantalla en que se produjo
    pantalla_ = p;
    cursor_ = cursor;
}

int MenuPausa::numLineas() const { return static_cast<int>(lineas().size()); }

std::string MenuPausa::titulo() const {
    switch (pantalla_) {
        case PantallaMenu::Principal: return "Pausa";
        case PantallaMenu::Guardar: return "Guardar partida";
        case PantallaMenu::Cargar: return "Cargar partida";
        case PantallaMenu::Opciones: return "Opciones";
        case PantallaMenu::Controles: return "Controles";
        case PantallaMenu::EsperandoTecla:
            return "Pulsa la nueva tecla para «" + std::string(config::textoAccion(static_cast<Accion>(cursor_))) + "» (Esc cancela)";
        case PantallaMenu::Confirmar:
            return pendiente_.tipo == PeticionMenu::Tipo::Salir ? "Hay progreso sin guardar. ¿Salir del juego?"
                                                                : "Hay progreso sin guardar. ¿Cargar la ranura " +
                                                                      std::to_string(pendiente_.ranura) + "?";
        case PantallaMenu::Cerrado: break;
    }
    return {};
}

std::vector<std::string> MenuPausa::lineas() const {
    std::vector<std::string> v;
    switch (pantalla_) {
        case PantallaMenu::Principal: v = {"Continuar", "Guardar partida", "Cargar partida", "Opciones", "Salir del juego"}; break;
        case PantallaMenu::Guardar:
        case PantallaMenu::Cargar:
            for (int i = 0; i < kRanuras; ++i)
                v.push_back("Ranura " + std::to_string(i + 1) + ": " + ranuras_[static_cast<std::size_t>(i)]);
            v.emplace_back("Volver");
            break;
        case PantallaMenu::Opciones: {
            char buf[48];
            std::snprintf(buf, sizeof(buf), "Volumen: < %d %% >", static_cast<int>(std::lround(opciones_.volumen * 100.0f)));
            v.emplace_back(buf);
            v.push_back(std::string("Tutorial: < ") + (opciones_.tutorial ? "activado" : "desactivado") + " >");
            v.emplace_back("Controles...");
            v.emplace_back("Volver");
            break;
        }
        case PantallaMenu::Controles:
        case PantallaMenu::EsperandoTecla:
            for (std::size_t i = 0; i < kNumAcciones; ++i) {
                const auto a = static_cast<Accion>(i);
                v.push_back(std::string(config::textoAccion(a)) + "\t" + config::textoTecla(controles_.tecla(a)));
            }
            v.emplace_back("Restaurar controles por defecto");
            v.emplace_back("Volver");
            break;
        case PantallaMenu::Confirmar: v = {"Sí", "No"}; break;
        case PantallaMenu::Cerrado: break;
    }
    return v;
}

PeticionMenu MenuPausa::pulsar(int t, bool sinGuardar) {
    if (pantalla_ == PantallaMenu::Cerrado) return {};
    if (pantalla_ == PantallaMenu::EsperandoTecla) {
        const auto a = static_cast<Accion>(cursor_);
        pantalla_ = PantallaMenu::Controles;
        if (t == tecla::kEscape) return {};
        if (!config::teclaAsignable(t)) {
            fijarMensaje(config::teclaReservada(t) ? "Esa tecla está reservada para el menú" : "Esa tecla no se puede asignar", true);
            return {};
        }
        const int otra = controles_.accionDe(t);
        const int previa = controles_.tecla(a);
        controles_.asignar(a, t);
        std::string m = std::string(config::textoAccion(a)) + ": " + config::textoTecla(t);
        if (otra >= 0 && otra != cursor_)
            m += " (" + std::string(config::textoAccion(static_cast<Accion>(otra))) + " pasa a " + config::textoTecla(previa) + ")";
        fijarMensaje(m, false);
        return {PeticionMenu::Tipo::OpcionesCambiadas, 0};
    }
    const int n = numLineas();
    switch (t) {
        case tecla::kArriba: cursor_ = (cursor_ + n - 1) % n; return {};
        case tecla::kAbajo: cursor_ = (cursor_ + 1) % n; return {};
        case tecla::kIzquierda: return cambiarValor(-1);
        case tecla::kDerecha: return cambiarValor(+1);
        case tecla::kEnter: return aceptar(sinGuardar);
        case tecla::kEscape:
            switch (pantalla_) {
                case PantallaMenu::Principal: cerrar(); return {PeticionMenu::Tipo::Continuar, 0};
                case PantallaMenu::Guardar: ir(PantallaMenu::Principal, kPrincipalGuardar); return {};
                case PantallaMenu::Cargar: ir(PantallaMenu::Principal, kPrincipalCargar); return {};
                case PantallaMenu::Opciones: ir(PantallaMenu::Principal, kPrincipalOpciones); return {};
                case PantallaMenu::Controles: ir(PantallaMenu::Opciones, kOpcionesControles); return {};
                case PantallaMenu::Confirmar: ir(anterior_, cursorAnterior_); return {};
                default: return {};
            }
        default: return {};
    }
}

PeticionMenu MenuPausa::cambiarValor(int delta) {
    if (pantalla_ == PantallaMenu::Confirmar) {
        cursor_ = cursor_ == 0 ? 1 : 0;
        return {};
    }
    if (pantalla_ != PantallaMenu::Opciones) return {};
    if (cursor_ == kOpcionesVolumen) {
        const int pasos = std::clamp(static_cast<int>(std::lround(opciones_.volumen * 10.0f)) + delta, 0, 10);
        opciones_.volumen = static_cast<float>(pasos) / 10.0f;
        return {PeticionMenu::Tipo::OpcionesCambiadas, 0};
    }
    if (cursor_ == kOpcionesTutorial) {
        opciones_.tutorial = !opciones_.tutorial;
        return {PeticionMenu::Tipo::OpcionesCambiadas, 0};
    }
    return {};
}

PeticionMenu MenuPausa::pedir(PeticionMenu p, bool sinGuardar) {
    if (!sinGuardar) return p;
    pendiente_ = p;
    anterior_ = pantalla_;
    cursorAnterior_ = cursor_;
    ir(PantallaMenu::Confirmar, 1);
    return {};
}

PeticionMenu MenuPausa::aceptar(bool sinGuardar) {
    switch (pantalla_) {
        case PantallaMenu::Principal:
            switch (cursor_) {
                case kPrincipalContinuar: cerrar(); return {PeticionMenu::Tipo::Continuar, 0};
                case kPrincipalGuardar: ir(PantallaMenu::Guardar); return {};
                case kPrincipalCargar: ir(PantallaMenu::Cargar); return {};
                case kPrincipalOpciones: ir(PantallaMenu::Opciones); return {};
                case kPrincipalSalir: return pedir({PeticionMenu::Tipo::Salir, 0}, sinGuardar);
                default: return {};
            }
        case PantallaMenu::Guardar:
            if (cursor_ == kRanuras) {
                ir(PantallaMenu::Principal, kPrincipalGuardar);
                return {};
            }
            return {PeticionMenu::Tipo::Guardar, cursor_ + 1};
        case PantallaMenu::Cargar:
            if (cursor_ == kRanuras) {
                ir(PantallaMenu::Principal, kPrincipalCargar);
                return {};
            }
            return pedir({PeticionMenu::Tipo::Cargar, cursor_ + 1}, sinGuardar);
        case PantallaMenu::Opciones:
            switch (cursor_) {
                case kOpcionesTutorial: return cambiarValor(+1);
                case kOpcionesControles: ir(PantallaMenu::Controles); return {};
                case kOpcionesVolver: ir(PantallaMenu::Principal, kPrincipalOpciones); return {};
                default: return {};
            }
        case PantallaMenu::Controles:
            if (cursor_ == kControlesVolver) {
                ir(PantallaMenu::Opciones, kOpcionesControles);
                return {};
            }
            if (cursor_ == kControlesRestaurar) {
                controles_ = config::Controles{};
                fijarMensaje("Controles por defecto restaurados", false);
                return {PeticionMenu::Tipo::OpcionesCambiadas, 0};
            }
            pantalla_ = PantallaMenu::EsperandoTecla;
            return {};
        case PantallaMenu::Confirmar:
            if (cursor_ == 0) {
                const PeticionMenu p = pendiente_;
                ir(anterior_ == PantallaMenu::Cerrado ? PantallaMenu::Principal : anterior_, cursorAnterior_);
                if (anterior_ == PantallaMenu::Cerrado) cerrar();
                return p;
            }
            ir(anterior_, cursorAnterior_);
            return {};
        default: return {};
    }
}

}  // namespace refugio::app
