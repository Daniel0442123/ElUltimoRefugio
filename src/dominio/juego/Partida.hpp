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

#include <algorithm>
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
#include "dominio/juego/RegistroIa.hpp"
#include "dominio/agentes/Percepcion.hpp"
#include "dominio/mundo/Grafo.hpp"
#include "dominio/navegacion/BuscadorRuta.hpp"
#include "dominio/navegacion/Comparacion.hpp"
#include "dominio/navegacion/CampoFlujo.hpp"
#include "dominio/navegacion/PlanificadorRutas.hpp"
#include "dominio/navegacion/Quadtree.hpp"
#include "dominio/navegacion/Steering.hpp"
#include "dominio/nucleo/Pcg32.hpp"
#include "dominio/superviviente/ArbolComportamiento.hpp"
#include "dominio/superviviente/Superviviente.hpp"

namespace refugio::juego {

using agentes::EstadoInfectado;
using agentes::MotivoTransicion;
using navegacion::Vec2;

enum class ModoMovimiento : std::uint8_t { Sigilo, Caminar, Correr };
enum class EstadoPartida : std::uint8_t { EnCurso, Victoria, Derrota };
/// Por qué terminó (RF-010, RF-011, RF-012).
enum class MotivoFin : std::uint8_t { Ninguno, Extraccion, Salud, Refugio };
enum class TipoObjeto : std::uint8_t { Medicamento, Combustible, Bateria };
inline constexpr int kNumTiposObjeto = 3;
/// RF-048: rol de cada infectado, según su prototipo de config/ia.json.
enum class RolInfectado : std::uint8_t { Perseguidor, Explorador };
[[nodiscard]] std::string_view nombreRol(RolInfectado r) noexcept;

/// RF-048: prototipo «explorador» (visión amplia, lento). El «perseguidor» son los campos
/// visionRadio, visionAngulo, velPerseguir y danioPorSegundo de ConfigPartida.
struct PrototipoExplorador {
    float visionRadio{7.0f};
    float visionAngulo{110.0f};
    float velocidad{2.0f};  ///< Celdas/s al perseguir.
};

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
    float curaMedicamento{35.0f};      ///< RF-015: CURA_MEDICAMENTO, puntos de salud.
    float ruidoPuertaMetalica{10.0f};  ///< RF-006: RUIDO_PUERTA_METALICA, celdas.
    // Extracción y refugio (RF-009, RF-010, RF-012, RF-063)
    float tExtraccion{60.0f};     ///< T_EXTRACCION: segundos que la zona sigue abierta.
    float radioExtraccion{1.5f};  ///< Celdas alrededor del centro de la zona.
    float ruidoInstalacion{4.0f};  ///< Instalar hace ruido (STB Escena 4: la horda se acerca al refugio).
    float integridadMax{100.0f};  ///< INTEGRIDAD_REFUGIO_MAX
    float integridadInicial{100.0f};
    float danioPuerta{5.0f};      ///< Puntos por segundo de cada infectado que golpea la puerta.
    float alcancePuerta{1.2f};    ///< Distancia al centro de la puerta para golpearla.
    // Infectados (prototipo «perseguidor» de config/ia.json, algo más lento para ser jugable)
    float visionRadio{6.0f};
    float visionAngulo{110.0f};
    float sentidoCercano{1.5f};  ///< A esta distancia y con línea de visión se nota a Mara aunque esté detrás.
    float velExplorar{1.4f};
    float velPerseguir{3.4f};
    float danioPorSegundo{10.0f};
    PrototipoExplorador explorador{};  ///< RF-048
    float periodoReplan{0.3f};   ///< Segundos mínimos entre dos rutas del mismo agente al perseguir.
    float radioExplorar{10.0f};
    int presupuestoNodosTick{4000};  ///< RF-026: nodos expandidos por tick de IA como máximo.
    /// RF-024, RF-074: algoritmo de búsqueda de todos los agentes (F2 lo alterna en la partida).
    navegacion::Algoritmo algoritmo{navegacion::Algoritmo::AEstrella};
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
    // Superviviente de la radio (HU-13, RF-060 a RF-064)
    float velSuperviviente{2.5f};
    float visionSuperviviente{8.0f};  ///< Ve a su alrededor (360°) con línea de visión.
    float distanciaSegura{6.0f};      ///< Huye hasta no ver infectados más cerca que esto.
    float reparacionPorSegundo{8.0f}; ///< RF-063
    float puntosTablas{40.0f};        ///< Integridad que se repara con una carga de tablas.
    float tRecogerTablas{1.0f};
    float tArrancarTablas{15.0f};     ///< Sin almacén: arrancar tablas de muebles es lento.
    float factorAyuda{1.5f};          ///< La instalación avanza un 50 % más rápido si ayuda.
    superviviente::ParametrosUtilidad utilidad{};
};

struct Superviviente {
    Vec2 pos{};
    Vec2 orientacion{1.0f, 0.0f};
    ModoMovimiento modo{ModoMovimiento::Caminar};
    float salud{100.0f};
    std::array<bool, kNumTiposObjeto> inventario{};
    int medicamentos{0};  ///< RF-015: uno queda reservado para la radio; los demás curan.
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

/// RF-073: mensajes entre agentes. Las alertas vienen de la propagación recursiva (RF-047); los
/// anuncios, pujas y adjudicaciones, de una ronda de Contract Net (RF-049).
enum class TipoMensaje : std::uint8_t { Alerta, Anuncio, Puja, Adjudicacion };
[[nodiscard]] std::string_view nombreTipoMensaje(TipoMensaje t) noexcept;

/// RF-073: tick, emisor, receptor, tipo, salto y confianza. @c costo solo en pujas y
/// adjudicaciones (distancia octil de la puja); -1 en los demás.
struct RegistroMensaje {
    std::uint64_t tick{};
    std::uint16_t emisor{};
    std::uint16_t receptor{};
    TipoMensaje tipo{};
    std::uint8_t salto{};
    float confianza{};
    float costo{-1.0f};
};

/// RF-023 / RF-070: métricas de la última búsqueda A* de un agente, para el panel de monitoreo.
struct MetricasBusqueda {
    bool valida{false};  ///< Ya planificó alguna vez.
    navegacion::Algoritmo algoritmo{navegacion::Algoritmo::AEstrella};
    bool encontrada{false};
    double costo{0.0};
    std::uint32_t expandidos{0};
    std::uint32_t generados{0};
    std::uint32_t maxAbierta{0};
    std::uint32_t largoRuta{0};  ///< Celdas de la ruta, origen y destino incluidos.
    double tiempoUs{0.0};
    std::size_t memoriaBytes{0};
    std::uint64_t tick{0};
    mundo::Celda origen{};
    mundo::Celda destino{};
};

/// RF-070: percepción relevante del último ciclo de IA (lo que vio, oyó y le llegó por mensaje).
struct PercepcionMonitor {
    bool veAMara{false};
    float distanciaMara{0.0f};
    std::uint8_t ruidosOidos{0};
    bool recibioAlerta{false};
    float confianzaAlerta{0.0f};
    bool recibioTarea{false};
    std::uint8_t evidencias{0};  ///< Entradas en su memoria (RF-043).
};

struct ObjetoMision {
    TipoObjeto tipo{};
    mundo::Celda celda{};
    bool recogido{false};
};

/// RF-006: puerta metálica. Cerrada bloquea su celda; Mara la abre con interactuar y hace ruido.
/// Los infectados no saben abrirlas. Una vez abierta se queda abierta.
struct PuertaMetalica {
    mundo::Celda celda{};
    bool abierta{false};
};

struct Infectado {
    RolInfectado rol{RolInfectado::Perseguidor};  ///< RF-048
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
    bool rutaPendiente{false};  ///< Necesita una ruta: está (o entra) en la cola del planificador (RF-026).
    bool sinRuta{false};
    mundo::Celda origenBusqueda{};   ///< Origen y destino de la búsqueda en curso o de la última
    mundo::Celda destinoBusqueda{};  ///< (la cola los fija al empezarla, no al pedirla).
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
    bool golpeaPuerta{false};  ///< En este paso golpea la puerta del refugio (RF-063).
    // Táctica (RF-054) con sus puntuaciones y punto de corte (RF-053)
    agentes::Tactica tactica{agentes::Tactica::Perseguir};
    std::array<float, agentes::kNumTacticas> puntuaciones{};
    bool hayCorte{false};
    mundo::Celda puntoCorte{};
    // Panel de monitoreo (RF-070)
    MetricasBusqueda busqueda{};
    PercepcionMonitor percibido{};
};

/// Lo que hace el superviviente en cada paso de simulación; lo fija la hoja del árbol que corre.
enum class Actividad : std::uint8_t { Ninguna, Mover, Recoger, Arrancar, Reparar, Ayudar, Vigilar, Huir };
[[nodiscard]] std::string_view nombreActividad(Actividad a) noexcept;

/// Por qué el superviviente genera un plan nuevo (RF-061, RF-064).
enum class MotivoReplan : std::uint8_t { Inicio, CambioMeta, PlanCompletado, AccionFallida, PrecondicionRota, SinPlan };
[[nodiscard]] std::string_view nombreMotivoReplan(MotivoReplan m) noexcept;

/// Superviviente de la radio (HU-13): Utility elige la meta, GOAP planifica y un árbol ejecuta.
struct SupervivienteRadio {
    bool activo{false};
    Vec2 pos{};
    Vec2 orientacion{1.0f, 0.0f};
    // Decisión (RF-060)
    superviviente::Meta meta{superviviente::Meta::Vigilar};
    std::array<float, superviviente::kNumMetas> utilidades{};
    float distanciaAmenaza{1e9f};  ///< Al infectado visible más cercano.
    Vec2 amenaza{};
    // Plan (RF-061) y acción en curso
    superviviente::PlanGoap plan{};
    std::size_t paso{0};
    bool replanificar{true};
    MotivoReplan motivo{MotivoReplan::Inicio};
    std::uint64_t tickFallo{0};
    superviviente::Accion accionFallida{};
    std::uint64_t planes{0};
    // Ejecución (RF-062, RF-063)
    Actividad actividad{Actividad::Ninguna};
    float progreso{0.0f};  ///< Segundos recogiendo o arrancando tablas.
    float tablas{0.0f};    ///< Integridad que aún puede reparar con lo que lleva.
    bool reparo{false};    ///< Ya reparó algo en la acción en curso.
    bool ayudando{false};  ///< En este paso ayuda a instalar.
    bool almacenInaccesible{false};
    std::uint64_t cambiosMapaAlmacen{0};
    // Ruta A*
    std::vector<mundo::Nodo> ruta;
    std::size_t indiceRuta{0};
    mundo::Celda destino{-1, -1};
    std::uint64_t cambiosMapaRuta{0};
    bool hayRuta{false};
};

/// RF-064: cada plan nuevo del superviviente, con su motivo.
struct RegistroSuperviviente {
    std::uint64_t tick{};
    superviviente::Meta meta{};
    MotivoReplan motivo{};
    std::uint64_t tickFallo{};          ///< Solo para AccionFallida y PrecondicionRota.
    superviviente::Accion accionFallida{};
    std::uint8_t longitud{};
    float costo{};
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
    bool usarMedicamento{false};   ///< Pulsación (RF-015).
    Vec2 apuntar{};                ///< Punto del mundo: destino del señuelo y lado de la barricada.
};

/// Escenario explícito (para pruebas) o generado (crearEscenarioAleatorio).
struct Escenario {
    mundo::GrafoRejilla mapa{1, 1};
    Vec2 inicioMara{};
    std::vector<Vec2> infectados;
    /// RF-048: rol de cada infectado; si falta (vector más corto), Perseguidor.
    std::vector<RolInfectado> roles;
    std::vector<ObjetoMision> objetos;
    /// RF-006: celdas de puerta metálica (cerradas al empezar: la partida las bloquea).
    std::vector<mundo::Celda> puertas;
    mundo::Celda radio{};  ///< La radio está en el refugio; su celda es también la puerta (RF-063).
    mundo::Celda extraccion{-1, -1};  ///< Zona de extracción; sin definir, el punto de partida de Mara.
    Vec2 inicioSuperviviente{-1.0f, -1.0f};  ///< Sin definir (x < 0), no hay superviviente.
    mundo::Celda almacen{-1, -1};            ///< Donde hay tablas; sin definir, solo puede arrancarlas.
};

/// Ciudad de generarMapa con Mara en una esquina, los objetos y la radio lejos de ella y
/// @p numInfectados infectados repartidos a no menos de 12 celdas de Mara. Determinista.
/// Uno de cada tres infectados es explorador (RF-048); hay dos medicamentos de más para curarse
/// (RF-015) y hasta dos puertas metálicas en huecos de muro (RF-006).
[[nodiscard]] Escenario crearEscenarioAleatorio(int ancho, int alto, int numInfectados, std::uint64_t semilla);

class Partida {
public:
    static constexpr std::size_t kCapacidadRegistro = 64;
    static constexpr std::size_t kMaxSenuelos = 4;
    static constexpr std::size_t kMaxBarricadas = 8;
    static constexpr std::size_t kCapacidadRegistroSuperviviente = 16;
    static constexpr std::size_t kCapacidadMensajes = 256;
    static constexpr std::size_t kMaxPuertas = 16;

    Partida(Escenario escenario, const ConfigPartida& cfg = {}, std::uint64_t semilla = 42);

    /// Avanza un paso fijo de simulación. Sin asignaciones de memoria (RNF-006).
    void paso(const EntradaJugador& entrada);

    [[nodiscard]] const mundo::GrafoRejilla& mapa() const noexcept { return mapa_; }
    [[nodiscard]] const Superviviente& mara() const noexcept { return mara_; }
    [[nodiscard]] const std::vector<Infectado>& infectados() const noexcept { return infectados_; }
    [[nodiscard]] const std::vector<ObjetoMision>& objetos() const noexcept { return objetos_; }
    [[nodiscard]] mundo::Celda radio() const noexcept { return radio_; }
    [[nodiscard]] EstadoPartida estado() const noexcept { return estado_; }
    [[nodiscard]] MotivoFin motivoFin() const noexcept { return motivoFin_; }
    /// «Victoria», «Derrota: Mara ha caído» o «Derrota: refugio inutilizado» (o «En curso»).
    [[nodiscard]] std::string_view textoResultado() const noexcept;

    /// Extracción (RF-009): se abre T_EXTRACCION segundos cada vez que se activa la radio.
    [[nodiscard]] mundo::Celda extraccion() const noexcept { return extraccion_; }
    [[nodiscard]] bool zonaAbierta() const noexcept { return restanteExtraccion_ > 0.0f; }
    [[nodiscard]] float restanteExtraccion() const noexcept { return restanteExtraccion_; }
    [[nodiscard]] int activacionesRadio() const noexcept { return activacionesRadio_; }
    /// Refugio (RF-063): integridad de la puerta y cuántos la golpean en este paso.
    [[nodiscard]] float integridad() const noexcept { return integridad_; }
    [[nodiscard]] int golpeandoPuerta() const noexcept { return golpeandoPuerta_; }
    [[nodiscard]] std::uint64_t tick() const noexcept { return tick_; }
    [[nodiscard]] const ConfigPartida& config() const noexcept { return cfg_; }
    [[nodiscard]] float dt() const noexcept { return dt_; }
    [[nodiscard]] bool tieneTodosLosObjetos() const noexcept;

    /// RF-006: puertas metálicas y cuántas se han abierto.
    [[nodiscard]] const std::vector<PuertaMetalica>& puertas() const noexcept { return puertas_; }
    [[nodiscard]] int puertasAbiertas() const noexcept { return puertasAbiertas_; }
    /// RF-015: medicamentos que Mara puede usar ahora (los que tiene menos el de la radio).
    [[nodiscard]] int medicamentosUsables() const noexcept { return std::max(0, mara_.medicamentos - 1); }
    [[nodiscard]] int medicamentosUsados() const noexcept { return medicamentosUsados_; }
    /// RF-048: parámetros del rol del infectado.
    [[nodiscard]] float visionRadio(const Infectado& z) const noexcept {
        return z.rol == RolInfectado::Explorador ? cfg_.explorador.visionRadio : cfg_.visionRadio;
    }
    [[nodiscard]] float visionAngulo(const Infectado& z) const noexcept {
        return z.rol == RolInfectado::Explorador ? cfg_.explorador.visionAngulo : cfg_.visionAngulo;
    }
    [[nodiscard]] float velPerseguir(const Infectado& z) const noexcept {
        return z.rol == RolInfectado::Explorador ? cfg_.explorador.velocidad : cfg_.velPerseguir;
    }

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
    /// HU-23: cola de rutas con presupuesto (RF-026): pendientes, búsqueda a medias y espera.
    [[nodiscard]] const navegacion::PlanificadorRutas& planificador() const noexcept { return planificador_; }
    /// Mayor espera (en ticks de IA, desde la solicitud) de una ruta en toda la partida.
    [[nodiscard]] std::uint32_t esperaMaximaRuta() const noexcept { return esperaMaximaRuta_; }
    /// Mayor número de nodos expandidos en un tick de IA en toda la partida.
    [[nodiscard]] std::uint32_t maxNodosTickIa() const noexcept { return maxNodosTickIa_; }

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

    /// Superviviente de la radio (HU-13).
    [[nodiscard]] const SupervivienteRadio& superviviente() const noexcept { return sup_; }
    [[nodiscard]] mundo::Celda almacen() const noexcept { return almacen_; }
    [[nodiscard]] std::size_t numRegistrosSuperviviente() const noexcept;
    [[nodiscard]] const RegistroSuperviviente& ultimoRegistroSuperviviente(std::size_t haceN) const noexcept;
    [[nodiscard]] std::uint64_t totalRegistrosSuperviviente() const noexcept { return totalRegistrosSup_; }
    [[nodiscard]] const superviviente::ArbolComportamiento& arbolSuperviviente() const noexcept { return arbolSup_; }

    /// Mensajes entre agentes (RF-073), anillo como los demás registros.
    [[nodiscard]] std::size_t numMensajes() const noexcept;
    [[nodiscard]] const RegistroMensaje& ultimoMensaje(std::size_t haceN) const noexcept;
    [[nodiscard]] std::uint64_t totalMensajes() const noexcept { return totalMensajes_; }

    /// Registro de IA en texto (RF-072, RF-073): transiciones, cambios de plan, planes del
    /// superviviente y mensajes, en el orden en que ocurren. Es la consola del panel.
    [[nodiscard]] const RegistroIa& registroIa() const noexcept { return registroIa_; }
    /// Sumidero para el archivo logs/ia_<semilla>.log (no es dueño; nullptr lo desactiva).
    void fijarRegistro(puertos::IRegistro* sumidero) noexcept { registroIa_.fijarSumidero(sumidero); }
    [[nodiscard]] std::uint64_t semilla() const noexcept { return semilla_; }

    /// Panel de monitoreo (HU-14, RF-070, RF-071). Seleccionar un infectado hace que sus búsquedas
    /// guarden el orden de expansión, que la vista pinta sobre el mapa. Al seleccionarlo se repite
    /// su última consulta sobre el mapa actual para que los nodos se vean al momento. No cambia
    /// la simulación: solo escribe métricas y el orden de expansión.
    void seleccionar(int agente);
    [[nodiscard]] int seleccionado() const noexcept { return seleccionado_; }
    /// Índice del infectado más cercano a @p punto a @p radio celdas o menos; -1 si no hay.
    [[nodiscard]] int agenteEn(Vec2 punto, float radio = 0.6f) const noexcept;
    /// Nodos cerrados por la última búsqueda del agente seleccionado, en orden de expansión.
    /// Su tamaño es igual a infectados()[seleccionado()].busqueda.expandidos (CP-071).
    [[nodiscard]] const std::vector<mundo::Nodo>& expansionSeleccionado() const noexcept { return expansionSel_; }

    /// Algoritmo de búsqueda activo para todos los agentes, infectados y superviviente (RF-024,
    /// RF-074). Cambiarlo solo afecta a las búsquedas siguientes: las rutas vigentes se conservan.
    void fijarAlgoritmo(navegacion::Algoritmo a) noexcept { cfg_.algoritmo = a; }
    /// F2: pasa de A* a Dijkstra o al revés y devuelve el nuevo algoritmo activo.
    navegacion::Algoritmo alternarAlgoritmo() noexcept;
    [[nodiscard]] navegacion::Algoritmo algoritmo() const noexcept { return cfg_.algoritmo; }

    /// F3 (HU-16, RF-074): repite la última consulta del agente seleccionado con Dijkstra y con A*
    /// sobre el mapa actual y guarda la tabla en comparacion(). No cambia la simulación: usa sus
    /// propios resultados y no toca rutas, métricas ni el generador aleatorio. Devuelve false si no
    /// hay agente seleccionado o aún no ha planificado.
    bool compararSeleccionado();
    /// Última comparación (valida == false si no hay ninguna) y agente y tick en que se hizo.
    [[nodiscard]] const navegacion::ComparacionBusqueda& comparacion() const noexcept { return comparacion_; }
    [[nodiscard]] int agenteComparado() const noexcept { return agenteComparado_; }
    [[nodiscard]] std::uint64_t tickComparacion() const noexcept { return tickComparacion_; }
    /// Olvida la comparación (el panel deja de mostrar la tabla).
    void descartarComparacion() noexcept {
        comparacion_.valida = false;
        agenteComparado_ = -1;
    }

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
    bool abrirPuertaCercana();
    void terminar(EstadoPartida e, MotivoFin m) noexcept;
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
    void registrarMensaje(std::size_t emisor, std::size_t receptor, TipoMensaje t, int salto, float confianza,
                          float costo = -1.0f);
    void anotarMetricas(std::size_t i, mundo::Celda origen, mundo::Celda destino,
                        const navegacion::ResultadoBusqueda& r) noexcept;
    void entregarRuta(std::size_t i, const navegacion::ResultadoBusqueda& r);
    [[nodiscard]] navegacion::BuscadorMejorPrimero& buscadorActivo() noexcept {
        return cfg_.algoritmo == navegacion::Algoritmo::Dijkstra ? dijkstra_ : buscador_;
    }
    void fijarObjetivo(Infectado& z, mundo::Celda c);
    [[nodiscard]] Vec2 moverConColision(Vec2 pos, Vec2 delta) const noexcept;
    [[nodiscard]] bool libre(Vec2 p) const noexcept;
    [[nodiscard]] float factorTerreno(Vec2 p) const noexcept;
    [[nodiscard]] bool celdaLibreAleatoriaCerca(mundo::Celda centro, float radio, mundo::Celda& salida);
    // Superviviente (PartidaSuperviviente.cpp)
    friend class HojasSuperviviente;
    void construirArbolSuperviviente();
    void decidirSuperviviente();
    void planificarSuperviviente();
    void ejecutarSuperviviente();
    [[nodiscard]] superviviente::Hechos hechosSuperviviente() const noexcept;
    [[nodiscard]] bool supEn(mundo::Celda c) const noexcept;
    [[nodiscard]] superviviente::EstadoNodo avanzarSuperviviente(mundo::Celda destino);
    void fallarPlanSuperviviente(MotivoReplan m, superviviente::Accion a) noexcept;

    ConfigPartida cfg_;
    float dt_;
    int pasosPorTickIa_;
    mundo::GrafoRejilla mapa_;
    Superviviente mara_;
    std::vector<Infectado> infectados_;
    std::vector<ObjetoMision> objetos_;
    mundo::Celda radio_;
    EstadoPartida estado_{EstadoPartida::EnCurso};
    MotivoFin motivoFin_{MotivoFin::Ninguno};
    mundo::Celda extraccion_{};
    float restanteExtraccion_{0.0f};
    int activacionesRadio_{0};
    std::vector<PuertaMetalica> puertas_;
    int puertasAbiertas_{0};
    int medicamentosUsados_{0};
    float desdeRuidoInstalacion_{0.0f};
    float integridad_{100.0f};
    int golpeandoPuerta_{0};
    std::uint64_t tick_{0};
    std::uint64_t semilla_{0};
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
    navegacion::BuscadorMejorPrimero dijkstra_{navegacion::Algoritmo::Dijkstra};  ///< RF-024, RF-074
    navegacion::ResultadoBusqueda resultado_;
    /// HU-23: búsquedas reanudables de los infectados, por prioridad y antigüedad (RF-026).
    navegacion::PlanificadorRutas planificador_;
    std::uint32_t esperaMaximaRuta_{0};
    std::uint32_t maxNodosTickIa_{0};
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

    // Superviviente de la radio (HU-13)
    SupervivienteRadio sup_;
    mundo::Celda almacen_{-1, -1};
    superviviente::PlanificadorGoap goap_;
    superviviente::ArbolComportamiento arbolSup_;
    std::array<superviviente::ArbolComportamiento::Id, superviviente::kNumAcciones> raizAccion_{};
    std::array<RegistroSuperviviente, kCapacidadRegistroSuperviviente> registroSup_{};
    std::uint64_t totalRegistrosSup_{0};

    std::array<RegistroTransicion, kCapacidadRegistro> registro_{};
    std::uint64_t totalTransiciones_{0};

    // Mensajes y registro de IA en texto (RF-072, RF-073)
    std::array<RegistroMensaje, kCapacidadMensajes> mensajes_{};
    std::uint64_t totalMensajes_{0};
    RegistroIa registroIa_;

    // Monitoreo del agente seleccionado (RF-070, RF-071)
    int seleccionado_{-1};
    std::vector<mundo::Nodo> expansionSel_;

    // Comparación en vivo Dijkstra / A* (RF-074): buscadores propios para no tocar los del juego.
    navegacion::BuscadorMejorPrimero compDijkstra_{navegacion::Algoritmo::Dijkstra};
    navegacion::BuscadorMejorPrimero compAEstrella_{navegacion::Algoritmo::AEstrella};
    navegacion::ComparacionBusqueda comparacion_;
    int agenteComparado_{-1};
    std::uint64_t tickComparacion_{0};
};

}  // namespace refugio::juego
