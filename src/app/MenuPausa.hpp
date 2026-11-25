#pragma once
/// @file MenuPausa.hpp
/// @brief Menú de pausa: guardar, cargar, opciones y controles (HU-19, RF-090 a RF-092, RNF-012, RNF-014).
///
/// Es solo la lógica: recibe códigos de tecla (los mismos que raylib, ver config::tecla) y
/// devuelve peticiones (guardar en la ranura N, cargar, salir…) que ejecuta el juego. No dibuja
/// ni lee el teclado, así que se puede probar sin ventana.
///
/// Esc abre y cierra el menú (o vuelve atrás), las flechas mueven el cursor y cambian valores y
/// Enter acepta. Cargar o salir con progreso sin guardar pide confirmación (RNF-014).

#include <array>
#include <string>
#include <vector>

#include "dominio/config/Configuracion.hpp"

namespace refugio::app {

enum class PantallaMenu : unsigned char { Cerrado, Principal, Guardar, Cargar, Opciones, Controles, EsperandoTecla, Confirmar };

struct PeticionMenu {
    enum class Tipo : unsigned char { Ninguna, Continuar, Guardar, Cargar, Salir, OpcionesCambiadas };
    Tipo tipo{Tipo::Ninguna};
    int ranura{0};
};

class MenuPausa {
public:
    static constexpr int kRanuras = 3;

    MenuPausa(const config::OpcionesJuego& opciones, const config::Controles& controles);

    /// Abre en la pantalla principal. @p ranuras describe cada ranura («vacía», «tick 1234»…).
    void abrir(const std::array<std::string, kRanuras>& ranuras);
    /// Abre directamente la confirmación de @p p (F9 con progreso sin guardar).
    void confirmar(PeticionMenu p, const std::array<std::string, kRanuras>& ranuras);
    void cerrar() noexcept;
    [[nodiscard]] bool abierto() const noexcept { return pantalla_ != PantallaMenu::Cerrado; }

    /// Procesa una pulsación. @p sinGuardar: hay progreso que se perdería al cargar o salir.
    PeticionMenu pulsar(int tecla, bool sinGuardar);

    /// Mensaje bajo el menú (resultado de guardar o cargar, tecla reservada…).
    void fijarMensaje(std::string m, bool error) {
        mensaje_ = std::move(m);
        mensajeError_ = error;
    }
    void actualizarRanuras(const std::array<std::string, kRanuras>& ranuras) { ranuras_ = ranuras; }

    [[nodiscard]] PantallaMenu pantalla() const noexcept { return pantalla_; }
    [[nodiscard]] int cursor() const noexcept { return cursor_; }
    [[nodiscard]] std::string titulo() const;
    /// Textos de las opciones de la pantalla actual, en orden.
    [[nodiscard]] std::vector<std::string> lineas() const;
    [[nodiscard]] const std::string& mensaje() const noexcept { return mensaje_; }
    [[nodiscard]] bool mensajeEsError() const noexcept { return mensajeError_; }

    [[nodiscard]] const config::OpcionesJuego& opciones() const noexcept { return opciones_; }
    [[nodiscard]] const config::Controles& controles() const noexcept { return controles_; }

private:
    [[nodiscard]] int numLineas() const;
    void ir(PantallaMenu p, int cursor = 0);
    PeticionMenu aceptar(bool sinGuardar);
    PeticionMenu cambiarValor(int delta);
    PeticionMenu pedir(PeticionMenu p, bool sinGuardar);

    PantallaMenu pantalla_{PantallaMenu::Cerrado};
    PantallaMenu anterior_{PantallaMenu::Principal};  ///< A dónde vuelve «No» en la confirmación.
    int cursor_{0};
    int cursorAnterior_{0};
    PeticionMenu pendiente_{};
    std::array<std::string, kRanuras> ranuras_{};
    std::string mensaje_;
    bool mensajeError_{false};
    config::OpcionesJuego opciones_;
    config::Controles controles_;
};

}  // namespace refugio::app
