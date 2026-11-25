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
#include <string_view>
#include <cstdint>
#include <vector>

#include "dominio/agentes/MaquinaEstados.hpp"
#include "dominio/agentes/Memoria.hpp"
#include "dominio/agentes/Tactica.hpp"
#include "dominio/coordinacion/AsignadorAccesos.hpp"
#include "dominio/coordinacion/ContractNet.hpp"
#include "dominio/coordinacion/PropagadorAlertas.hpp"
#include "dominio/agentes/Percepcion.hpp"
#include "dominio/mundo/Grafo.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"
#include "dominio/navegacion/CampoFlujo.hpp"
#include "dominio/navegacion/Quadtree.hpp"
#include "dominio/navegacion/Steering.hpp"
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
    // Coordinación (RF-047, config/ia.json «alertas»)
    bool coordinado{true};             ///< Modo coordinado: los infectados se avisan al ver a Mara.
    coordinacion::ParametrosAlerta alertas{};  ///< radio 5, profundidad 3, atenuación 0,7
    float confianzaMinimaAlerta{0.6f};  ///< RF-047: solo se propaga una evidencia con confianza >= 0,6.
    float periodoAlerta{2.0f};          ///< Segundos entre dos alertas del mismo infectado.
    // Horda (RF-028, config/ia.json «flowfield_umbral»)
    int flowfieldUmbral{8};  ///< Con este número de perseguidores o más se usa un campo de flujo.
    // Horda coordinada (HU-10)
    bool contractNet{true};       ///< RF-049: el que ve a Mara reparte tareas entre los alertados.
    float horizonteIntercepcion{2.0f};  ///< RF-053: segundos hacia delante para predecir a Mara.
    int radioFlanqueo{3};         ///< Celdas (Chebyshev) de los accesos para flanquear.
    int maxAgentesPorCelda{2};    ///< RF-051, MAX_AGENTES_POR_CELDA.
    float radioAproximacion{2.5f};  ///< A esta distancia de Mara cada uno va a su celda de llegada.
    navegacion::ParametrosSteering steering{};  ///< RF-029
    // Herramientas de Mara (RF-013, RF-014) y respuesta de la horda (RF-050, RF-052)
    int senuelosIniciales{2};
    int barricadasIniciales{3};
    float senueloAlcance{8.0f};    ///< SENUELO_ALCANCE, celdas.
    float senueloRadio{8.0f};      ///< SENUELO_RADIO, celdas.
    int senueloRuidos{10};         ///< SENUELO_DURACION / SENUELO_PERIODO = 10 s / 1 s.
    float senueloPeriodo{1.0f};    ///< SENUELO_PERIODO, s.
    float durabilidadBarricada{100.0f};
    float danioBarricada{10.0f};   ///< Puntos por segundo de cada infectado que la ataca.
    float factorDesvioBarricada{2.0f};  ///< FACTOR_DESVIO_BARRICADA
    float confianzaRuido{0.8f};    ///< Confianza de un ruido oído (no se sabe si es Mara o un señuelo).
    float semividaMemoria{agentes::kMemoriaSemivida};  ///< MEMORIA_SEMIVIDA, s.
    // Selección de táctica (RF-053, RF-054)
    bool tacticas{true};
    agentes::ParametrosTactica tactica{};
    float histeresisTactica{1.1f};  ///< Para cambiar de táctica la nueva debe puntuar 10 % más.
};

struct Superviviente {
    Vec2 pos{};
    Vec2 orientacion{1.0f, 0.0f};
    ModoMovimiento modo{ModoMovimiento::Caminar};
    float salud{100.0f};
    std::array<bool, kNumTiposObjeto> inventario{};
    float progresoInstalacion{0.0f};
    bool recibioDanio{false};  ///< En este paso; interrumpe la instalación (RF-008).
    int senuelos{0};
    int barricadas{0};
};

/// Señuelo lanzado (RF-013): hace ruido cada SENUELO_PERIODO hasta agotar sus ruidos.
struct Senuelo {
    mundo::Celda celda{};
    int ruidosRestantes{0};
    float desdeRuido{0.0f};
    bool activo{false};
};

/// Barricada colocada (RF-014): celda bloqueada con durabilidad; al romperse vuelve a ser transitable.
struct Barricada {
    mundo::Celda celda{};
    float durabilidad{0.0f};
    std::uint8_t multiplicadorPrevio{mundo::kCostoMin};
    bool activa{false};
};

/// Evento Ruido (RF-003, RF-013). @c senuelo solo lo usa el registro: los infectados no lo distinguen.
struct EventoRuido {
    Vec2 origen{};
    float radio{0.0f};
    bool senuelo{false};
};

/// Motivo de un cambio de plan (RF-072).
enum class MotivoPlan : std::uint8_t { Ruido, Senuelo, Alerta, Rodeo, AtacarBarricada, TacticaInterceptar, TacticaExplorar };
[[nodiscard]] std::string_view nombreMotivoPlan(MotivoPlan m) noexcept;

/// RF-072: cambio de objetivo o de plan. Para Ruido, Senuelo y Alerta los valores son utilidades
/// (RF-050); para Rodeo y AtacarBarricada son costos de ruta (sin la barricada y con el rodeo);
/// para las tácticas (RF-054) son la puntuación de Perseguir y la de la táctica elegida.
struct RegistroPlan {
    std::uint64_t tick{};
    std::uint16_t agente{};
    mundo::Celda anterior{};
    mundo::Celda nuevo{};
    float valorAnterior{};
    float valorNuevo{};
    MotivoPlan motivo{};
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
    // Coordinación
    bool alertaPendiente{false};  ///< Recibió una alerta; la procesa en su próximo ciclo de IA.
    mundo::Celda celdaAlerta{};
    float confianzaAlerta{0.0f};
    float desdeUltimaAlerta{1e9f};
    bool usaCampo{false};  ///< Persigue siguiendo el campo de flujo compartido en vez de su ruta A*.
    // Contract Net (RF-049): tarea adjudicada pendiente y tarea en curso.
    bool tareaPendiente{false};
    coordinacion::TipoTarea tipoTareaPendiente{};
    mundo::Celda destinoTarea{};
    bool tieneTarea{false};
    coordinacion::TipoTarea tarea{};
    // Últimos dos avistamientos propios de Mara para predecir su rumbo (RF-053).
    Vec2 vista{};
    Vec2 vistaPrevia{};
    std::uint64_t tickVista{0};
    std::uint64_t tickVistaPrevia{0};
    int avistamientos{0};
    // Celda de llegada alrededor de Mara (RF-051) y posición dentro de ella (0 o 1).
    bool tieneLlegada{false};
    mundo::Celda llegada{};
    int puesto{0};
    // Movimiento con steering (RF-029)
    Vec2 vel{};
    Vec2 velDeseada{};
    // Memoria de evidencias (RF-043) y evidencia que justifica el objetivo actual (RF-050).
    agentes::MemoriaEvidencias memoria;
    agentes::Evidencia evidenciaObjetivo{};
    bool tieneEvidencia{false};
    // Barricadas (RF-052)
    double costoRuta{0.0};          ///< Costo de la ruta A* vigente.
    bool verificarDesvio{false};    ///< La replanificación en curso es por una barricada.
    double costoSinBarricada{0.0};  ///< Costo restante de la ruta antigua, como si no hubiera barricada.
    int barricada{-1};              ///< Índice de la barricada que bloquea o ataca.
    mundo::Celda puestoAtaque{};    ///< Celda desde la que la ataca.
    mundo::Celda objetivoPrevio{};  ///< Objetivo al que vuelve cuando la rompe.
    bool rutaBloqueada{false};
    bool barricadaDestruida{false};
    // Táctica (RF-054) con sus puntuaciones y punto de corte (RF-053)
    agentes::Tactica tactica{agentes::Tactica::Perseguir};
    std::array<float, agentes::kNumTacticas> puntuaciones{};
    bool hayCorte{false};
    mundo::Celda puntoCorte{};
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
    bool lanzarSenuelo{false};     ///< Pulsación (RF-013).
    bool colocarBarricada{false};  ///< Pulsación (RF-014).
    Vec2 apuntar{};                ///< Punto del mundo: destino del señuelo y lado de la barricada.
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
    static constexpr std::size_t kMaxSenuelos = 4;
    static constexpr std::size_t kMaxBarricadas = 8;

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
    [[nodiscard]] std::uint64_t busquedasTotales() const noexcept { return busquedasTotales_; }

    /// Coordinación (RF-047, RF-073).
    void fijarCoordinado(bool activo) noexcept { cfg_.coordinado = activo; }
    [[nodiscard]] std::uint64_t alertasEmitidas() const noexcept { return alertasEmitidas_; }
    [[nodiscard]] std::uint64_t mensajesAlerta() const noexcept { return mensajesAlerta_; }
    /// Última propagación (mensajes emisor -> receptor con salto y confianza) y su tick.
    [[nodiscard]] const coordinacion::ResultadoPropagacion& ultimaPropagacion() const noexcept { return propagacion_; }
    [[nodiscard]] std::uint64_t tickUltimaAlerta() const noexcept { return tickUltimaAlerta_; }
    [[nodiscard]] bool huboAlerta() const noexcept { return alertasEmitidas_ > 0; }

    /// Campo de flujo de la horda (RF-028).
    [[nodiscard]] bool campoActivo() const noexcept { return campoActivo_; }
    [[nodiscard]] const navegacion::CampoFlujo& campo() const noexcept { return campo_; }
    [[nodiscard]] std::uint64_t camposCalculados() const noexcept { return camposCalculados_; }
    [[nodiscard]] std::uint32_t agentesEnCampo() const noexcept { return agentesEnCampo_; }

    /// Herramientas de Mara (RF-013, RF-014).
    [[nodiscard]] const std::array<Senuelo, kMaxSenuelos>& senuelos() const noexcept { return senuelos_; }
    [[nodiscard]] const std::array<Barricada, kMaxBarricadas>& barricadas() const noexcept { return barricadas_; }
    [[nodiscard]] std::uint64_t ruidosSenuelo() const noexcept { return ruidosSenuelo_; }
    [[nodiscard]] std::uint64_t cambiosMapa() const noexcept { return cambiosMapa_; }
    /// Ruidos que se procesaron en el último tick de IA (de Mara y de señuelos).
    [[nodiscard]] const std::vector<EventoRuido>& ruidosUltimoTickIa() const noexcept { return ruidosTick_; }
    /// Coloca una barricada en @p c si es transitable y no hay nadie (sin exigir adyacencia: la
    /// acción de Mara la exige). Publica CambioDeMapa. Devuelve false si no se pudo.
    bool colocarBarricadaEn(mundo::Celda c);
    /// Mensaje de alerta externo para un agente (escenarios guiados y pruebas); entra como cualquier alerta.
    void enviarAlerta(std::size_t agente, mundo::Celda celda, float confianza);
    [[nodiscard]] double tiempo() const noexcept { return static_cast<double>(tick_) * static_cast<double>(dt_); }

    /// Cambios de plan (RF-072), anillo como las transiciones.
    [[nodiscard]] std::size_t numCambiosPlan() const noexcept;
    [[nodiscard]] const RegistroPlan& ultimoCambioPlan(std::size_t haceN) const noexcept;
    [[nodiscard]] std::uint64_t totalCambiosPlan() const noexcept { return totalCambiosPlan_; }

    /// Contract Net (RF-049) y reparto de llegadas (RF-051).
    [[nodiscard]] std::uint64_t subastas() const noexcept { return subastas_; }
    [[nodiscard]] std::uint64_t adjudicaciones() const noexcept { return adjudicaciones_; }
    [[nodiscard]] std::uint64_t mensajesContractNet() const noexcept { return mensajesContractNet_; }
    [[nodiscard]] const std::vector<coordinacion::Tarea>& ultimasTareas() const noexcept { return tareas_; }
    [[nodiscard]] const coordinacion::ResultadoContractNet& ultimoContractNet() const noexcept { return contrato_; }

private:
    void moverMara(const EntradaJugador& e);
    void interactuar(const EntradaJugador& e);
    void tickIa();
    void percibirYDecidir(std::size_t i);
    void elegirObjetivo(std::size_t i);
    void planificarRutas();
    void ejecutarInfectado(std::size_t i);
    void moverHorda();
    void accionesMara(const EntradaJugador& e);
    void actualizarSenuelos();
    [[nodiscard]] mundo::Celda destinoSenuelo(Vec2 apuntar) const noexcept;
    void revisarBarricadas();
    void atacarBarricada(Infectado& z);
    void destruirBarricada(std::size_t b);
    [[nodiscard]] int barricadaEn(mundo::Celda c) const noexcept;
    void evaluarTactica(Infectado& z);
    void registrarPlan(std::size_t i, mundo::Celda anterior, mundo::Celda nuevo, float vAnt, float vNuevo, MotivoPlan m);
    void contratar(std::size_t gestor);
    void repartirLlegadas();
    [[nodiscard]] Vec2 puntoDeLlegada(const Infectado& z) const noexcept;
    [[nodiscard]] mundo::Celda celdaLibreCercana(mundo::Celda c, int radioMax) const noexcept;
    void emitirAlerta(std::size_t i);
    void actualizarCampo();
    void velocidadHacia(Infectado& z, Vec2 meta, float velocidad, float frenado);
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

    // Ruidos (RF-003, RF-013): se acumulan entre ticks de IA y se consumen en el siguiente.
    float desdeUltimoRuido_{0.0f};
    std::vector<EventoRuido> ruidosPendientes_;
    std::vector<EventoRuido> ruidosTick_;
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

    std::uint64_t busquedasTotales_{0};

    // Coordinación por alertas recursivas (RF-047).
    coordinacion::PropagadorAlertas propagador_;
    coordinacion::ResultadoPropagacion propagacion_;
    std::vector<Vec2> posiciones_;
    std::uint64_t alertasEmitidas_{0};
    std::uint64_t mensajesAlerta_{0};
    std::uint64_t tickUltimaAlerta_{0};

    // Campo de flujo compartido (RF-028).
    navegacion::CampoFlujo campo_;
    bool campoActivo_{false};
    std::uint64_t camposCalculados_{0};
    std::uint32_t agentesEnCampo_{0};

    // Contract Net y reparto de llegadas (RF-049, RF-051).
    std::vector<coordinacion::Tarea> tareas_;
    std::vector<coordinacion::Postor> postores_;
    coordinacion::ResultadoContractNet contrato_;
    coordinacion::AsignadorAccesos asignador_;
    std::vector<std::uint32_t> idsLlegada_;
    std::vector<mundo::Celda> celdasLlegada_;
    std::vector<coordinacion::Asignacion> asignaciones_;
    std::uint64_t subastas_{0};
    std::uint64_t adjudicaciones_{0};
    std::uint64_t mensajesContractNet_{0};

    // Steering (RF-029)
    std::vector<navegacion::Movil> moviles_;

    // Consulta espacial para el steering y las alertas (RF-030).
    navegacion::Quadtree quadtree_;
    std::vector<std::uint32_t> vecinos_;

    // Señuelos y barricadas (RF-013, RF-014, RF-025)
    std::array<Senuelo, kMaxSenuelos> senuelos_{};
    std::array<Barricada, kMaxBarricadas> barricadas_{};
    std::uint64_t ruidosSenuelo_{0};
    std::uint64_t cambiosMapa_{0};
    bool campoSucio_{false};  ///< Un CambioDeMapa obliga a recalcular el campo de flujo.

    std::array<RegistroPlan, kCapacidadRegistro> planes_{};
    std::uint64_t totalCambiosPlan_{0};

    std::array<RegistroTransicion, kCapacidadRegistro> registro_{};
    std::uint64_t totalTransiciones_{0};
};

}  // namespace refugio::juego
