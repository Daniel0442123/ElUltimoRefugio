#pragma once
/// @file Partida.hpp
/// @brief Primer prototipo jugable: Mara contra infectados autónomos (HU-01, HU-08, RF-040, RF-044).
///
/// Bucle de paso fijo (ADR-0005): paso() avanza 1/tickSimHz segundos; cada tickSimHz/tickIaHz
/// pasos corre el ciclo de IA de RF-040 (percepción -> memoria -> decisión -> planificación).
/// La ejecución (seguir la ruta, separarse, atacar) corre en cada paso de simulación.
///
/// Todo el estado se reserva al construir la partida; paso() no asigna memoria (RNF-006,
/// comprobado por CP-106). El dominio no conoce raylib: la entrada llega como EntradaJugador.

#include <array>
#include <cstdint>
#include <vector>

#include "dominio/agentes/MaquinaEstados.hpp"
#include "dominio/agentes/Percepcion.hpp"
#include "dominio/mundo/Grafo.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"
#include "dominio/navegacion/Quadtree.hpp"
#include "dominio/nucleo/Pcg32.hpp"

namespace refugio::juego {

using agentes::EstadoInfectado;
using agentes::MotivoTransicion;
using navegacion::Vec2;

enum class ModoMovimiento : std::uint8_t { Sigilo, Caminar, Correr };
enum class EstadoPartida : std::uint8_t { EnCurso, Victoria, Derrota };
enum class TipoObjeto : std::uint8_t { Medicamento, Combustible, Bateria };
inline constexpr int kNumTiposObjeto = 3;

/// Parámetros (valores iniciales de config/ia.json).
struct ConfigPartida {
    int tickSimHz{60};
    int tickIaHz{10};
    // Mara (RF-002, RF-003)
    float velSigilo{1.5f}, velCaminar{3.0f}, velCorrer{6.0f};
    float ruidoSigilo{1.0f}, ruidoCaminar{3.0f}, ruidoCorrer{6.0f};
    float periodoRuido{0.5f};  ///< 2 eventos de ruido por segundo en movimiento (CP-003).
    float saludMax{100.0f};
    float radioCuerpo{0.3f};   ///< Celdas.
    float alcanceInteraccion{1.0f};
    float tInstalacion{8.0f};  ///< Segundos manteniendo interactuar en la radio (RF-007).
    // Infectados (prototipo «perseguidor» de config/ia.json, algo más lento para ser jugable)
    float visionRadio{6.0f};
    float visionAngulo{110.0f};
    float sentidoCercano{1.5f};  ///< A esta distancia y con línea de visión se nota a Mara aunque esté detrás.
    float velExplorar{1.4f};
    float velPerseguir{3.4f};
    float danioPorSegundo{10.0f};
    float periodoReplan{0.3f};   ///< Segundos mínimos entre dos rutas del mismo agente al perseguir.
    float radioExplorar{10.0f};
    int presupuestoNodosTick{4000};  ///< RF-026: nodos expandidos por tick de IA como máximo.
    agentes::ParametrosFsm fsm{};
};

struct Superviviente {
    Vec2 pos{};
    Vec2 orientacion{1.0f, 0.0f};
    ModoMovimiento modo{ModoMovimiento::Caminar};
    float salud{100.0f};
    std::array<bool, kNumTiposObjeto> inventario{};
    float progresoInstalacion{0.0f};
    bool recibioDanio{false};  ///< En este paso; interrumpe la instalación (RF-008).
};

struct ObjetoMision {
    TipoObjeto tipo{};
    mundo::Celda celda{};
    bool recogido{false};
};

struct Infectado {
    Vec2 pos{};
    Vec2 orientacion{1.0f, 0.0f};
    EstadoInfectado estado{EstadoInfectado::Explorar};
    float tiempoEnEstado{0.0f};
    // Memoria mínima (RF-043 completo llega después): última posición conocida de Mara.
    bool recuerdaMara{false};
    mundo::Celda ultimaPosMara{};
    // Planificación
    mundo::Celda objetivo{};
    bool tieneObjetivo{false};
    bool rutaPendiente{false};
    bool sinRuta{false};
    float desdeUltimaRuta{1e9f};
    std::vector<mundo::Nodo> ruta;  ///< Capacidad reservada al crear la partida.
    std::size_t indiceRuta{0};
    std::vector<std::uint8_t> visitas;  ///< Mapa de visitas propio en bloques de 4x4 (RF-045).
};

struct RegistroTransicion {
    std::uint64_t tick{};
    std::uint16_t agente{};
    EstadoInfectado desde{};
    EstadoInfectado hacia{};
    MotivoTransicion motivo{};
};

struct EntradaJugador {
    float ejeX{0.0f};  ///< -1..1
    float ejeY{0.0f};
    ModoMovimiento modo{ModoMovimiento::Caminar};
    bool interactuar{false};  ///< Mantenido.
};

/// Escenario explícito (para pruebas) o generado (crearEscenarioAleatorio).
struct Escenario {
    mundo::GrafoRejilla mapa{1, 1};
    Vec2 inicioMara{};
    std::vector<Vec2> infectados;
    std::vector<ObjetoMision> objetos;
    mundo::Celda radio{};
};

/// Ciudad de generarMapa con Mara en una esquina, los objetos y la radio lejos de ella y
/// @p numInfectados infectados repartidos a no menos de 12 celdas de Mara. Determinista.
[[nodiscard]] Escenario crearEscenarioAleatorio(int ancho, int alto, int numInfectados, std::uint64_t semilla);

class Partida {
public:
    static constexpr std::size_t kCapacidadRegistro = 64;

    Partida(Escenario escenario, const ConfigPartida& cfg = {}, std::uint64_t semilla = 42);

    /// Avanza un paso fijo de simulación. Sin asignaciones de memoria (RNF-006).
    void paso(const EntradaJugador& entrada);

    [[nodiscard]] const mundo::GrafoRejilla& mapa() const noexcept { return mapa_; }
    [[nodiscard]] const Superviviente& mara() const noexcept { return mara_; }
    [[nodiscard]] const std::vector<Infectado>& infectados() const noexcept { return infectados_; }
    [[nodiscard]] const std::vector<ObjetoMision>& objetos() const noexcept { return objetos_; }
    [[nodiscard]] mundo::Celda radio() const noexcept { return radio_; }
    [[nodiscard]] EstadoPartida estado() const noexcept { return estado_; }
    [[nodiscard]] std::uint64_t tick() const noexcept { return tick_; }
    [[nodiscard]] const ConfigPartida& config() const noexcept { return cfg_; }
    [[nodiscard]] float dt() const noexcept { return dt_; }
    [[nodiscard]] bool tieneTodosLosObjetos() const noexcept;

    /// Últimas transiciones de la FSM (anillo; la más reciente en ultimaTransicion(0)).
    [[nodiscard]] std::size_t numTransiciones() const noexcept;
    [[nodiscard]] const RegistroTransicion& ultimaTransicion(std::size_t haceN) const noexcept;
    [[nodiscard]] std::uint64_t totalTransiciones() const noexcept { return totalTransiciones_; }

    /// Métricas del último tick de IA (panel de monitoreo, RF-070).
    [[nodiscard]] std::uint32_t busquedasUltimoTickIa() const noexcept { return busquedasTickIa_; }
    [[nodiscard]] std::uint32_t nodosUltimoTickIa() const noexcept { return nodosTickIa_; }
    [[nodiscard]] double tiempoBusquedaUltimoTickIaUs() const noexcept { return tiempoTickIaUs_; }
    [[nodiscard]] bool huboRuidoUltimoTickIa() const noexcept { return ruidoUltimoTickIa_; }
    [[nodiscard]] float radioUltimoRuido() const noexcept { return radioRuido_; }
    [[nodiscard]] Vec2 origenUltimoRuido() const noexcept { return origenRuido_; }

private:
    void moverMara(const EntradaJugador& e);
    void interactuar(const EntradaJugador& e);
    void tickIa();
    void percibirYDecidir(std::size_t i);
    void elegirObjetivo(std::size_t i);
    void planificarRutas();
    void ejecutarInfectado(std::size_t i);
    void separarInfectados();
    void registrar(std::size_t i, EstadoInfectado desde, EstadoInfectado hacia, MotivoTransicion m);
    void fijarObjetivo(Infectado& z, mundo::Celda c);
    [[nodiscard]] Vec2 moverConColision(Vec2 pos, Vec2 delta) const noexcept;
    [[nodiscard]] bool libre(Vec2 p) const noexcept;
    [[nodiscard]] float factorTerreno(Vec2 p) const noexcept;
    [[nodiscard]] bool celdaLibreAleatoriaCerca(mundo::Celda centro, float radio, mundo::Celda& salida);

    ConfigPartida cfg_;
    float dt_;
    int pasosPorTickIa_;
    mundo::GrafoRejilla mapa_;
    Superviviente mara_;
    std::vector<Infectado> infectados_;
    std::vector<ObjetoMision> objetos_;
    mundo::Celda radio_;
    EstadoPartida estado_{EstadoPartida::EnCurso};
    std::uint64_t tick_{0};
    nucleo::Pcg32 rng_;

    // Ruido de Mara (RF-003): se acumula entre ticks de IA y se consume en el siguiente.
    float desdeUltimoRuido_{0.0f};
    bool ruidoPendiente_{false};
    bool ruidoUltimoTickIa_{false};
    Vec2 origenRuido_{};
    float radioRuido_{0.0f};

    // Planificación con presupuesto (RF-026) y A* compartido.
    navegacion::BuscadorMejorPrimero buscador_{navegacion::Algoritmo::AEstrella};
    navegacion::ResultadoBusqueda resultado_;
    std::size_t inicioCola_{0};  ///< Turno rotativo: los agentes que esperan más se atienden antes.
    std::uint32_t busquedasTickIa_{0};
    std::uint32_t nodosTickIa_{0};
    double tiempoTickIaUs_{0.0};

    // Consulta espacial para la separación (RF-030).
    navegacion::Quadtree quadtree_;
    std::vector<std::uint32_t> vecinos_;

    std::array<RegistroTransicion, kCapacidadRegistro> registro_{};
    std::uint64_t totalTransiciones_{0};
};

}  // namespace refugio::juego
