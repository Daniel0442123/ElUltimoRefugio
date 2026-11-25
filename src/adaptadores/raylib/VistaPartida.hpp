#pragma once
/// @file VistaPartida.hpp
/// @brief Adaptador raylib del prototipo jugable: mapa, Mara, infectados, objetos y HUD (RF-016, RF-070, RF-071).
///
/// Solo lee la Partida; no la modifica. Los textos se formatean en búferes de pila para que el
/// dibujo tampoco asigne memoria por frame (RNF-006).

#include <raylib.h>

#include "adaptadores/raylib/VistaDepuracion.hpp"
#include "dominio/config/Configuracion.hpp"
#include "dominio/juego/Partida.hpp"
#include "dominio/juego/Tutorial.hpp"

namespace refugio::adaptadores {

/// Dibuja el mundo. Con @p depuracion: conos de visión, rutas de A* y el último ruido (RF-071).
/// Si hay un agente seleccionado se resaltan su ruta y su objetivo y, con @p nodosExpandidos,
/// los nodos que cerró su última búsqueda (RF-071, CP-071).
void dibujarPartida(const juego::Partida& p, const EstiloVista& e, bool depuracion, bool nodosExpandidos = true);

/// Qué muestra la consola del registro de IA (RF-072, RF-073).
enum class ModoConsola : unsigned char { Oculta, Planes, Mensajes, Todo };
[[nodiscard]] ModoConsola siguienteModo(ModoConsola m) noexcept;

/// Consola del registro de IA sobre el mapa: las últimas líneas del mismo texto que va a
/// logs/ia_<semilla>.log, filtradas por @p modo.
void dibujarConsolaIa(const juego::Partida& p, ModoConsola modo, int x, int y, int ancho, int filas);

/// Panel lateral: salud, inventario, instalación, estados de los infectados, métricas de A* y
/// registro de transiciones de la FSM (RF-016, RF-070, RF-072). Con un agente seleccionado, la
/// mitad inferior pasa a ser su monitor: estado, objetivo, percepción, ruta, algoritmo,
/// heurística, costo, nodos expandidos, tiempo de búsqueda y su historial de planes (RF-070).
/// La ayuda del final del panel usa las teclas de @p controles (RNF-012).
void dibujarPanelPartida(const juego::Partida& p, int x, int y, int alto, int fps, const config::Controles& controles);

/// Indicación actual del tutorial (RF-017) centrada arriba del mapa, con su barra de progreso y
/// el botón «Saltar tutorial». Devuelve el rectángulo del botón para que el juego compruebe el
/// clic (vacío si no hay tutorial activo).
Rectangle dibujarTutorial(const juego::Tutorial& t, const config::Controles& controles, int anchoMapa);

/// Mensaje de victoria o derrota sobre el mapa.
void dibujarFinPartida(const juego::Partida& p, int anchoMapa, int altoMapa);

}  // namespace refugio::adaptadores
