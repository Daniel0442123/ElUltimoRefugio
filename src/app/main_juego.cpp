/// @file main_juego.cpp
/// @brief Prototipo jugable (HU-01, HU-08), laboratorio Dijkstra / A* (HU-16), guardar, cargar y
/// opciones (HU-19) y tutorial de sigilo (HU-20).
///
/// Escena «Partida» (por defecto): Mara reúne medicamentos, combustible y batería y activa la
/// radio mientras los infectados exploran, la oyen, la persiguen con A* y la atacan. Teclas por
/// defecto (todas reasignables en Esc > Opciones > Controles, que las guarda en
/// config/controles.json):
///   WASD (o flechas) = mover, Shift = correr, Ctrl = sigilo, E = recoger / activar la radio,
///   Q = señuelo hacia el cursor, B = barricada en la celda de al lado hacia el cursor,
///   F1 = depuración, F4 = modo aislado / coordinado de la horda (HU-18), clic izquierdo sobre
///   un infectado = monitor del agente (HU-14), N = nodos expandidos, L = consola del registro
///   de IA (HU-15), F2 = alternar Dijkstra / A*, F3 = comparar (HU-16), R = nueva partida,
///   Tab = laboratorio, F5 / F9 = guardado y carga rápidos (ranura 1), T = saltar tutorial,
///   Esc = menú de pausa (guardar y cargar en 3 ranuras, volumen, tutorial, controles, salir).
///   El registro de IA se escribe también en logs/ia_<semilla>.log y las partidas en
///   saves/slotN.sav. Al iniciar se leen config/juego.json, config/controles.json y config/ia.json
///   (RF-092); si alguno es inválido se avisa y se usan los valores por defecto (RF-093).
/// Escena «Laboratorio»: clic izquierdo = origen, clic derecho = destino, B = barricada,
///   F2 = alternar algoritmo, F3 = comparar ambos, R = nuevo mapa, Tab = volver a la partida.
/// Con --bench, --estres-recursion o --coord-exp se ejecuta sin ventana.
///
/// Bucle de paso fijo (ADR-0005): la simulación avanza en pasos de 1/60 s acumulando el tiempo
/// real del frame; el dibujo va aparte y no cambia el resultado de la simulación.

#include <raylib.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#include "adaptadores/archivo/ArchivosLocales.hpp"
#include "adaptadores/archivo/RegistroArchivo.hpp"

#include "adaptadores/raylib/Sonidos.hpp"
#include "adaptadores/raylib/VistaDepuracion.hpp"
#include "adaptadores/raylib/VistaPartida.hpp"
#include "app/Cli.hpp"
#include "app/MenuPausa.hpp"
#include "dominio/experimentos/GeneradorMapas.hpp"
#include "dominio/juego/Partida.hpp"
#include "dominio/juego/Tutorial.hpp"
#include "dominio/navegacion/Comparacion.hpp"
#include "dominio/persistencia/PartidaGuardada.hpp"

using namespace refugio;
using config::Accion;

// Los códigos de tecla del dominio son los de raylib (config::tecla, RNF-023).
static_assert(config::tecla::kEscape == KEY_ESCAPE && config::tecla::kEnter == KEY_ENTER);
static_assert(config::tecla::kArriba == KEY_UP && config::tecla::kAbajo == KEY_DOWN);
static_assert(config::tecla::kIzquierda == KEY_LEFT && config::tecla::kDerecha == KEY_RIGHT);
static_assert(config::tecla::kF1 == KEY_F1 && config::tecla::kF1 + 11 == KEY_F12 && config::tecla::kTab == KEY_TAB);
static_assert(config::tecla::kShiftIzq == KEY_LEFT_SHIFT && config::tecla::kCtrlIzq == KEY_LEFT_CONTROL);
static_assert(config::tecla::kAltDer == KEY_RIGHT_ALT && config::tecla::kRetroceso == KEY_BACKSPACE);
static_assert(config::tecla::kEspacio == KEY_SPACE && 'W' == KEY_W && '0' == KEY_ZERO);

namespace {
constexpr int kAnchoVentana = 1280;
constexpr int kAltoVentana = 720;
constexpr int kAnchoPanel = 256;
constexpr int kCelda = 32;
constexpr int kColumnas = (kAnchoVentana - kAnchoPanel) / kCelda;  // 32
constexpr int kFilas = kAltoVentana / kCelda;                       // 22
constexpr int kInfectados = 10;
// config/ia.json fija flowfield_umbral = 8 para la horda completa; con 10 infectados en el
// prototipo se baja a 5 para que el campo de flujo se vea en una partida normal.
constexpr int kUmbralCampoPrototipo = 5;
constexpr const char* kCarpetaConfig = "config";
constexpr const char* kCarpetaPartidas = "saves";

/// Aviso temporal sobre el mapa (guardado, error de carga, configuración inválida…).
struct Aviso {
    std::string texto;
    bool error{false};
    float restante{0.0f};
};

// ------------------------------------------------------------------------------- Partida
class EscenaPartida {
public:
    EscenaPartida(std::uint64_t semilla, adaptadores::ConfigLocal cfg, const adaptadores::Sonidos& sonidos)
        : config_(std::move(cfg)), menu_(config_.opciones, config_.controles), sonidos_(sonidos) {
        sonidos_.volumen(config_.opciones.volumen);
        reiniciar(semilla);
        if (!config_.errores.empty()) {  // RF-093: se avisa y se sigue con los valores por defecto
            for (const auto& e : config_.errores) std::fprintf(stderr, "config: %s\n", e.c_str());
            avisar(config_.errores.front() + (config_.errores.size() > 1 ? " (y más: ver la consola)" : ""), true, 8.0f);
        }
    }

    [[nodiscard]] bool salir() const noexcept { return salir_; }
    [[nodiscard]] bool menuAbierto() const noexcept { return menu_.abierto(); }
    [[nodiscard]] int tecla(Accion a) const noexcept { return config_.controles.tecla(a); }

    void reiniciar(std::uint64_t semilla) {
        persistencia::InicioPartida inicio;
        inicio.semilla = semilla;
        inicio.ancho = kColumnas;
        inicio.alto = kFilas;
        inicio.infectados = kInfectados;
        inicio.ia = config_.ia;
        inicio.flowfieldUmbral = kUmbralCampoPrototipo;
        inicio.coordinado = coordinado_;
        if (partida_) inicio.algoritmo = partida_->algoritmo();  // F2 se mantiene al reiniciar
        auto nueva = persistencia::crearPartida(inicio);
        // RF-017: el tutorial sale en la primera partida nueva de la sesión si está activado.
        tutorial_ = juego::Tutorial(config_.opciones.tutorial && !tutorialVisto_);
        instalar(std::move(nueva), persistencia::GrabadorPartida(inicio));
    }

    void actualizar() {
        if (aviso_.restante > 0.0f) aviso_.restante -= GetFrameTime();
        if (menu_.abierto()) {
            actualizarMenu();
            acumulado_ = 0.0f;  // en pausa no se acumula tiempo
            return;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            menu_.abrir(descripcionRanuras());
            sonidos_.reproducir(adaptadores::Efecto::Menu);
            return;
        }
        if (pulsada(Accion::GuardadoRapido)) guardar(1);
        if (pulsada(Accion::CargaRapida)) {
            if (progresoSinGuardar())
                menu_.confirmar({app::PeticionMenu::Tipo::Cargar, 1}, descripcionRanuras());  // RNF-014
            else
                cargar(1);
        }
        if (menu_.abierto()) return;
        if (pulsada(Accion::SaltarTutorial)) saltarTutorial();
        if (pulsada(Accion::NuevaPartida)) reiniciar(semilla_ + 1);
        if (pulsada(Accion::Depuracion)) depuracion_ = !depuracion_;
        if (pulsada(Accion::ModoHorda)) {  // RF-056: horda aislada o coordinada (HU-18)
            coordinado_ = !coordinado_;
            partida_->fijarCoordinado(coordinado_);
        }
        if (pulsada(Accion::Algoritmo)) partida_->alternarAlgoritmo();  // RF-074: todos los agentes
        if (pulsada(Accion::Comparar)) {                               // RF-074: tabla Dijkstra / A*
            comparar_ = !comparar_;
            if (!comparar_) partida_->descartarComparacion();
            consultaComparada_ = kSinConsulta;
        }
        if (pulsada(Accion::Nodos)) nodos_ = !nodos_;                                  // RF-071
        if (pulsada(Accion::Consola)) consola_ = adaptadores::siguienteModo(consola_);  // RF-072, RF-073
        // RF-070: clic izquierdo en el mapa selecciona el infectado bajo el cursor (o suelta). El
        // clic sobre «Saltar tutorial» solo salta el tutorial.
        const Vector2 r = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && tutorial_.activo() && CheckCollisionPointRec(r, botonTutorial_))
            saltarTutorial();
        else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && r.x < static_cast<float>(kColumnas * kCelda))
            partida_->seleccionar(
                partida_->agenteEn({r.x / static_cast<float>(kCelda), r.y / static_cast<float>(kCelda)}, 0.7f));
        if (++frames_ % 60 == 0 && registro_) registro_->vaciar();  // el archivo se puede abrir con el juego en marcha

        juego::EntradaJugador e;
        if (mantenida(Accion::Derecha) || IsKeyDown(KEY_RIGHT)) e.ejeX += 1.0f;
        if (mantenida(Accion::Izquierda) || IsKeyDown(KEY_LEFT)) e.ejeX -= 1.0f;
        if (mantenida(Accion::Abajo) || IsKeyDown(KEY_DOWN)) e.ejeY += 1.0f;
        if (mantenida(Accion::Arriba) || IsKeyDown(KEY_UP)) e.ejeY -= 1.0f;
        if (mantenida(Accion::Correr))
            e.modo = juego::ModoMovimiento::Correr;
        else if (mantenida(Accion::Sigilo))
            e.modo = juego::ModoMovimiento::Sigilo;
        e.interactuar = mantenida(Accion::Interactuar);
        // RF-013 / RF-014: señuelo al cursor y barricada a su lado. Son pulsaciones: se guardan
        // hasta el siguiente paso de simulación y se usan una sola vez.
        if (pulsada(Accion::Senuelo)) senueloPendiente_ = true;
        if (pulsada(Accion::Barricada)) barricadaPendiente_ = true;
        const Vector2 raton = GetMousePosition();
        e.apuntar = {raton.x / static_cast<float>(kCelda), raton.y / static_cast<float>(kCelda)};

        // Paso fijo: se recorta el frame a 0,25 s para no entrar en espiral tras una pausa.
        const float frame = GetFrameTime();
        acumulado_ += frame < 0.25f ? frame : 0.25f;
        while (acumulado_ >= partida_->dt()) {
            e.lanzarSenuelo = senueloPendiente_;
            e.colocarBarricada = barricadaPendiente_;
            senueloPendiente_ = barricadaPendiente_ = false;
            pasoSimulacion(e);
            acumulado_ -= partida_->dt();
        }
        // En vivo: la tabla se rehace cuando el agente seleccionado planifica una ruta nueva.
        if (comparar_) {
            const int sel = partida_->seleccionado();
            const auto consulta = sel >= 0 ? partida_->infectados()[static_cast<std::size_t>(sel)].busqueda.tick
                                           : kSinConsulta;
            if (sel != agenteComparado_ || consulta != consultaComparada_) {
                if (sel >= 0) partida_->compararSeleccionado();
                else partida_->descartarComparacion();
                agenteComparado_ = sel;
                consultaComparada_ = consulta;
            }
        }
    }

    void dibujar() {
        const adaptadores::EstiloVista estilo{kCelda, 0, 0};
        adaptadores::dibujarPartida(*partida_, estilo, depuracion_, nodos_);
        adaptadores::dibujarConsolaIa(*partida_, consola_, 0, kFilas * kCelda - 12 * 13 - 8, kColumnas * kCelda, 11);
        adaptadores::dibujarFinPartida(*partida_, kColumnas * kCelda, kFilas * kCelda);
        adaptadores::dibujarPanelPartida(*partida_, kColumnas * kCelda + 12, 12, kAltoVentana, GetFPS(), config_.controles);
        botonTutorial_ = menu_.abierto() ? Rectangle{}
                                         : adaptadores::dibujarTutorial(tutorial_, config_.controles, kColumnas * kCelda);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "semilla %llu  tick %llu", static_cast<unsigned long long>(semilla_),
                      static_cast<unsigned long long>(partida_->tick()));
        DrawText(buf, kColumnas * kCelda + 12, kAltoVentana - 144, 12, GRAY);
        if (aviso_.restante > 0.0f && !menu_.abierto()) dibujarAviso();
        if (menu_.abierto()) dibujarMenu();
    }

private:
    [[nodiscard]] bool pulsada(Accion a) const { return IsKeyPressed(config_.controles.tecla(a)); }
    [[nodiscard]] bool mantenida(Accion a) const { return IsKeyDown(config_.controles.tecla(a)); }

    void instalar(std::unique_ptr<juego::Partida> p, persistencia::GrabadorPartida g) {
        semilla_ = p->semilla();
        agenteComparado_ = -1;
        consultaComparada_ = kSinConsulta;
        comparar_ = false;
        partida_.reset();  // deja de escribir en el registro anterior antes de cerrarlo
        registro_ = std::make_unique<adaptadores::RegistroArchivo>("logs", semilla_);  // RF-072, RF-073
        partida_ = std::move(p);
        grabador_ = std::move(g);
        coordinado_ = partida_->config().coordinado;
        if (registro_->abierto()) partida_->fijarRegistro(registro_.get());
        tickGuardado_ = partida_->tick();
        acumulado_ = 0.0f;
        senueloPendiente_ = barricadaPendiente_ = false;
    }

    void pasoSimulacion(const juego::EntradaJugador& e) {
        const int senuelos = partida_->mara().senuelos;
        const int barricadas = partida_->mara().barricadas;
        int objetos = 0;
        for (const bool b : partida_->mara().inventario) objetos += b ? 1 : 0;
        // RF-090: cada paso queda grabado para poder guardar la partida en cualquier momento.
        partida_->paso(grabador_.registrar(e, *partida_));
        int ahora = 0;
        for (const bool b : partida_->mara().inventario) ahora += b ? 1 : 0;
        if (ahora > objetos) sonidos_.reproducir(adaptadores::Efecto::Recoger);
        if (partida_->mara().senuelos < senuelos) sonidos_.reproducir(adaptadores::Efecto::Senuelo);
        if (partida_->mara().barricadas < barricadas) sonidos_.reproducir(adaptadores::Efecto::Barricada);
        if (tutorial_.activo()) {
            const int antes = tutorial_.completados();
            tutorial_.observar(*partida_);
            if (tutorial_.completados() > antes) sonidos_.reproducir(adaptadores::Efecto::Tutorial);
            if (!tutorial_.activo()) terminarTutorial();
        }
    }

    void saltarTutorial() {
        if (!tutorial_.activo()) return;
        tutorial_.saltar();
        terminarTutorial();
    }

    /// RNF-011: el tiempo del tutorial queda en el registro de IA para medirlo en las pruebas de usabilidad.
    void terminarTutorial() {
        tutorialVisto_ = true;
        if (!registro_ || !registro_->abierto()) return;
        char buf[96];
        std::snprintf(buf, sizeof(buf), "-\t%s\tsegundos=%.1f\tpasos=%d/%d", tutorial_.saltado() ? "saltado" : "completado",
                      tutorial_.segundos(), tutorial_.completados(), juego::kNumPasosTutorial);
        registro_->escribir(partida_->tick(), "tutorial", buf);
    }

    [[nodiscard]] bool progresoSinGuardar() const noexcept {
        return partida_->estado() == juego::EstadoPartida::EnCurso && partida_->tick() != tickGuardado_;
    }

    [[nodiscard]] std::array<std::string, app::MenuPausa::kRanuras> descripcionRanuras() const {
        std::array<std::string, app::MenuPausa::kRanuras> d;
        for (int i = 0; i < app::MenuPausa::kRanuras; ++i) {
            const auto l = adaptadores::leerArchivo(adaptadores::rutaRanura(kCarpetaPartidas, i + 1));
            if (!l.existe) {
                d[static_cast<std::size_t>(i)] = "vacía";
                continue;
            }
            const auto r = persistencia::decodificar(std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(l.datos.data()), l.datos.size()));
            char buf[96];
            if (r.valida())
                std::snprintf(buf, sizeof(buf), "semilla %llu, %.0f s de juego",
                              static_cast<unsigned long long>(r.datos.inicio.semilla),
                              static_cast<double>(r.datos.ticks) / static_cast<double>(r.datos.inicio.ia.tickSimHz));
            else
                std::snprintf(buf, sizeof(buf), "dañada");
            d[static_cast<std::size_t>(i)] = buf;
        }
        return d;
    }

    void guardar(int ranura) {
        const std::string e = adaptadores::guardarRanura(kCarpetaPartidas, ranura, persistencia::instantanea(grabador_, *partida_));
        std::string texto;
        if (e.empty()) {
            tickGuardado_ = partida_->tick();
            texto = "Partida guardada en " + adaptadores::rutaRanura(kCarpetaPartidas, ranura).generic_string();
            sonidos_.reproducir(adaptadores::Efecto::Guardado);
        } else {
            texto = "No se pudo guardar: " + e;
            sonidos_.reproducir(adaptadores::Efecto::Error);
        }
        std::fprintf(stderr, "%s\n", texto.c_str());
        if (menu_.abierto()) {
            menu_.fijarMensaje(texto, !e.empty());
            menu_.actualizarRanuras(descripcionRanuras());
        } else {
            avisar(texto, !e.empty());
        }
    }

    /// RF-091 / RF-093: si la ranura está vacía o dañada, se avisa y la partida actual sigue.
    bool cargar(int ranura) {
        auto r = adaptadores::cargarRanura(kCarpetaPartidas, ranura);
        if (!r.valida()) {
            const std::string texto = persistencia::mensajeError(r.error, ranura);
            std::fprintf(stderr, "%s%s%s\n", texto.c_str(), r.detalle.empty() ? "" : ": ", r.detalle.c_str());
            sonidos_.reproducir(adaptadores::Efecto::Error);
            if (menu_.abierto())
                menu_.fijarMensaje(texto, true);
            else
                avisar(texto, true);
            return false;
        }
        tutorial_ = juego::Tutorial(false);  // al continuar una partida no hay tutorial
        instalar(std::move(r.partida), std::move(r.grabador));
        const std::string texto = "Ranura " + std::to_string(ranura) + " cargada";
        std::fprintf(stderr, "%s\n", texto.c_str());
        avisar(texto, false);
        sonidos_.reproducir(adaptadores::Efecto::Guardado);
        return true;
    }

    void actualizarMenu() {
        for (int k = GetKeyPressed(); k != 0; k = GetKeyPressed()) {
            const auto p = menu_.pulsar(k, progresoSinGuardar());
            sonidos_.reproducir(adaptadores::Efecto::Menu);
            switch (p.tipo) {
                case app::PeticionMenu::Tipo::Ninguna:
                case app::PeticionMenu::Tipo::Continuar: break;
                case app::PeticionMenu::Tipo::Guardar: guardar(p.ranura); break;
                case app::PeticionMenu::Tipo::Cargar:
                    if (cargar(p.ranura)) menu_.cerrar();
                    break;
                case app::PeticionMenu::Tipo::Salir: salir_ = true; break;
                case app::PeticionMenu::Tipo::OpcionesCambiadas: {
                    config_.opciones = menu_.opciones();
                    config_.controles = menu_.controles();
                    sonidos_.volumen(config_.opciones.volumen);
                    // RF-092: los cambios del menú se guardan al momento en config/*.json.
                    const std::string e = adaptadores::guardarConfigLocal(kCarpetaConfig, config_.opciones, config_.controles);
                    if (!e.empty()) menu_.fijarMensaje("No se pudo guardar la configuración: " + e, true);
                    break;
                }
            }
            if (!menu_.abierto() || salir_) break;
        }
    }

    void avisar(std::string texto, bool error, float segundos = 3.5f) {
        aviso_ = {std::move(texto), error, segundos};
    }

    void dibujarAviso() const {
        const int ancho = MeasureText(aviso_.texto.c_str(), 18) + 24;
        const int x = (kColumnas * kCelda - ancho) / 2;
        const int y = kFilas * kCelda - 60;
        DrawRectangle(x, y, ancho, 32, Fade({16, 18, 24, 255}, 0.9f));
        DrawRectangleLines(x, y, ancho, 32, aviso_.error ? RED : Color{120, 220, 130, 255});
        DrawText(aviso_.texto.c_str(), x + 12, y + 7, 18, aviso_.error ? Color{255, 120, 110, 255} : RAYWHITE);
    }

    void dibujarMenu() const {
        const int anchoMapa = kColumnas * kCelda;
        DrawRectangle(0, 0, anchoMapa, kFilas * kCelda, Fade(BLACK, 0.65f));
        const auto lineas = menu_.lineas();
        const bool controles = menu_.pantalla() == app::PantallaMenu::Controles ||
                               menu_.pantalla() == app::PantallaMenu::EsperandoTecla;
        const int alto = controles ? 15 : 30;
        const int fuente = controles ? 15 : 22;
        const int ancho = 600;
        const int x = (anchoMapa - ancho) / 2;
        const int y0 = controles ? 56 : 170;
        const std::string titulo = menu_.titulo();
        DrawText(titulo.c_str(), x, controles ? 16 : y0 - 40, controles ? 22 : 30, GOLD);
        for (std::size_t i = 0; i < lineas.size(); ++i) {
            const int y = y0 + static_cast<int>(i) * (alto + 6);
            const bool sel = static_cast<int>(i) == menu_.cursor();
            if (sel) DrawRectangle(x - 10, y - 3, ancho + 20, alto + 6, {70, 62, 30, 255});
            const Color c = sel ? (menu_.pantalla() == app::PantallaMenu::EsperandoTecla ? ORANGE : RAYWHITE) : LIGHTGRAY;
            const auto tab = lineas[i].find('\t');
            if (tab == std::string::npos) {
                DrawText(lineas[i].c_str(), x, y, fuente, c);
            } else {
                DrawText(lineas[i].substr(0, tab).c_str(), x, y, fuente, c);
                const std::string t = menu_.pantalla() == app::PantallaMenu::EsperandoTecla && sel ? "..." : lineas[i].substr(tab + 1);
                DrawText(t.c_str(), x + ancho - MeasureText(t.c_str(), fuente), y, fuente, sel ? GOLD : c);
            }
        }
        const int yPie = kFilas * kCelda - 70;
        if (!menu_.mensaje().empty()) {
            const int w = MeasureText(menu_.mensaje().c_str(), 18);
            DrawText(menu_.mensaje().c_str(), (anchoMapa - w) / 2, yPie, 18,
                     menu_.mensajeEsError() ? Color{255, 120, 110, 255} : Color{120, 220, 130, 255});
        }
        const char* ayuda = menu_.pantalla() == app::PantallaMenu::Opciones ? "Flechas: elegir y cambiar   Enter: aceptar   Esc: volver"
                                                                          : "Flechas: elegir   Enter: aceptar   Esc: volver";
        DrawText(ayuda, (anchoMapa - MeasureText(ayuda, 14)) / 2, yPie + 30, 14, GRAY);
    }

    adaptadores::ConfigLocal config_;
    app::MenuPausa menu_;
    const adaptadores::Sonidos& sonidos_;
    std::unique_ptr<adaptadores::RegistroArchivo> registro_;
    std::unique_ptr<juego::Partida> partida_;
    persistencia::GrabadorPartida grabador_;
    juego::Tutorial tutorial_{false};
    bool tutorialVisto_{false};
    Rectangle botonTutorial_{};
    Aviso aviso_;
    std::uint64_t tickGuardado_{0};
    bool salir_{false};
    std::uint64_t semilla_{42};
    std::uint64_t frames_{0};
    bool nodos_{true};
    adaptadores::ModoConsola consola_{adaptadores::ModoConsola::Oculta};
    float acumulado_{0.0f};
    bool depuracion_{false};
    bool coordinado_{true};
    bool senueloPendiente_{false};
    bool barricadaPendiente_{false};
    // HU-16: comparación en vivo con F3.
    static constexpr std::uint64_t kSinConsulta = ~std::uint64_t{0};
    bool comparar_{false};
    int agenteComparado_{-1};
    std::uint64_t consultaComparada_{kSinConsulta};
};

// --------------------------------------------------------------------------- Laboratorio
void textoMetricas(const navegacion::ResultadoBusqueda& r, int x, int& y) {
    char buf[160];
    const auto nombre = navegacion::nombreAlgoritmo(r.algoritmo);
    std::snprintf(buf, sizeof(buf), "%.*s", static_cast<int>(nombre.size()), nombre.data());
    DrawText(buf, x, y, 20, r.algoritmo == navegacion::Algoritmo::Dijkstra ? Color{0, 200, 220, 255}
                                                                          : Color{220, 60, 220, 255});
    y += 24;
    std::snprintf(buf, sizeof(buf), "costo: %.3f", r.costo);
    DrawText(r.encontrada ? buf : "sin ruta", x, y, 16, RAYWHITE);
    y += 20;
    std::snprintf(buf, sizeof(buf), "expandidos: %u", r.expandidos);
    DrawText(buf, x, y, 16, RAYWHITE);
    y += 20;
    std::snprintf(buf, sizeof(buf), "generados: %u", r.generados);
    DrawText(buf, x, y, 16, RAYWHITE);
    y += 20;
    std::snprintf(buf, sizeof(buf), "tiempo: %.1f us", r.tiempoUs);
    DrawText(buf, x, y, 16, RAYWHITE);
    y += 20;
    std::snprintf(buf, sizeof(buf), "memoria: %.1f KB", static_cast<double>(r.memoriaBytes) / 1024.0);
    DrawText(buf, x, y, 16, RAYWHITE);
    y += 30;
}

class EscenaLaboratorio {
public:
    EscenaLaboratorio() : grafo_(generar()) {
        destino_ = grafo_.numCeldas() - 1;
        grafo_.fijarMultiplicador(0, 0, 1);
        grafo_.fijarMultiplicador(kColumnas - 1, kFilas - 1, 1);
    }

    void actualizar() {
        const Vector2 raton = GetMousePosition();
        const int cx = static_cast<int>(raton.x) / kCelda;
        const int cy = static_cast<int>(raton.y) / kCelda;
        const bool enMapa = grafo_.dentro(cx, cy) && raton.x < static_cast<float>(kColumnas * kCelda);

        if (enMapa && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && grafo_.transitable(cx, cy)) {
            origen_ = grafo_.nodo(cx, cy);
            recalcular_ = true;
        }
        if (enMapa && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && grafo_.transitable(cx, cy)) {
            destino_ = grafo_.nodo(cx, cy);
            recalcular_ = true;
        }
        if (enMapa && IsKeyPressed(KEY_B)) {
            const auto n = grafo_.nodo(cx, cy);
            if (n != origen_ && n != destino_) {
                grafo_.fijarMultiplicador(cx, cy, grafo_.transitable(n) ? mundo::kBloqueado : std::uint8_t{1});
                recalcular_ = true;  // CambioDeMapa -> replanificar (RF-025)
            }
        }
        if (IsKeyPressed(KEY_F2)) {
            usarAEstrella_ = !usarAEstrella_;
            recalcular_ = true;
        }
        if (IsKeyPressed(KEY_F3)) {
            comparar_ = !comparar_;
            recalcular_ = true;
        }
        if (IsKeyPressed(KEY_R)) {
            ++semilla_;
            grafo_ = generar();
            grafo_.fijarMultiplicador(grafo_.celda(origen_).x, grafo_.celda(origen_).y, 1);
            grafo_.fijarMultiplicador(grafo_.celda(destino_).x, grafo_.celda(destino_).y, 1);
            recalcular_ = true;
        }
        if (recalcular_) {
            auto& activo = usarAEstrella_ ? aestrella_ : dijkstra_;
            auto& otro = usarAEstrella_ ? dijkstra_ : aestrella_;
            activo.buscarEn(grafo_, origen_, destino_, resActivo_, true);
            if (comparar_) otro.buscarEn(grafo_, origen_, destino_, resOtro_, false);
            recalcular_ = false;
        }
    }

    void dibujar() const {
        const adaptadores::EstiloVista estilo{kCelda, 0, 0};
        adaptadores::dibujarGrafo(grafo_, estilo);
        adaptadores::dibujarExpansion(grafo_, resActivo_, estilo);
        adaptadores::dibujarRuta(grafo_, resActivo_, estilo);
        const auto co = grafo_.celda(origen_);
        const auto cd = grafo_.celda(destino_);
        DrawCircle(co.x * kCelda + kCelda / 2, co.y * kCelda + kCelda / 2, 10, GREEN);
        DrawCircle(cd.x * kCelda + kCelda / 2, cd.y * kCelda + kCelda / 2, 10, MAROON);

        const int px = kColumnas * kCelda + 12;
        int py = 12;
        DrawText("Laboratorio de rutas", px, py, 20, GOLD);
        py += 32;
        textoMetricas(resActivo_, px, py);
        if (comparar_) {
            DrawText("Comparacion (F3)", px, py, 18, GOLD);
            py += 24;
            textoMetricas(resOtro_, px, py);
            // RF-082 y HU-16: mismo costo con tolerancia 1e-6 y nodos que ahorra la heurística.
            const auto& d = usarAEstrella_ ? resOtro_ : resActivo_;
            const auto& a = usarAEstrella_ ? resActivo_ : resOtro_;
            const double dif = d.encontrada && a.encontrada ? std::fabs(d.costo - a.costo) : 0.0;
            const bool mismo = d.encontrada == a.encontrada && dif <= navegacion::kToleranciaCosto;
            char buf[96];
            std::snprintf(buf, sizeof(buf), "mismo costo: %s (dif %.1e)", mismo ? "si" : "NO", dif);
            DrawText(buf, px, py, 16, mismo ? Color{120, 220, 130, 255} : RED);
            py += 20;
            const double ahorro = d.expandidos > 0 && a.expandidos < d.expandidos
                                      ? 100.0 * (1.0 - static_cast<double>(a.expandidos) / static_cast<double>(d.expandidos))
                                      : 0.0;
            std::snprintf(buf, sizeof(buf), "A* expande %.0f%% menos", ahorro);
            DrawText(buf, px, py, 16, a.expandidos <= d.expandidos ? Color{120, 220, 130, 255} : RED);
            py += 20;
        }
        // Leyenda de colores del mapa y de la expansión.
        int ly = kAltoVentana - 210;
        const struct {
            Color c;
            const char* t;
        } leyenda[] = {{{200, 200, 190, 255}, "calle (costo 1)"},
                       {{130, 100, 70, 255}, "escombros (costo 2-5)"},
                       {{40, 44, 52, 255}, "pared / barricada"},
                       {{0, 200, 220, 255}, "expandido por Dijkstra"},
                       {{220, 60, 220, 255}, "expandido por A*"}};
        for (const auto& l : leyenda) {
            DrawRectangle(px, ly, 12, 12, l.c);
            DrawRectangleLines(px, ly, 12, 12, GRAY);
            DrawText(l.t, px + 18, ly, 13, LIGHTGRAY);
            ly += 16;
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "semilla: %llu   FPS: %d", static_cast<unsigned long long>(semilla_), GetFPS());
        DrawText(buf, px, kAltoVentana - 118, 14, LIGHTGRAY);
        DrawText("Izq: origen  Der: destino", px, kAltoVentana - 98, 14, LIGHTGRAY);
        DrawText("B: barricada  R: nuevo mapa", px, kAltoVentana - 78, 14, LIGHTGRAY);
        DrawText("F2: algoritmo  F3: comparar", px, kAltoVentana - 58, 14, LIGHTGRAY);
        DrawText("Tab: volver a la partida", px, kAltoVentana - 38, 14, LIGHTGRAY);
    }

private:
    mundo::GrafoRejilla generar() {
        nucleo::Pcg32 rng(semilla_, 1u);
        return experimentos::generarMapa({kColumnas, kFilas, 0.22, 0.10}, rng);
    }

    std::uint64_t semilla_{42};
    mundo::GrafoRejilla grafo_;
    mundo::Nodo origen_{0};
    mundo::Nodo destino_{0};
    navegacion::BuscadorMejorPrimero dijkstra_{navegacion::Algoritmo::Dijkstra};
    navegacion::BuscadorMejorPrimero aestrella_{navegacion::Algoritmo::AEstrella};
    navegacion::ResultadoBusqueda resActivo_;
    navegacion::ResultadoBusqueda resOtro_;
    bool usarAEstrella_{true};
    bool comparar_{false};
    bool recalcular_{true};
};
}  // namespace

int main(int argc, char** argv) {
    int codigo = 0;
    if (app::ejecutarModoSinVentana(argc, argv, codigo)) return codigo;

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kAnchoVentana, kAltoVentana, "El ultimo refugio - prototipo");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);  // Esc abre el menú de pausa; se sale desde el menú o cerrando la ventana
    InitAudioDevice();     // sin dispositivo de audio el juego sigue en silencio

    {
        const adaptadores::Sonidos sonidos;
        EscenaPartida partida(42, adaptadores::leerConfigLocal(kCarpetaConfig), sonidos);  // RF-092
        EscenaLaboratorio laboratorio;
        bool enLaboratorio = false;

        while (!WindowShouldClose() && !partida.salir()) {
            if (!partida.menuAbierto() && IsKeyPressed(partida.tecla(Accion::Laboratorio))) enLaboratorio = !enLaboratorio;
            else if (enLaboratorio)
                laboratorio.actualizar();
            else
                partida.actualizar();

            BeginDrawing();
            ClearBackground({24, 26, 30, 255});
            if (enLaboratorio)
                laboratorio.dibujar();
            else
                partida.dibujar();
            EndDrawing();
        }
    }
    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseWindow();
    return 0;
}
