/// @file main_juego.cpp
/// @brief Primer prototipo jugable (HU-01, HU-08) y laboratorio de navegación Dijkstra / A* (HU-16).
///
/// Escena «Partida» (por defecto): Mara reúne medicamentos, combustible y batería y activa la
/// radio mientras los infectados exploran, la oyen, la persiguen con A* y la atacan.
///   WASD / flechas = mover, Shift = correr, Ctrl = sigilo, E = recoger / activar la radio,
///   Q = señuelo hacia el cursor, B = barricada en la celda de al lado hacia el cursor,
///   F1 = depuración (conos de visión, rutas, ruido, campo de flujo), C = modo coordinado,
///   clic izquierdo sobre un infectado = monitor del agente (HU-14), N = nodos expandidos,
///   L = consola del registro de IA (HU-15), R = nueva partida, Tab = laboratorio.
///   El registro de IA se escribe también en logs/ia_<semilla>.log.
/// Escena «Laboratorio»: clic izquierdo = origen, clic derecho = destino, B = barricada,
///   F2 = alternar algoritmo, F3 = comparar ambos, R = nuevo mapa, Tab = volver a la partida.
/// Con --bench o --estres-recursion se ejecuta sin ventana.
///
/// Bucle de paso fijo (ADR-0005): la simulación avanza en pasos de 1/60 s acumulando el tiempo
/// real del frame; el dibujo va aparte y no cambia el resultado de la simulación.

#include <raylib.h>

#include <cstdio>
#include <memory>

#include "adaptadores/archivo/RegistroArchivo.hpp"

#include "adaptadores/raylib/VistaDepuracion.hpp"
#include "adaptadores/raylib/VistaPartida.hpp"
#include "app/Cli.hpp"
#include "dominio/experimentos/GeneradorMapas.hpp"
#include "dominio/juego/Partida.hpp"

using namespace refugio;

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

// ------------------------------------------------------------------------------- Partida
class EscenaPartida {
public:
    explicit EscenaPartida(std::uint64_t semilla) { reiniciar(semilla); }

    void reiniciar(std::uint64_t semilla) {
        semilla_ = semilla;
        juego::ConfigPartida cfg;
        cfg.flowfieldUmbral = kUmbralCampoPrototipo;
        cfg.coordinado = coordinado_;
        partida_.reset();  // deja de escribir en el registro anterior antes de cerrarlo
        registro_ = std::make_unique<adaptadores::RegistroArchivo>("logs", semilla);  // RF-072, RF-073
        partida_ = std::make_unique<juego::Partida>(
            juego::crearEscenarioAleatorio(kColumnas, kFilas, kInfectados, semilla), cfg, semilla);
        if (registro_->abierto()) partida_->fijarRegistro(registro_.get());
        acumulado_ = 0.0f;
    }

    void actualizar() {
        if (IsKeyPressed(KEY_R)) reiniciar(semilla_ + 1);
        if (IsKeyPressed(KEY_F1)) depuracion_ = !depuracion_;
        if (IsKeyPressed(KEY_C)) {  // RF-084: comparar horda aislada y coordinada
            coordinado_ = !coordinado_;
            partida_->fijarCoordinado(coordinado_);
        }
        if (IsKeyPressed(KEY_N)) nodos_ = !nodos_;                                  // RF-071
        if (IsKeyPressed(KEY_L)) consola_ = adaptadores::siguienteModo(consola_);  // RF-072, RF-073
        // RF-070: clic izquierdo en el mapa selecciona el infectado bajo el cursor (o suelta).
        const Vector2 r = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && r.x < static_cast<float>(kColumnas * kCelda))
            partida_->seleccionar(
                partida_->agenteEn({r.x / static_cast<float>(kCelda), r.y / static_cast<float>(kCelda)}, 0.7f));
        if (++frames_ % 60 == 0) registro_->vaciar();  // el archivo se puede abrir con el juego en marcha

        juego::EntradaJugador e;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) e.ejeX += 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) e.ejeX -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) e.ejeY += 1.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) e.ejeY -= 1.0f;
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
            e.modo = juego::ModoMovimiento::Correr;
        else if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
            e.modo = juego::ModoMovimiento::Sigilo;
        e.interactuar = IsKeyDown(KEY_E);
        // RF-013 / RF-014: Q lanza un señuelo al cursor y B pone una barricada a su lado. Son
        // pulsaciones: se guardan hasta el siguiente paso de simulación y se usan una sola vez.
        if (IsKeyPressed(KEY_Q)) senueloPendiente_ = true;
        if (IsKeyPressed(KEY_B)) barricadaPendiente_ = true;
        const Vector2 raton = GetMousePosition();
        e.apuntar = {raton.x / static_cast<float>(kCelda), raton.y / static_cast<float>(kCelda)};

        // Paso fijo: se recorta el frame a 0,25 s para no entrar en espiral tras una pausa.
        const float frame = GetFrameTime();
        acumulado_ += frame < 0.25f ? frame : 0.25f;
        while (acumulado_ >= partida_->dt()) {
            e.lanzarSenuelo = senueloPendiente_;
            e.colocarBarricada = barricadaPendiente_;
            senueloPendiente_ = barricadaPendiente_ = false;
            partida_->paso(e);
            acumulado_ -= partida_->dt();
        }
    }

    void dibujar() const {
        const adaptadores::EstiloVista estilo{kCelda, 0, 0};
        adaptadores::dibujarPartida(*partida_, estilo, depuracion_, nodos_);
        adaptadores::dibujarConsolaIa(*partida_, consola_, 0, kFilas * kCelda - 12 * 13 - 8, kColumnas * kCelda, 11);
        adaptadores::dibujarFinPartida(*partida_, kColumnas * kCelda, kFilas * kCelda);
        adaptadores::dibujarPanelPartida(*partida_, kColumnas * kCelda + 12, 12, kAltoVentana, GetFPS());
        char buf[48];
        std::snprintf(buf, sizeof(buf), "semilla %llu", static_cast<unsigned long long>(semilla_));
        DrawText(buf, kColumnas * kCelda + 12, kAltoVentana - 146, 12, GRAY);
    }

private:
    std::unique_ptr<adaptadores::RegistroArchivo> registro_;
    std::unique_ptr<juego::Partida> partida_;
    std::uint64_t semilla_{42};
    std::uint64_t frames_{0};
    bool nodos_{true};
    adaptadores::ModoConsola consola_{adaptadores::ModoConsola::Oculta};
    float acumulado_{0.0f};
    bool depuracion_{false};
    bool coordinado_{true};
    bool senueloPendiente_{false};
    bool barricadaPendiente_{false};
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
    SetExitKey(KEY_ESCAPE);

    EscenaPartida partida(42);
    EscenaLaboratorio laboratorio;
    bool enLaboratorio = false;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_TAB)) enLaboratorio = !enLaboratorio;
        if (enLaboratorio)
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
    CloseWindow();
    return 0;
}
