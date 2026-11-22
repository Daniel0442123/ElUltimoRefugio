#pragma once
/// @file VistaPartida.hpp
/// @brief Adaptador raylib del prototipo jugable: mapa, Mara, infectados, objetos y HUD (RF-016, RF-070, RF-071).
///
/// Solo lee la Partida; no la modifica. Los textos se formatean en búferes de pila para que el
/// dibujo tampoco asigne memoria por frame (RNF-006).

#include "adaptadores/raylib/VistaDepuracion.hpp"
#include "dominio/juego/Partida.hpp"

namespace refugio::adaptadores {

/// Dibuja el mundo. Con @p depuracion: conos de visión, rutas de A* y el último ruido (RF-071).
void dibujarPartida(const juego::Partida& p, const EstiloVista& e, bool depuracion);

/// Panel lateral: salud, inventario, instalación, estados de los infectados, métricas de A* y
/// registro de transiciones de la FSM (RF-016, RF-070, RF-072).
void dibujarPanelPartida(const juego::Partida& p, int x, int y, int alto, int fps);

/// Mensaje de victoria o derrota sobre el mapa.
void dibujarFinPartida(const juego::Partida& p, int anchoMapa, int altoMapa);

}  // namespace refugio::adaptadores
