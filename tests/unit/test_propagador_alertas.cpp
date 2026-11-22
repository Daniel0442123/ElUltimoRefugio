#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

#include "dominio/coordinacion/PropagadorAlertas.hpp"

using namespace refugio::coordinacion;
using refugio::navegacion::Quadtree;
using refugio::navegacion::Vec2;

namespace {
Quadtree indexar(const std::vector<Vec2>& pos) {
    Quadtree qt({0.0f, 0.0f, 128.0f, 128.0f});
    for (std::size_t i = 0; i < pos.size(); ++i) qt.insertar(static_cast<std::uint32_t>(i), pos[i]);
    return qt;
}
}  // namespace

TEST_CASE("CP-047 Cadena A-B-C-D-E: la alerta llega hasta el salto 3", "[RF-047][HU-09]") {
    const std::vector<Vec2> pos = {{10, 10}, {14, 10}, {18, 10}, {22, 10}, {26, 10}};
    const auto qt = indexar(pos);
    PropagadorAlertas p({5.0f, 3, 0.7f});
    const auto r = p.propagar(pos, qt, 0, 1.0f);

    CHECK(r.saltoPorAgente == std::vector<int>{0, 1, 2, 3, -1});
    REQUIRE(r.mensajes.size() == 3);
    CHECK(r.mensajes[0].confianza > r.mensajes[2].confianza);  // se atenúa por salto
    CHECK(r.profundidadAlcanzada == 3);
}

TEST_CASE("CP-047 Ciclo X-Y-Z: cada agente recibe la alerta una sola vez", "[RF-047][HU-09]") {
    const std::vector<Vec2> pos = {{10, 10}, {12, 10}, {11, 12}};
    const auto qt = indexar(pos);
    PropagadorAlertas p({5.0f, 3, 0.7f});
    const auto r = p.propagar(pos, qt, 0, 1.0f);
    CHECK(r.mensajes.size() == 2);
    CHECK(r.saltoPorAgente == std::vector<int>{0, 1, 1});
}

TEST_CASE("CP-127 Grafo completo de 200 agentes: profundidad acotada y sin duplicados", "[RF-047][RNF-027]") {
    std::vector<Vec2> pos;
    for (int i = 0; i < 200; ++i) pos.push_back({20.0f + static_cast<float>(i % 3), 20.0f + static_cast<float>(i % 2)});
    const auto qt = indexar(pos);
    PropagadorAlertas p({5.0f, 3, 0.7f});
    const auto r = p.propagar(pos, qt, 0, 1.0f);
    CHECK(r.mensajes.size() == 199);
    CHECK(r.profundidadAlcanzada <= 3);
    CHECK(r.bytesPilaEstimados <= 64u * 1024u);
    std::vector<int> recibidos(pos.size(), 0);
    for (const auto& m : r.mensajes) ++recibidos[m.receptor];
    CHECK(std::all_of(recibidos.begin() + 1, recibidos.end(), [](int c) { return c == 1; }));
}

TEST_CASE("Profundidad 0: el origen no avisa a nadie", "[RF-047]") {
    const std::vector<Vec2> pos = {{10, 10}, {11, 10}};
    const auto qt = indexar(pos);
    PropagadorAlertas p({5.0f, 0, 0.7f});
    const auto r = p.propagar(pos, qt, 0, 1.0f);
    CHECK(r.mensajes.empty());
    CHECK(r.llamadas == 1);
}
