/// @file main_juego.cpp
/// @brief Prototipo jugable inicial: comparación en vivo de Dijkstra y A* (HU-16, RF-074).
///
/// Controles: clic izquierdo = origen, clic derecho = destino, B = barricada en el cursor,
/// F2 = alternar algoritmo, F3 = comparar ambos, R = nuevo mapa con otra semilla.
/// Con --bench o --estres-recursion se ejecuta sin ventana.

#include <raylib.h>

#include <cstdio>
#include <string>

#include "adaptadores/raylib/VistaDepuracion.hpp"
#include "app/Cli.hpp"
#include "dominio/experimentos/GeneradorMapas.hpp"

using namespace refugio;

namespace {
constexpr int kAnchoVentana = 1280;
constexpr int kAltoVentana = 720;
constexpr int kAnchoPanel = 256;
constexpr int kCelda = 32;

void textoMetricas(const navegacion::ResultadoBusqueda& r, int x, int& y) {
    char buf[160];
    std::snprintf(buf, sizeof(buf), "%s", std::string(navegacion::nombreAlgoritmo(r.algoritmo)).c_str());
    DrawText(buf, x, y, 20, RAYWHITE);
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
}  // namespace

int main(int argc, char** argv) {
    int codigo = 0;
    if (app::ejecutarModoSinVentana(argc, argv, codigo)) return codigo;

    const int columnas = (kAnchoVentana - kAnchoPanel) / kCelda;
    const int filas = kAltoVentana / kCelda;
    std::uint64_t semilla = 42;
    nucleo::Pcg32 rng(semilla, 1u);
    auto grafo = experimentos::generarMapa({columnas, filas, 0.22, 0.10}, rng);
    mundo::Nodo origen = 0;
    mundo::Nodo destino = grafo.numCeldas() - 1;
    grafo.fijarMultiplicador(0, 0, 1);
    grafo.fijarMultiplicador(columnas - 1, filas - 1, 1);

    auto dijkstra = navegacion::crearBuscador(navegacion::Algoritmo::Dijkstra);
    auto aestrella = navegacion::crearBuscador(navegacion::Algoritmo::AEstrella);
    bool usarAEstrella = true;
    bool comparar = false;
    bool recalcular = true;
    navegacion::ResultadoBusqueda resActivo;
    navegacion::ResultadoBusqueda resOtro;

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kAnchoVentana, kAltoVentana, "El ultimo refugio - prototipo de navegacion");
    SetTargetFPS(60);
    const adaptadores::EstiloVista estilo{kCelda, 0, 0};

    while (!WindowShouldClose()) {
        const Vector2 raton = GetMousePosition();
        const int cx = static_cast<int>(raton.x) / kCelda;
        const int cy = static_cast<int>(raton.y) / kCelda;
        const bool enMapa = grafo.dentro(cx, cy) && raton.x < static_cast<float>(columnas * kCelda);

        if (enMapa && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && grafo.transitable(cx, cy)) {
            origen = grafo.nodo(cx, cy);
            recalcular = true;
        }
        if (enMapa && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && grafo.transitable(cx, cy)) {
            destino = grafo.nodo(cx, cy);
            recalcular = true;
        }
        if (enMapa && IsKeyPressed(KEY_B)) {
            const auto n = grafo.nodo(cx, cy);
            if (n != origen && n != destino) {
                grafo.fijarMultiplicador(cx, cy, grafo.transitable(n) ? mundo::kBloqueado : std::uint8_t{1});
                recalcular = true;  // CambioDeMapa -> replanificar (RF-025)
            }
        }
        if (IsKeyPressed(KEY_F2)) {
            usarAEstrella = !usarAEstrella;
            recalcular = true;
        }
        if (IsKeyPressed(KEY_F3)) {
            comparar = !comparar;
            recalcular = true;
        }
        if (IsKeyPressed(KEY_R)) {
            ++semilla;
            rng.sembrar(semilla, 1u);
            grafo = experimentos::generarMapa({columnas, filas, 0.22, 0.10}, rng);
            grafo.fijarMultiplicador(grafo.celda(origen).x, grafo.celda(origen).y, 1);
            grafo.fijarMultiplicador(grafo.celda(destino).x, grafo.celda(destino).y, 1);
            recalcular = true;
        }
        if (recalcular) {
            auto& activo = usarAEstrella ? *aestrella : *dijkstra;
            auto& otro = usarAEstrella ? *dijkstra : *aestrella;
            resActivo = activo.buscar(grafo, origen, destino, true);
            if (comparar) resOtro = otro.buscar(grafo, origen, destino, false);
            recalcular = false;
        }

        BeginDrawing();
        ClearBackground({24, 26, 30, 255});
        adaptadores::dibujarGrafo(grafo, estilo);
        adaptadores::dibujarExpansion(grafo, resActivo, estilo);
        adaptadores::dibujarRuta(grafo, resActivo, estilo);
        const auto co = grafo.celda(origen);
        const auto cd = grafo.celda(destino);
        DrawCircle(co.x * kCelda + kCelda / 2, co.y * kCelda + kCelda / 2, 10, GREEN);
        DrawCircle(cd.x * kCelda + kCelda / 2, cd.y * kCelda + kCelda / 2, 10, MAROON);

        const int px = columnas * kCelda + 12;
        int py = 12;
        DrawText("Panel de monitoreo", px, py, 20, GOLD);
        py += 32;
        textoMetricas(resActivo, px, py);
        if (comparar) {
            DrawText("Comparacion (F3)", px, py, 18, GOLD);
            py += 24;
            textoMetricas(resOtro, px, py);
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "semilla: %llu   FPS: %d", static_cast<unsigned long long>(semilla), GetFPS());
        DrawText(buf, px, kAltoVentana - 130, 14, LIGHTGRAY);
        DrawText("Izq: origen  Der: destino", px, kAltoVentana - 108, 14, LIGHTGRAY);
        DrawText("B: barricada  R: nuevo mapa", px, kAltoVentana - 88, 14, LIGHTGRAY);
        DrawText("F2: algoritmo  F3: comparar", px, kAltoVentana - 68, 14, LIGHTGRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
