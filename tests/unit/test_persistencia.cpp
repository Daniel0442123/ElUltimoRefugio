// Guardar y cargar la partida (HU-19): RF-090, RF-091, RF-093, RNF-007, RNF-016, RNF-017, RNF-021.
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "adaptadores/archivo/ArchivosLocales.hpp"
#include "dominio/nucleo/Pcg32.hpp"
#include "dominio/persistencia/Crc32.hpp"
#include "dominio/persistencia/PartidaGuardada.hpp"

using namespace refugio;
using persistencia::ErrorCarga;
using persistencia::GrabadorPartida;
using persistencia::InicioPartida;

namespace {

InicioPartida inicioPrueba(std::uint64_t semilla = 7) {
    InicioPartida in;
    in.semilla = semilla;
    in.ancho = 32;
    in.alto = 22;
    in.infectados = 10;
    in.flowfieldUmbral = 5;
    return in;
}

/// Jugadora aleatoria pero reproducible: cambia de entrada cada 12 ticks, a veces corre, recoge,
/// lanza señuelos o pone barricadas, y a mitad cambia el algoritmo (F2) y el modo de la horda (F4).
struct Jugadora {
    nucleo::Pcg32 rng{99, 3};
    juego::EntradaJugador e{};
    juego::EntradaJugador siguiente(juego::Partida& p) {
        const auto t = p.tick();
        if (t % 12 == 0) {
            e = {};
            e.ejeX = static_cast<float>(static_cast<int>(rng.siguiente() % 3) - 1);
            e.ejeY = static_cast<float>(static_cast<int>(rng.siguiente() % 3) - 1);
            e.modo = static_cast<juego::ModoMovimiento>(rng.siguiente() % 3);
            e.interactuar = rng.siguiente() % 4 == 0;
        }
        juego::EntradaJugador r = e;
        r.lanzarSenuelo = t % 97 == 50;
        r.colocarBarricada = t % 131 == 70;
        // El ratón se mueve en cada tick: no debe romper los tramos ni la repetición.
        r.apuntar = {p.mara().pos.x + static_cast<float>(t % 7) - 3.0f, p.mara().pos.y + 2.0f};
        if (t == 200) p.alternarAlgoritmo();
        if (t == 400) p.fijarCoordinado(false);
        if (t == 300) p.seleccionar(0);  // el monitor y la comparación no cambian la simulación
        if (t == 310) p.compararSeleccionado();
        return r;
    }
};

struct Jugada {
    std::unique_ptr<juego::Partida> partida;
    GrabadorPartida grabador;
};

Jugada jugar(int ticks, std::uint64_t semilla = 7) {
    Jugada j{persistencia::crearPartida(inicioPrueba(semilla)), GrabadorPartida(inicioPrueba(semilla))};
    Jugadora jugadora;
    for (int i = 0; i < ticks; ++i) j.partida->paso(j.grabador.registrar(jugadora.siguiente(*j.partida), *j.partida));
    return j;
}

std::span<const std::uint8_t> vista(const std::vector<std::uint8_t>& v) { return {v.data(), v.size()}; }

std::filesystem::path carpetaPrueba(const std::string& nombre) {
    const auto d = std::filesystem::path(REFUGIO_DIR_SALIDA) / nombre;
    std::filesystem::remove_all(d);
    std::filesystem::create_directories(d);
    return d;
}

void escribirBytes(const std::filesystem::path& ruta, const std::string& datos) {
    std::ofstream f(ruta, std::ios::binary);
    f << datos;
}

}  // namespace

TEST_CASE("CRC32 de referencia", "[persistencia]") {
    const std::string s = "123456789";
    CHECK(persistencia::crc32({reinterpret_cast<const std::uint8_t*>(s.data()), s.size()}) == 0xCBF43926u);
    CHECK(persistencia::crc32({}) == 0u);
}

TEST_CASE("CP-091 guardar y cargar reproduce el estado exacto", "[persistencia][CP-091]") {
    auto j = jugar(900);
    REQUIRE(j.partida->algoritmo() == navegacion::Algoritmo::Dijkstra);
    REQUIRE_FALSE(j.partida->config().coordinado);
    const auto guardada = persistencia::instantanea(j.grabador, *j.partida);
    CHECK(guardada.ticks == 900);
    CHECK(guardada.versionJuego == REFUGIO_VERSION);

    const auto bytes = persistencia::codificar(guardada);
    auto r = persistencia::cargar(vista(bytes));
    REQUIRE(r.valida());
    CHECK(r.partida->tick() == j.partida->tick());
    CHECK(persistencia::huellaPartida(*r.partida) == persistencia::huellaPartida(*j.partida));
    CHECK(r.partida->mara().pos.x == j.partida->mara().pos.x);
    CHECK(r.partida->mara().pos.y == j.partida->mara().pos.y);
    CHECK(r.partida->algoritmo() == navegacion::Algoritmo::Dijkstra);
    CHECK_FALSE(r.partida->config().coordinado);
    CHECK(r.grabador.ticks() == j.grabador.ticks());

    // Las dos siguen igual con las mismas entradas, y la cargada se puede volver a guardar.
    Jugadora a, b;
    for (int i = 0; i < 300; ++i) {
        const auto ea = a.siguiente(*j.partida);
        j.partida->paso(j.grabador.registrar(ea, *j.partida));
        const auto eb = b.siguiente(*r.partida);
        r.partida->paso(r.grabador.registrar(eb, *r.partida));
    }
    CHECK(persistencia::huellaPartida(*r.partida) == persistencia::huellaPartida(*j.partida));
    const auto otra = persistencia::cargar(vista(persistencia::codificar(persistencia::instantanea(r.grabador, *r.partida))));
    REQUIRE(otra.valida());
    CHECK(persistencia::huellaPartida(*otra.partida) == persistencia::huellaPartida(*j.partida));
}

TEST_CASE("Grabador agrupa ticks iguales e ignora el raton sin accion", "[persistencia]") {
    auto p = persistencia::crearPartida(inicioPrueba());
    GrabadorPartida g(inicioPrueba());
    for (int i = 0; i < 1000; ++i) {
        juego::EntradaJugador e;
        e.ejeX = 1.0f;
        e.apuntar = {static_cast<float>(i), 3.0f};  // el ratón se mueve sin señuelo ni barricada
        const auto usada = g.registrar(e, *p);
        CHECK(usada.apuntar.x == 0.0f);
        p->paso(usada);
    }
    REQUIRE(g.tramos().size() == 1);
    CHECK(g.tramos()[0].repeticiones == 1000);
    juego::EntradaJugador s;
    s.lanzarSenuelo = true;
    s.apuntar = {5.0f, 6.0f};
    CHECK(g.registrar(s, *p).apuntar.x == 5.0f);  // con acción, el punto se conserva
    CHECK(g.tramos().size() == 2);
    p->alternarAlgoritmo();  // F2 parte el tramo aunque la entrada sea igual
    juego::EntradaJugador e;
    e.ejeX = 1.0f;
    g.registrar(e, *p);
    g.registrar(e, *p);
    CHECK(g.tramos().size() == 3);
    CHECK(g.ticks() == 1003);
}

TEST_CASE("CP-107 cabecera REFG y partida de 10 minutos de 64 KB o menos", "[persistencia][CP-107]") {
    const auto j = jugar(60);
    const auto b = persistencia::codificar(persistencia::instantanea(j.grabador, *j.partida));
    REQUIRE(b.size() > persistencia::kTamCabecera);
    CHECK(std::string(b.begin(), b.begin() + 4) == "REFG");
    CHECK((b[4] | (b[5] << 8)) == persistencia::kVersionFormato);
    // 10 min a 60 Hz cambiando la entrada cada 0,25 s (2400 tramos, con señuelos de vez en cuando).
    persistencia::PartidaGuardada g;
    g.versionJuego = REFUGIO_VERSION;
    g.inicio = inicioPrueba();
    for (int i = 0; i < 2400; ++i) {
        persistencia::Tramo t;
        t.tick.entrada.ejeX = static_cast<float>(i % 3 - 1);
        t.tick.entrada.lanzarSenuelo = i % 50 == 0;
        t.tick.entrada.apuntar = {3.5f, 4.5f};
        t.repeticiones = 15;
        g.tramos.push_back(t);
        g.ticks += 15;
    }
    const auto grande = persistencia::codificar(g);
    CHECK(g.ticks == 36000u);
    CHECK(grande.size() <= 64u * 1024u);
    const auto r = persistencia::decodificar(vista(grande));
    REQUIRE(r.valida());
    CHECK(r.datos.tramos.size() == 2400);
}

TEST_CASE("CP-121 todas las mutaciones de 1 byte se detectan", "[persistencia][CP-121]") {
    const auto j = jugar(240);
    const auto original = persistencia::codificar(persistencia::instantanea(j.grabador, *j.partida));
    nucleo::Pcg32 rng(2026, 9);
    int detectadas = 0;
    constexpr int kMutaciones = 1000;
    for (int i = 0; i < kMutaciones; ++i) {
        auto m = original;
        const auto pos = rng.siguiente() % m.size();
        m[pos] = static_cast<std::uint8_t>(m[pos] ^ static_cast<std::uint8_t>(1u + rng.siguiente() % 255u));
        const auto r = persistencia::decodificar(vista(m));
        if (!r.valida()) ++detectadas;
    }
    CHECK(detectadas == kMutaciones);  // objetivo >= 99,9 %; el CRC-32 detecta el 100 %
}

TEST_CASE("CP-116 archivos de partida truncados o vacios no cierran el juego", "[persistencia][CP-116]") {
    const auto j = jugar(120);
    const auto original = persistencia::codificar(persistencia::instantanea(j.grabador, *j.partida));
    for (std::size_t n = 0; n < original.size(); ++n) {
        const std::vector<std::uint8_t> corto(original.begin(), original.begin() + static_cast<std::ptrdiff_t>(n));
        const auto r = persistencia::cargar(vista(corto));
        CHECK_FALSE(r.valida());
        CHECK(persistencia::mensajeError(r.error) == "La partida guardada está dañada");
    }
    auto sobra = original;
    sobra.push_back(0);
    CHECK(persistencia::decodificar(vista(sobra)).error == ErrorCarga::Longitud);
    auto version = original;
    version[4] = 9;
    CHECK(persistencia::decodificar(vista(version)).error == ErrorCarga::Version);
    CHECK(persistencia::mensajeError(ErrorCarga::Version) != "La partida guardada está dañada");
}

TEST_CASE("CP-093 rangos invalidos con CRC correcto se rechazan", "[persistencia][CP-093]") {
    const auto j = jugar(60);
    const auto base = persistencia::instantanea(j.grabador, *j.partida);

    auto mapa = base;
    mapa.inicio.ancho = 3;  // crearEscenarioAleatorio no podría generarlo
    CHECK(persistencia::cargar(vista(persistencia::codificar(mapa))).error == ErrorCarga::Formato);

    auto ia = base;
    ia.inicio.ia.tickIaHz = 7;  // no divide a 60
    CHECK(persistencia::cargar(vista(persistencia::codificar(ia))).error == ErrorCarga::Formato);

    auto suma = base;
    suma.ticks += 1;  // los tramos no suman los ticks
    CHECK(persistencia::cargar(vista(persistencia::codificar(suma))).error == ErrorCarga::Formato);

    auto eje = base;
    eje.tramos[0].tick.entrada.ejeX = 5.0f;
    CHECK(persistencia::cargar(vista(persistencia::codificar(eje))).error == ErrorCarga::Formato);

    // Bien formada pero con otras entradas: al repetirla no se llega a la huella guardada.
    auto huella = base;
    huella.tramos[0].tick.entrada.ejeX = huella.tramos[0].tick.entrada.ejeX == 1.0f ? -1.0f : 1.0f;
    huella.tramos[0].tick.entrada.ejeY = 1.0f;
    CHECK(persistencia::cargar(vista(persistencia::codificar(huella))).error == ErrorCarga::Huella);

    auto otraVersion = huella;
    otraVersion.versionJuego = "0.0.1";
    const auto r = persistencia::cargar(vista(persistencia::codificar(otraVersion)));
    CHECK(r.error == ErrorCarga::OtraVersionJuego);
    CHECK(r.detalle.find("0.0.1") != std::string::npos);
}

TEST_CASE("CP-090 guardar escribe saves/slot1.sav sin dejar temporal", "[persistencia][CP-090]") {
    const auto dir = carpetaPrueba("cp090");
    const auto j = jugar(180);
    REQUIRE(adaptadores::guardarRanura(dir, 1, persistencia::instantanea(j.grabador, *j.partida)).empty());
    CHECK(std::filesystem::exists(dir / "slot1.sav"));
    CHECK_FALSE(std::filesystem::exists(dir / "slot1.sav.tmp"));
    CHECK(std::filesystem::file_size(dir / "slot1.sav") <= 64u * 1024u);  // RNF-007
    // Sobrescribir la misma ranura también es atómico y deja la partida nueva.
    const auto k = jugar(240);
    REQUIRE(adaptadores::guardarRanura(dir, 1, persistencia::instantanea(k.grabador, *k.partida)).empty());
    const auto r = adaptadores::cargarRanura(dir, 1);
    REQUIRE(r.valida());
    CHECK(r.partida->tick() == 240);
    CHECK(persistencia::huellaPartida(*r.partida) == persistencia::huellaPartida(*k.partida));
}

TEST_CASE("CP-117 un guardado interrumpido no estropea la partida anterior", "[persistencia][CP-117]") {
    const auto dir = carpetaPrueba("cp117");
    const auto j = jugar(150);
    REQUIRE(adaptadores::guardarRanura(dir, 2, persistencia::instantanea(j.grabador, *j.partida)).empty());
    const auto k = jugar(300);
    const auto bytes = persistencia::codificar(persistencia::instantanea(k.grabador, *k.partida));
    nucleo::Pcg32 rng(117, 1);
    // El proceso muere tras escribir una parte del temporal y antes del renombrado: queda
    // slot2.sav.tmp a medias. 100 interrupciones en puntos aleatorios.
    for (int i = 0; i < 100; ++i) {
        const auto corte = rng.siguiente() % bytes.size();
        escribirBytes(dir / "slot2.sav.tmp", std::string(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(corte)));
        const auto r = adaptadores::cargarRanura(dir, 2);
        REQUIRE(r.valida());
        CHECK(r.partida->tick() == 150);
    }
    // El siguiente guardado reemplaza el temporal huérfano.
    REQUIRE(adaptadores::guardarRanura(dir, 2, persistencia::instantanea(j.grabador, *j.partida)).empty());
    CHECK_FALSE(std::filesystem::exists(dir / "slot2.sav.tmp"));
}

TEST_CASE("CP-093 ranura con un byte alterado avisa que esta danada", "[persistencia][CP-093]") {
    const auto dir = carpetaPrueba("cp093");
    const auto j = jugar(90);
    REQUIRE(adaptadores::guardarRanura(dir, 1, persistencia::instantanea(j.grabador, *j.partida)).empty());
    {
        std::fstream f(dir / "slot1.sav", std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(40);
        f.put('\x7F');
    }
    const auto r = adaptadores::cargarRanura(dir, 1);
    CHECK_FALSE(r.valida());
    CHECK(r.partida == nullptr);
    CHECK(persistencia::mensajeError(r.error, 1) == "La partida guardada está dañada");

    const auto vacia = adaptadores::cargarRanura(dir, 3);
    CHECK(vacia.error == ErrorCarga::Vacia);
    CHECK(persistencia::mensajeError(vacia.error, 3) == "La ranura 3 está vacía");
}

TEST_CASE("RNF-021 archivos de mas de 10 MB se rechazan sin leerlos", "[persistencia][RNF-021]") {
    const auto dir = carpetaPrueba("rnf021");
    {
        std::ofstream f(dir / "slot1.sav", std::ios::binary);
        f << "REFG";
    }
    std::filesystem::resize_file(dir / "slot1.sav", persistencia::kMaxBytesArchivo + 1);
    const auto r = adaptadores::cargarRanura(dir, 1);
    CHECK(r.error == ErrorCarga::Grande);
    CHECK(persistencia::mensajeError(r.error) == "La partida guardada está dañada");
    const auto l = adaptadores::leerArchivo(dir / "slot1.sav");
    CHECK(l.existe);
    CHECK(l.grande);
    CHECK(l.datos.empty());
}
