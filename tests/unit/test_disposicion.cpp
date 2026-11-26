// Ventana adaptable: la interfaz escala con la pantalla y el mapa cabe entero en su zona.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "adaptadores/vista/Disposicion.hpp"

using Catch::Approx;
using namespace refugio;

namespace {
constexpr float kMundoAncho = 32.0f * 32.0f;  // el prototipo: 32 x 22 celdas de 32 px
constexpr float kMundoAlto = 22.0f * 32.0f;
}  // namespace

TEST_CASE("En 1280x720 la disposicion es la de siempre", "[vista][pantalla]") {
    const auto d = vista::calcularDisposicion(1280, 720, kMundoAncho, kMundoAlto);
    CHECK(d.escala == Approx(1.0f));
    CHECK(d.panelX == Approx(1024.0f));
    CHECK(d.mapa.ancho == Approx(1024.0f));
    CHECK(d.mapa.alto == Approx(704.0f));
    CHECK(d.zoomMapa == Approx(1.0f));
    CHECK(d.mundoPx.x == Approx(0.0f));
    CHECK(d.mundoPx.y == Approx(0.0f));
}

TEST_CASE("En 1920x1080 todo crece 1,5 veces y el mapa llena su zona", "[vista][pantalla]") {
    const auto d = vista::calcularDisposicion(1920, 1080, kMundoAncho, kMundoAlto);
    CHECK(d.escala == Approx(1.5f));
    CHECK(d.ancho == Approx(1280.0f));
    CHECK(d.alto == Approx(720.0f));
    CHECK(d.mapaPx.ancho == Approx(1536.0f));
    CHECK(d.mapaPx.alto == Approx(1056.0f));
    CHECK(d.zoomMapa == Approx(1.5f));
    CHECK(d.mundoPx.ancho == Approx(1536.0f));
}

TEST_CASE("En 4K la interfaz se escala por 3 (pantallas de alta densidad)", "[vista][pantalla]") {
    const auto d = vista::calcularDisposicion(3840, 2160, kMundoAncho, kMundoAlto);
    CHECK(d.escala == Approx(3.0f));
    CHECK(d.zoomMapa == Approx(3.0f));
}

TEST_CASE("Una ventana panoramica da el ancho sobrante al mapa y lo centra", "[vista][pantalla]") {
    const auto d = vista::calcularDisposicion(2560, 1080, kMundoAncho, kMundoAlto);
    CHECK(d.escala == Approx(1.5f));
    CHECK(d.ancho > 1280.0f);
    CHECK(d.panelX == Approx(d.ancho - vista::kAnchoPanel));
    // El panel sigue pegado al borde derecho y el mapa, entero y centrado, no lo pisa.
    CHECK(d.mundoPx.x > 0.0f);
    CHECK(d.mundoPx.x + d.mundoPx.ancho <= d.panelX * d.escala + 0.5f);
    CHECK(d.mundoPx.alto == Approx(d.mapaPx.alto));
}

TEST_CASE("Una ventana 16:10 da el alto sobrante al mapa y al panel", "[vista][pantalla]") {
    const auto d = vista::calcularDisposicion(1920, 1200, kMundoAncho, kMundoAlto);
    CHECK(d.escala == Approx(1.5f));
    CHECK(d.alto == Approx(800.0f));
    CHECK(d.mapa.alto == Approx(800.0f - vista::kAltoFranja));
    CHECK(d.mundoPx.ancho == Approx(d.mapaPx.ancho));
    CHECK(d.mundoPx.y > 0.0f);
}

TEST_CASE("Una ventana diminuta no baja de la escala minima", "[vista][pantalla]") {
    CHECK(vista::escalaInterfaz(320, 200) == Approx(vista::kEscalaMinima));
    CHECK(vista::escalaInterfaz(0, 0) == Approx(vista::kEscalaMinima));
    const auto d = vista::calcularDisposicion(0, 0, kMundoAncho, kMundoAlto);
    CHECK(d.zoomMapa > 0.0f);
}

TEST_CASE("La camara que sigue a Mara no ensena nada fuera del mapa", "[vista][pantalla]") {
    // Vista de 1536 x 1056 px con zoom 3: media vista de 256 x 176 px de mundo.
    auto p = vista::encuadrar({0.0f, 0.0f}, 3.0f, 1536.0f, 1056.0f, kMundoAncho, kMundoAlto);
    CHECK(p.x == Approx(256.0f));
    CHECK(p.y == Approx(176.0f));
    p = vista::encuadrar({1000.0f, 700.0f}, 3.0f, 1536.0f, 1056.0f, kMundoAncho, kMundoAlto);
    CHECK(p.x == Approx(kMundoAncho - 256.0f));
    CHECK(p.y == Approx(kMundoAlto - 176.0f));
    // Si la vista es más grande que el mapa en un eje, el mapa queda centrado en ese eje.
    p = vista::encuadrar({0.0f, 0.0f}, 1.0f, 3000.0f, 500.0f, kMundoAncho, kMundoAlto);
    CHECK(p.x == Approx(kMundoAncho / 2.0f));
    CHECK(p.y == Approx(250.0f));
}
