// Panel de monitoreo (HU-14) y registro de planes y mensajes (HU-15): RF-023, RF-070 a RF-073.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "adaptadores/archivo/RegistroArchivo.hpp"
#include "contador_asignaciones.hpp"
#include "dominio/juego/Partida.hpp"

using namespace refugio;
using juego::Partida;
using juego::TipoEventoIa;
using juego::TipoMensaje;
using mundo::Celda;
using E = agentes::EstadoInfectado;

namespace {
juego::Escenario abierto(int ancho, int alto, navegacion::Vec2 mara, std::vector<navegacion::Vec2> infectados) {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(ancho, alto);
    e.inicioMara = mara;
    e.radio = {ancho - 1, alto - 1};
    e.infectados = std::move(infectados);
    return e;
}

/// Mara al este de un muro con una sola abertura abajo; A la ve, B y C están tras el muro (CP-047).
juego::Escenario escenarioMuro() {
    juego::Escenario e;
    e.mapa = mundo::GrafoRejilla(30, 12);
    for (int y = 0; y < 11; ++y) e.mapa.bloquear(15, y);
    e.inicioMara = {20.5f, 5.5f};
    e.radio = {29, 11};
    e.infectados = {{17.5f, 5.5f}, {13.5f, 5.5f}, {9.5f, 5.5f}};
    return e;
}

juego::ConfigPartida configPrueba() {
    juego::ConfigPartida c;
    c.visionAngulo = 360.0f;
    c.danioPorSegundo = 0.0f;
    return c;
}

juego::EntradaJugador lanzar(float x, float y) {
    juego::EntradaJugador e;
    e.lanzarSenuelo = true;
    e.apuntar = {x, y};
    return e;
}

/// Sumidero en memoria sin asignaciones: cuenta líneas por tipo y guarda la última.
class SumideroPrueba final : public puertos::IRegistro {
public:
    void escribir(std::uint64_t tick, std::string_view tipo, std::string_view linea) override {
        ++lineas;
        ultimoTick = tick;
        if (tipo == "plan") ++planes;
        if (tipo == "mensaje") ++mensajes;
        largoUltima = linea.size();
    }
    std::uint64_t lineas{0}, planes{0}, mensajes{0}, ultimoTick{0};
    std::size_t largoUltima{0};
};

std::vector<std::string> leerLineas(const std::filesystem::path& ruta) {
    std::ifstream f(ruta);
    std::vector<std::string> v;
    for (std::string l; std::getline(f, l);) v.push_back(l);
    return v;
}

std::vector<std::string> partirTabs(const std::string& s) {
    std::vector<std::string> campos;
    std::size_t inicio = 0;
    for (std::size_t k = 0; k <= s.size(); ++k)
        if (k == s.size() || s[k] == '\t') {
            campos.push_back(s.substr(inicio, k - inicio));
            inicio = k + 1;
        }
    return campos;
}
}  // namespace

// ----------------------------------------------------------------------------------- RF-070
TEST_CASE("CP-070 El agente seleccionado expone objetivo, estado, percepcion, ruta y metricas de A*", "[RF-070][RF-023][HU-14]") {
    Partida p(abierto(30, 12, {25.5f, 5.5f}, {{3.5f, 5.5f}, {3.5f, 9.5f}}), configPrueba(), 1);
    CHECK(p.seleccionado() == -1);
    CHECK(p.agenteEn({3.6f, 5.4f}) == 0);
    CHECK(p.agenteEn({3.5f, 9.2f}) == 1);
    CHECK(p.agenteEn({12.0f, 5.0f}) == -1);  // clic en una celda vacía
    p.seleccionar(0);
    CHECK(p.seleccionado() == 0);

    // Le llega una alerta: investiga la celda (20, 5) y planifica con A* en el mismo ciclo de IA.
    p.enviarAlerta(0, {20, 5}, 0.9f);
    p.paso({});
    const auto& z = p.infectados()[0];
    REQUIRE(z.estado == E::Investigar);
    CHECK(z.objetivo == Celda{20, 5});
    CHECK(z.percibido.recibioAlerta);
    CHECK(z.percibido.confianzaAlerta == Catch::Approx(0.9f));
    CHECK(z.percibido.evidencias >= 1);
    CHECK(z.percibido.distanciaMara == Catch::Approx(22.0f));

    // Las métricas del panel son las de una búsqueda A* independiente con la misma consulta.
    const auto& m = z.busqueda;
    REQUIRE(m.valida);
    CHECK(m.algoritmo == navegacion::Algoritmo::AEstrella);
    CHECK(m.origen == Celda{3, 5});
    CHECK(m.destino == Celda{20, 5});
    navegacion::BuscadorMejorPrimero a(navegacion::Algoritmo::AEstrella);
    const auto ref = a.buscar(p.mapa(), p.mapa().nodo(3, 5), p.mapa().nodo(20, 5));
    CHECK(m.encontrada);
    CHECK(m.costo == Catch::Approx(ref.costo));
    CHECK(m.costo == Catch::Approx(17.0));
    CHECK(m.expandidos == ref.expandidos);
    CHECK(m.generados == ref.generados);
    CHECK(m.largoRuta == ref.ruta.size());
    CHECK(m.tiempoUs >= 0.0);
    CHECK(m.memoriaBytes > 0);
    CHECK(z.ruta.size() == m.largoRuta);  // la ruta que sigue es la de esas métricas

    // Fuera de rango o -1 quitan la selección.
    p.seleccionar(7);
    CHECK(p.seleccionado() == -1);
    CHECK(p.expansionSeleccionado().empty());
}

// ----------------------------------------------------------------------------------- RF-071
TEST_CASE("CP-071 Los nodos pintados son exactamente los nodos expandidos del panel", "[RF-071][HU-14]") {
    auto e = abierto(30, 14, {27.5f, 7.5f}, {{2.5f, 7.5f}, {2.5f, 2.5f}});
    for (int y = 2; y < 13; ++y) e.mapa.bloquear(14, y);  // muro: A* tiene que rodear y expande más
    Partida p(std::move(e), configPrueba(), 3);
    p.seleccionar(0);
    CHECK(p.expansionSeleccionado().empty());  // aún no ha planificado nada

    p.enviarAlerta(0, {24, 7}, 0.9f);
    p.enviarAlerta(1, {24, 3}, 0.9f);
    p.paso({});
    const auto& z = p.infectados()[0];
    const auto& exp = p.expansionSeleccionado();
    REQUIRE(z.busqueda.valida);
    CHECK(exp.size() == z.busqueda.expandidos);
    CHECK(exp.size() > 17);  // más que la ruta: rodear el muro obliga a explorar
    // Sin repetidos (cada nodo se cierra una vez), todos transitables, y el último es el destino.
    auto ordenados = exp;
    std::sort(ordenados.begin(), ordenados.end());
    CHECK(std::adjacent_find(ordenados.begin(), ordenados.end()) == ordenados.end());
    for (const auto n : exp) CHECK(p.mapa().transitable(n));
    CHECK(exp.front() == p.mapa().nodo(2, 7));
    CHECK(exp.back() == p.mapa().nodo(24, 7));

    // Cambiar la selección repite al momento la última consulta del otro agente.
    p.seleccionar(1);
    const auto& z1 = p.infectados()[1];
    REQUIRE(z1.busqueda.valida);
    CHECK(p.expansionSeleccionado().size() == z1.busqueda.expandidos);
    CHECK(p.expansionSeleccionado().back() == p.mapa().nodo(24, 3));
}

TEST_CASE("CP-071 Seleccionar un agente no cambia la simulacion", "[RF-071][RNF-020]") {
    auto jugar = [](bool seleccionar) {
        Partida p(juego::crearEscenarioAleatorio(32, 22, 10, 21), {}, 21);
        for (int i = 0; i < 30 * 60; ++i) {
            if (seleccionar && i % 200 == 0) p.seleccionar((i / 200) % 10);
            const float a = static_cast<float>(i) * 0.013f;
            p.paso({std::cos(a), std::sin(a), juego::ModoMovimiento::Correr, false});
        }
        return p;
    };
    const auto a = jugar(false);
    const auto b = jugar(true);
    for (std::size_t i = 0; i < a.infectados().size(); ++i) {
        CHECK(a.infectados()[i].pos.x == b.infectados()[i].pos.x);
        CHECK(a.infectados()[i].pos.y == b.infectados()[i].pos.y);
        CHECK(a.infectados()[i].estado == b.infectados()[i].estado);
    }
    CHECK(a.totalTransiciones() == b.totalTransiciones());
    CHECK(a.totalCambiosPlan() == b.totalCambiosPlan());
    CHECK(a.registroIa().total() == b.registroIa().total());
}

// ----------------------------------------------------------------------------------- RF-072
TEST_CASE("CP-072 Tres cambios de plan aparecen en la consola del panel y en logs/ia_<semilla>.log", "[RF-072][HU-15]") {
    const auto carpeta = std::filesystem::temp_directory_path() / "refugio_cp072" / "logs";
    std::filesystem::remove_all(carpeta.parent_path());
    std::uint64_t escritas = 0;
    std::filesystem::path ruta;
    std::vector<juego::LineaIa> planes;
    Celda senuelo{};
    {
        adaptadores::RegistroArchivo archivo(carpeta, 77);
        REQUIRE(archivo.abierto());
        CHECK(archivo.ruta().filename() == "ia_77.log");
        Partida p(abierto(40, 20, {20.5f, 10.5f}, {{10.5f, 9.5f}, {10.5f, 11.5f}, {9.5f, 10.5f}}), configPrueba(), 77);
        p.fijarRegistro(&archivo);
        CHECK(p.semilla() == 77);
        for (std::size_t k = 0; k < 3; ++k) p.enviarAlerta(k, {1, 1}, 0.3f);
        p.paso({});
        p.paso(lanzar(14.5f, 15.5f));
        senuelo = p.senuelos()[0].celda;
        for (int i = 0; i < 30; ++i) p.paso({});
        REQUIRE(p.totalCambiosPlan() == 3);
        // Consola del panel: las tres líneas de plan con agente, plan anterior y nuevo, y motivo.
        const auto& reg = p.registroIa();
        CHECK(reg.totalDe(TipoEventoIa::Plan) == 3);
        CHECK(reg.totalDe(TipoEventoIa::Transicion) == p.totalTransiciones());
        for (std::size_t i = 0; i < reg.tamano(); ++i)
            if (reg.ultima(i).tipo == TipoEventoIa::Plan) planes.push_back(reg.ultima(i));
        escritas = archivo.lineas();
        ruta = archivo.ruta();
        CHECK(escritas == reg.total());
    }  // al destruirse el archivo se cierra y vuelca
    REQUIRE(planes.size() == 3);
    char nuevo[32];
    std::snprintf(nuevo, sizeof(nuevo), "nuevo=(%d,%d)", senuelo.x, senuelo.y);
    std::vector<int> agentes;
    for (const auto& l : planes) {
        const auto t = std::string(l.vista());
        CHECK(l.tick > 0);
        CHECK(t.find("anterior=(1,1)") != std::string::npos);
        CHECK(t.find(nuevo) != std::string::npos);
        CHECK(t.find("motivo=senuelo") != std::string::npos);
        agentes.push_back(l.agente);
    }
    std::sort(agentes.begin(), agentes.end());
    CHECK(agentes == std::vector<int>{0, 1, 2});

    // Archivo: cabecera de 2 comentarios y una línea por evento «tick\ttipo\tagente\tcampos».
    const auto lineas = leerLineas(ruta);
    REQUIRE(lineas.size() == escritas + 2);
    CHECK(lineas[0].rfind("# El ultimo refugio", 0) == 0);
    CHECK(lineas[0].find("semilla 77") != std::string::npos);
    int planesArchivo = 0;
    for (std::size_t i = 2; i < lineas.size(); ++i) {
        const auto c = partirTabs(lineas[i]);
        REQUIRE(c.size() >= 4);
        CHECK(c[0].find_first_not_of("0123456789") == std::string::npos);  // el tick
        if (c[1] != "plan") continue;
        ++planesArchivo;
        REQUIRE(c.size() == 7);
        CHECK(c[2].front() == 'Z');
        CHECK(c[3] == "anterior=(1,1)");
        CHECK(c[4] == nuevo);
        CHECK(c[5] == "motivo=senuelo");
        CHECK(c[6].rfind("valores=", 0) == 0);
    }
    CHECK(planesArchivo == 3);
    std::filesystem::remove_all(carpeta.parent_path());
}

TEST_CASE("CP-072 Si la carpeta de registros no se puede crear el juego sigue sin archivo", "[RF-072][RF-093]") {
    const auto base = std::filesystem::temp_directory_path() / "refugio_cp072_bloqueo";
    std::filesystem::remove_all(base);
    std::filesystem::create_directories(base);
    std::ofstream(base / "logs") << "esto es un archivo, no una carpeta";
    adaptadores::RegistroArchivo archivo(base / "logs", 5);
    CHECK_FALSE(archivo.abierto());
    archivo.escribir(1, "plan", "Z0");  // no falla
    CHECK(archivo.lineas() == 0);
    std::filesystem::remove_all(base);
}

TEST_CASE("CP-072 El superviviente y las transiciones de la FSM tambien van al registro de IA", "[RF-072][RF-064][HU-15]") {
    juego::Escenario e = abierto(24, 12, {2.5f, 2.5f}, {});
    e.radio = {12, 6};
    e.inicioSuperviviente = {12.5f, 7.5f};
    e.almacen = {18, 6};
    auto cfg = configPrueba();
    cfg.integridadInicial = 30.0f;
    Partida p(std::move(e), cfg, 2);
    p.paso({});
    const auto& reg = p.registroIa();
    REQUIRE(reg.totalDe(TipoEventoIa::Superviviente) == p.totalRegistrosSuperviviente());
    REQUIRE(reg.totalDe(TipoEventoIa::Superviviente) >= 1);
    const juego::LineaIa* sup = nullptr;
    for (std::size_t i = 0; i < reg.tamano() && sup == nullptr; ++i)
        if (reg.ultima(i).tipo == TipoEventoIa::Superviviente) sup = &reg.ultima(i);
    REQUIRE(sup != nullptr);
    CHECK(sup->agente == juego::kAgenteSuperviviente);
    const auto t = std::string(sup->vista());
    CHECK(t.rfind("S\tmeta=", 0) == 0);
    CHECK(t.find("motivo=") != std::string::npos);
    CHECK(t.find("plan=IrAlmacen>RecogerTablas>IrPuerta>RepararPuerta") != std::string::npos);
}

// ----------------------------------------------------------------------------------- RF-073
TEST_CASE("CP-073 Una alerta en la cadena de CP-047 registra emisor, receptor, tipo, salto y confianza", "[RF-073][RF-047][HU-15]") {
    auto cfg = configPrueba();
    cfg.contractNet = false;
    Partida p(escenarioMuro(), cfg, 1);
    for (int i = 0; i < 12; ++i) p.paso({});
    REQUIRE(p.alertasEmitidas() == 1);
    REQUIRE(p.totalMensajes() == 2);  // A -> B (salto 1) y B -> C (salto 2)
    const auto& ab = p.ultimoMensaje(1);
    const auto& bc = p.ultimoMensaje(0);
    CHECK(ab.tipo == TipoMensaje::Alerta);
    CHECK(ab.emisor == 0);
    CHECK(ab.receptor == 1);
    CHECK(ab.salto == 1);
    CHECK(ab.confianza == Catch::Approx(0.7f));
    CHECK(bc.emisor == 1);
    CHECK(bc.receptor == 2);
    CHECK(bc.salto == 2);
    CHECK(bc.confianza == Catch::Approx(0.49f));
    CHECK(ab.tick == p.tickUltimaAlerta());
    CHECK(ab.costo < 0.0f);
    const auto& reg = p.registroIa();
    REQUIRE(reg.totalDe(TipoEventoIa::Mensaje) == 2);
    std::string texto;
    for (std::size_t i = 0; i < reg.tamano(); ++i)
        if (reg.ultima(i).tipo == TipoEventoIa::Mensaje) texto += std::string(reg.ultima(i).vista()) + "\n";
    CHECK(texto.find("Z0>Z1\ttipo=alerta\tsalto=1\tconfianza=0.70") != std::string::npos);
    CHECK(texto.find("Z1>Z2\ttipo=alerta\tsalto=2\tconfianza=0.49") != std::string::npos);
}

TEST_CASE("CP-073 Contract Net registra cada anuncio, puja y adjudicacion", "[RF-073][RF-049][HU-15]") {
    Partida p(escenarioMuro(), configPrueba(), 1);
    for (int i = 0; i < 12; ++i) p.paso({});
    REQUIRE(p.subastas() == 1);
    const auto& cn = p.ultimoContractNet();
    std::uint32_t alertas = 0, anuncios = 0, pujas = 0, adjudicaciones = 0;
    for (std::size_t i = 0; i < p.numMensajes(); ++i) {
        const auto& m = p.ultimoMensaje(i);
        switch (m.tipo) {
            case TipoMensaje::Alerta: ++alertas; break;
            case TipoMensaje::Anuncio:
                ++anuncios;
                CHECK(m.emisor == 0);  // el gestor es quien la vio
                break;
            case TipoMensaje::Puja:
                ++pujas;
                CHECK(m.receptor == 0);
                CHECK(m.costo >= 0.0f);
                break;
            case TipoMensaje::Adjudicacion:
                ++adjudicaciones;
                CHECK(m.emisor == 0);
                CHECK(m.costo >= 0.0f);
                break;
        }
    }
    CHECK(alertas == 2);
    CHECK(anuncios == cn.anuncios);
    CHECK(pujas == cn.pujas);
    CHECK(adjudicaciones == cn.adjudicaciones.size());
    CHECK(p.totalMensajes() == 2 + cn.anuncios + cn.pujas + cn.adjudicaciones.size());
    CHECK(juego::nombreTipoMensaje(TipoMensaje::Adjudicacion) == "adjudicacion");
}

// ------------------------------------------------------------------------------ RegistroIa
TEST_CASE("RegistroIa guarda las ultimas lineas en anillo y recorta las largas", "[RF-072][RNF-006]") {
    juego::RegistroIa r;
    SumideroPrueba s;
    r.fijarSumidero(&s);
    CHECK(r.tamano() == 0);
    for (int i = 0; i < 300; ++i) r.anotar(static_cast<std::uint64_t>(i), TipoEventoIa::Plan, 3, "Z3\tn=%d", i);
    CHECK(r.total() == 300);
    CHECK(r.tamano() == juego::RegistroIa::kCapacidad);
    CHECK(r.ultima(0).vista() == "Z3\tn=299");
    CHECK(r.ultima(0).tick == 299);
    CHECK(r.ultima(juego::RegistroIa::kCapacidad - 1).vista() == "Z3\tn=44");
    CHECK(s.lineas == 300);
    CHECK(s.planes == 300);
    const std::string larga(400, 'x');
    r.anotar(1, TipoEventoIa::Mensaje, 0, "%s", larga.c_str());
    CHECK(r.ultima(0).vista().size() == juego::LineaIa::kLargoLinea - 1);
    CHECK(s.largoUltima == juego::LineaIa::kLargoLinea - 1);
    CHECK(r.totalDe(TipoEventoIa::Mensaje) == 1);
}

TEST_CASE("CP-106 Monitorear un agente y registrar en un sumidero no asigna memoria", "[RNF-006][RF-070][RF-072][RF-073]") {
    Partida p(juego::crearEscenarioAleatorio(32, 22, 10, 3), {}, 3);
    SumideroPrueba s;
    p.fijarRegistro(&s);
    p.seleccionar(2);
    auto entrada = [](int i) {
        const float a = static_cast<float>(i) * 0.02f;
        return juego::EntradaJugador{std::cos(a), std::sin(a),
                                     i % 300 < 150 ? juego::ModoMovimiento::Correr : juego::ModoMovimiento::Caminar,
                                     (i % 50) == 0};
    };
    for (int i = 0; i < 120; ++i) p.paso(entrada(i));
    std::uint64_t asignaciones = 0;
    {
        pruebas::VentanaAsignaciones v;
        for (int i = 120; i < 120 + 60 * 60 && p.estado() == juego::EstadoPartida::EnCurso; ++i) {
            p.paso(entrada(i));
            if (i % 600 == 0) p.seleccionar((i / 600) % 10);
        }
        asignaciones = v.asignaciones();
    }
    CHECK(s.lineas > 0);
    CHECK(s.lineas == p.registroIa().total());
    CHECK(asignaciones == 0);
}
