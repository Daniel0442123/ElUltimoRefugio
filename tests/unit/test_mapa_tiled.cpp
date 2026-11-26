// Mapas de Tiled en JSON (RF-001, IS-01).
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "dominio/juego/MapaTiled.hpp"
#include "dominio/persistencia/PartidaGuardada.hpp"

using namespace refugio;
using juego::leerMapaTiled;
using juego::RolInfectado;
using juego::TipoObjeto;

namespace {
std::string leerTexto(const std::filesystem::path& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string mapaEjemplo() { return leerTexto(std::filesystem::path(REFUGIO_DIR_FUENTES) / "maps" / "puerto.json"); }

/// Mapa de 6 x 4 (celdas de 16 px) con un muro en (3, 0..2), escombros m = 3 en (1, 3) y lo mínimo
/// para jugar. @p objetosExtra se añade a la capa «objetos».
std::string mapaMinimo(const std::string& objetosExtra = "", const std::string& colision = "") {
    const std::string datosColision = colision.empty() ? "0,0,0,9,0,0, 0,0,0,9,0,0, 0,0,0,9,0,0, 0,0,0,0,0,0" : colision;
    return R"({"type": "map", "orientation": "orthogonal", "infinite": false, "width": 6, "height": 4,
        "tilewidth": 16, "tileheight": 16,
        "tilesets": [{"firstgid": 1, "name": "ciudad"}, {"firstgid": 20, "name": "costo"}],
        "layers": [
          {"type": "tilelayer", "name": "suelo", "width": 6, "height": 4,
           "data": [1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1]},
          {"type": "tilelayer", "name": "colision", "width": 6, "height": 4, "data": [)" +
           datosColision + R"(]},
          {"type": "tilelayer", "name": "costo", "width": 6, "height": 4,
           "data": [0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0,22,0,0,0,0]},
          {"type": "group", "name": "entidades", "layers": [
            {"type": "objectgroup", "name": "objetos", "objects": [
              {"id": 1, "type": "medicamento", "x": 8, "y": 8, "point": true},
              {"id": 2, "type": "combustible", "x": 72, "y": 8, "point": true},
              {"id": 3, "class": "bateria", "x": 72, "y": 40, "point": true},
              {"id": 4, "name": "radio", "x": 88, "y": 56, "point": true},
              {"id": 5, "type": "puerta", "x": 48, "y": 32, "width": 16, "height": 16})" +
           objetosExtra + R"(]},
            {"type": "objectgroup", "name": "spawns", "objects": [
              {"id": 6, "type": "mara", "x": 8, "y": 56, "point": true},
              {"id": 7, "type": "infectado", "x": 88, "y": 24, "point": true,
               "properties": [{"name": "rol", "type": "string", "value": "explorador"}]},
              {"id": 8, "type": "perseguidor", "x": 72, "y": 24, "point": true},
              {"id": 9, "type": "superviviente", "x": 88, "y": 40, "point": true}]}]},
          {"type": "objectgroup", "name": "zonas", "objects": [
            {"id": 10, "type": "extraccion", "x": 0, "y": 0, "width": 32, "height": 32}]}]})";
}
}  // namespace

TEST_CASE("CP-001 Cargar un mapa de Tiled construye todas sus capas", "[RF-001][HU-01]") {
    const auto r = leerMapaTiled(mapaMinimo());
    INFO(r.error);
    REQUIRE(r.valido());
    const auto& e = r.escenario;
    // colision: muro en x = 3 de las filas 0 a 2; la puerta (3, 2) se cierra al crear la partida.
    CHECK(e.mapa.ancho() == 6);
    CHECK(e.mapa.alto() == 4);
    for (int y = 0; y < 3; ++y) CHECK_FALSE(e.mapa.transitable(3, y));
    CHECK(e.mapa.transitable(3, 3));
    // costo: gid 22 del conjunto que empieza en 20 es su patrón 2, m = 3.
    CHECK(e.mapa.multiplicador(e.mapa.nodo(1, 3)) == 3);
    CHECK(e.mapa.multiplicador(e.mapa.nodo(0, 0)) == 1);
    // objetos y zonas
    REQUIRE(e.objetos.size() == 3);
    CHECK(e.objetos[0].tipo == TipoObjeto::Medicamento);
    CHECK(e.objetos[0].celda == mundo::Celda{0, 0});
    CHECK(e.objetos[2].tipo == TipoObjeto::Bateria);  // el tipo también se lee de «class»
    CHECK(e.radio == mundo::Celda{5, 3});              // y de «name»
    REQUIRE(e.puertas.size() == 1);
    CHECK(e.puertas[0] == mundo::Celda{3, 2});  // el centro del rectángulo
    CHECK(e.extraccion == mundo::Celda{1, 1});
    // spawns (dentro de un grupo de capas)
    CHECK(e.inicioMara.x == 0.5f);
    CHECK(e.inicioMara.y == 3.5f);
    REQUIRE(e.infectados.size() == 2);
    CHECK(e.roles == std::vector<RolInfectado>{RolInfectado::Explorador, RolInfectado::Perseguidor});
    CHECK(e.inicioSuperviviente.x == 5.5f);

    // Se juega: la partida bloquea la puerta y corre sin errores.
    juego::Partida p(r.escenario);
    CHECK_FALSE(p.mapa().transitable(3, 2));
    for (int i = 0; i < 300; ++i) p.paso({});
    CHECK(p.infectados().size() == 2);
}

TEST_CASE("CP-001 El mapa de ejemplo maps/puerto.json es valido y jugable", "[RF-001]") {
    const auto r = leerMapaTiled(mapaEjemplo(), "maps/puerto.json");
    INFO(r.error);
    REQUIRE(r.valido());
    const auto& e = r.escenario;
    CHECK(e.mapa.ancho() == 32);
    CHECK(e.mapa.alto() == 22);
    CHECK(e.puertas.size() == 8);
    CHECK(e.infectados.size() == 7);
    CHECK(std::count(e.roles.begin(), e.roles.end(), RolInfectado::Explorador) == 3);
    CHECK(std::count_if(e.objetos.begin(), e.objetos.end(),
                        [](const juego::ObjetoMision& o) { return o.tipo == TipoObjeto::Medicamento; }) == 3);
    CHECK(e.almacen.x >= 0);
    CHECK(e.inicioSuperviviente.x >= 0.0f);
    juego::Partida p(r.escenario, {}, 7);
    for (int i = 0; i < 600; ++i) p.paso({0.3f, 0.2f, juego::ModoMovimiento::Caminar, false});
    CHECK(p.tick() > 0);
}

TEST_CASE("CP-001 Un mapa de Tiled invalido devuelve un error con su contexto y nunca lanza", "[RF-001][RNF-016]") {
    const struct {
        std::string texto;
        std::string_view esperado;
    } casos[] = {
        {"{ esto no es json", "JSON"},
        {"[]", "objeto JSON"},
        {R"({"width": 4, "height": 4, "tilewidth": 16, "tileheight": 16, "layers": []})", "«colision»"},
        {R"({"orientation": "isometric", "width": 4, "height": 4})", "orthogonal"},
        {R"({"infinite": true, "width": 4, "height": 4})", "infinitos"},
        {R"({"width": 2, "height": 4, "tilewidth": 16, "tileheight": 16, "layers": []})", "«width»"},
        {mapaMinimo("", "0,0,0"), "deberían ser 24"},
        {mapaMinimo(R"(, {"id": 20, "type": "tesoro", "x": 8, "y": 8})"), "tesoro"},
        {mapaMinimo(R"(, {"id": 21, "type": "medicamento", "x": 56, "y": 8})"), "sobre un muro"},
        {mapaMinimo(R"(, {"id": 22, "type": "radio", "x": 8, "y": 24})"), "más de una radio"},
        {mapaMinimo(R"(, {"id": 23, "type": "bateria", "x": 900, "y": 8})"), "fuera del mapa"},
    };
    for (const auto& c : casos) {
        juego::ResultadoMapaTiled r;
        REQUIRE_NOTHROW(r = leerMapaTiled(c.texto, "prueba.json"));
        INFO(r.error);
        CHECK_FALSE(r.valido());
        CHECK(r.error.find(c.esperado) != std::string::npos);
    }
    // Capa codificada en base64: se pide CSV.
    std::string b64 = mapaMinimo();
    const std::string capa = R"("name": "colision",)";
    b64.replace(b64.find(capa), capa.size(), R"("name": "colision", "encoding": "base64",)");
    const auto r = leerMapaTiled(b64);
    CHECK(r.error.find("CSV") != std::string::npos);
    // Sin Mara.
    std::string sinMara = mapaMinimo();
    sinMara.replace(sinMara.find(R"("type": "mara")"), 14, R"("type": "nadie")");
    CHECK(leerMapaTiled(sinMara).error.find("tipo «nadie»") != std::string::npos);
}

TEST_CASE("CP-001 Una partida en un mapa de Tiled se guarda y se carga con su mapa", "[RF-001][RF-090][RF-091]") {
    persistencia::InicioPartida in;
    in.semilla = 9;
    in.mapaTiled = mapaEjemplo();
    in.ancho = 32;
    in.alto = 22;
    in.infectados = 7;
    auto p = persistencia::crearPartida(in);
    persistencia::GrabadorPartida g(in);
    for (int i = 0; i < 400; ++i) {
        juego::EntradaJugador e;
        e.ejeX = i < 200 ? 1.0f : 0.0f;
        e.ejeY = i < 200 ? 0.0f : 1.0f;
        e.interactuar = true;
        e.usarMedicamento = i == 300;  // RF-015: el bit de usar medicamento viaja en los tramos
        p->paso(g.registrar(e, *p));
    }
    const auto bytes = persistencia::codificar(persistencia::instantanea(g, *p));
    const auto r = persistencia::cargar({bytes.data(), bytes.size()});
    INFO(r.detalle);
    REQUIRE(r.valida());
    CHECK(r.grabador.inicio().mapaTiled == in.mapaTiled);
    CHECK(persistencia::huellaPartida(*r.partida) == persistencia::huellaPartida(*p));
    CHECK(r.partida->puertas().size() == 8);

    // Un mapa estropeado dentro del archivo no se puede reconstruir: error, no excepción.
    auto mal = persistencia::decodificar({bytes.data(), bytes.size()});
    REQUIRE(mal.valida());
    mal.datos.inicio.mapaTiled = "{}";
    const auto rr = persistencia::restaurar(mal.datos);
    CHECK_FALSE(rr.valida());
    CHECK(rr.error == persistencia::ErrorCarga::Formato);
}
