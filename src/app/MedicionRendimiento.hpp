#pragma once
/// @file MedicionRendimiento.hpp
/// @brief Modo --medir-rendimiento del juego con ventana: FPS, tiempo de tick, CPU, memoria y
///        arranque (RNF-001, RNF-002, RNF-004, RNF-005, RNF-008; CP-101, CP-102, CP-104, CP-105,
///        CP-108).
///
/// main_juego.cpp juega sola una partida con un guion fijo de movimientos y entrega aquí, frame a
/// frame, cuánto duró cada frame, cuánto tardó la simulación y cuánta CPU y memoria lleva el
/// proceso. Esta clase no usa raylib ni el sistema operativo: solo acumula, calcula percentiles y
/// escribe los resultados, así se puede probar sin ventana (tests/unit/test_medicion_rendimiento.cpp).
///
/// Salida, con el prefijo de --salida (por defecto results/rendimiento/juego):
///   <prefijo>.json          resumen con objetivo, umbral y veredicto de cada RNF;
///   <prefijo>_frames.csv    una fila por frame medido;
///   <prefijo>_segundos.csv  una fila por segundo: CPU y memoria.
/// tools/rendimiento.py junta varias corridas y actualiza el informe y la trazabilidad.

#include <cstdint>
#include <string>
#include <vector>

#include "dominio/juego/Partida.hpp"

namespace refugio::app {

struct ConfigMedicion {
    std::string escenario{"juego"};  ///< Nombre libre: «juego», «horda100», «arranque»…
    int infectados{10};
    double calentamientoSeg{30.0};  ///< CM-04: 30 s que no cuentan.
    double medicionSeg{300.0};      ///< CM-04: 5 min medidos. 0 = solo arranque.
    std::string salida{"results/rendimiento/juego"};
};

/// Lee las opciones del modo. Devuelve false si no está --medir-rendimiento. Lanza
/// std::runtime_error si un valor no es válido.
bool leerConfigMedicion(int argc, char** argv, ConfigMedicion& cfg);

/// Lo que mide main_juego.cpp en un frame (tiempos en milisegundos).
struct MuestraFrame {
    double msFrame{0.0};       ///< Entre el final de un EndDrawing y el del siguiente (incluye VSync).
    double msActualizar{0.0};  ///< Entrada + pasos de simulación del frame.
    double msDibujar{0.0};     ///< Desde BeginDrawing hasta antes de EndDrawing.
    int pasos{0};              ///< Pasos de simulación del frame.
    double msPasoMax{0.0};     ///< El paso más largo del frame (física + IA + eventos).
    double usBusquedaIa{0.0};  ///< Búsquedas del último tick de IA (Partida::tiempoBusquedaUltimoTickIaUs).
    bool reinicio{false};      ///< La partida terminó y se empezó otra en este frame.
};

struct Estadistica {
    std::size_t n{0};
    double media{0.0};
    double min{0.0};
    double p5{0.0};
    double p50{0.0};
    double p95{0.0};
    double p99{0.0};
    double max{0.0};
};

/// Percentiles por rango más cercano (el valor de la muestra en la posición ceil(p·n)).
[[nodiscard]] Estadistica estadistica(std::vector<double> valores);

/// Entrada del guion fijo de la jugadora para el tick @p tick: Mara recorre las 8 direcciones,
/// 2 s cada una, y corre o va en sigilo en algunos tramos. No depende del reloj, así que todas las
/// corridas con la misma semilla juegan igual.
[[nodiscard]] juego::EntradaJugador entradaGuion(std::uint64_t tick, double hz);

class MedicionRendimiento {
public:
    explicit MedicionRendimiento(ConfigMedicion cfg);

    [[nodiscard]] const ConfigMedicion& config() const noexcept { return cfg_; }

    /// Arranque (RNF-008). @p ms desde la entrada a main.
    void fase(const std::string& nombre, double ms);
    void primerFrame(double msDesdeMain, double msDesdeProceso);
    /// Carga de una escena (crear la partida); se llama al empezar y en cada reinicio.
    void escena(double ms);
    /// Datos del entorno para la cabecera: ventana, monitor, mapa.
    void entorno(const std::string& clave, const std::string& valor);

    /// @p t: segundos desde el primer frame. Los frames del calentamiento no cuentan.
    void frame(double t, const MuestraFrame& m);
    /// Cada segundo de reloj (lo decide tocaMuestra) con la CPU y la memoria del proceso.
    [[nodiscard]] bool tocaMuestra(double t) const noexcept;
    void muestra(double t, double cpuSegundos, std::uint64_t memoriaBytes, std::uint64_t memoriaPicoBytes);
    /// Se terminó el periodo de medición.
    [[nodiscard]] bool terminado(double t) const noexcept;

    [[nodiscard]] std::size_t framesMedidos() const noexcept { return frames_.size(); }
    [[nodiscard]] double cpuMediaPct() const noexcept;
    [[nodiscard]] double memoriaPicoMb() const noexcept;
    [[nodiscard]] double fpsP5() const;

    /// Escribe <prefijo>.json, <prefijo>_frames.csv y <prefijo>_segundos.csv. Lanza
    /// std::runtime_error si no puede crearlos. @p meta: líneas clave=valor de app::metadatosEquipo.
    void escribir(const std::vector<std::string>& meta) const;
    /// Texto del resumen para la consola.
    [[nodiscard]] std::string resumenTexto() const;

private:
    struct Segundo {
        double t;
        double cpuPct;
        double memoriaMb;
    };
    [[nodiscard]] std::string json(const std::vector<std::string>& meta) const;
    [[nodiscard]] bool midiendo(double t) const noexcept;

    ConfigMedicion cfg_;
    std::vector<std::pair<std::string, double>> fases_;
    std::vector<std::pair<std::string, std::string>> entorno_;
    double primerFrameMain_{-1.0};
    double primerFrameProceso_{-1.0};
    std::vector<double> escenas_;
    std::vector<MuestraFrame> frames_;
    std::vector<double> tFrames_;
    std::vector<Segundo> segundos_;
    int reinicios_{0};
    // CPU acumulada al empezar la medición y en la última muestra.
    double tInicio_{-1.0};
    double cpuInicio_{0.0};
    double tUltima_{-1.0};
    double cpuUltima_{0.0};
    double proximaMuestra_{0.0};
    std::uint64_t memoriaPico_{0};
};

}  // namespace refugio::app
