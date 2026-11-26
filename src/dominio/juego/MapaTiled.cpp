#include "dominio/juego/MapaTiled.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

#include "dominio/agentes/Percepcion.hpp"
#include "dominio/nucleo/Json.hpp"

namespace refugio::juego {

using mundo::Celda;
using nucleo::ValorJson;

namespace {

/// Tiled guarda en los 3-4 bits altos del gid si el patrón está volteado o girado.
constexpr std::uint32_t kMascaraGid = 0x0FFFFFFFu;

/// Error de lectura con el contexto ya escrito; se convierte en ResultadoMapaTiled::error.
struct ErrorMapa {
    std::string mensaje;
};

std::string minusculas(std::string_view s) {
    std::string r(s);
    std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

const ValorJson& campo(const ValorJson& obj, std::string_view clave, std::string_view donde) {
    const ValorJson* v = obj.buscar(clave);
    if (v == nullptr) throw ErrorMapa{std::string(donde) + ": falta «" + std::string(clave) + "»"};
    return *v;
}

int entero(const ValorJson& obj, std::string_view clave, std::string_view donde, long long min, long long max) {
    const std::string ctx = std::string(donde) + ".«" + std::string(clave) + "»";
    return static_cast<int>(campo(obj, clave, donde).comoEntero(ctx, min, max));
}

double numero(const ValorJson& obj, std::string_view clave, std::string_view donde) {
    const ValorJson* v = obj.buscar(clave);
    if (v == nullptr) return 0.0;  // Tiled omite width/height en los objetos puntuales
    const double d = v->comoNumero(std::string(donde) + ".«" + std::string(clave) + "»");
    if (!std::isfinite(d)) throw ErrorMapa{std::string(donde) + ".«" + std::string(clave) + "» no es finito"};
    return d;
}

std::string cadena(const ValorJson& obj, std::string_view clave) {
    const ValorJson* v = obj.buscar(clave);
    return v != nullptr && v->esCadena() ? v->comoCadena() : std::string{};
}

/// Propiedad personalizada de Tiled: «properties»: [{"name": …, "type": …, "value": …}].
std::string propiedad(const ValorJson& obj, std::string_view nombre) {
    const ValorJson* ps = obj.buscar("properties");
    if (ps == nullptr || !ps->esArreglo()) return {};
    for (const auto& p : ps->comoArreglo())
        if (p.esObjeto() && minusculas(cadena(p, "name")) == nombre) return minusculas(cadena(p, "value"));
    return {};
}

struct Conjunto {
    std::uint32_t primerGid{1};
    std::string nombre;
};

struct Lector {
    std::string archivo;
    int ancho{0};
    int alto{0};
    double tw{1.0};
    double th{1.0};
    std::vector<Conjunto> conjuntos;  ///< Por firstgid creciente.

    /// Datos de una capa de patrones, ancho × alto gids sin los bits de volteo.
    std::vector<std::uint32_t> patrones(const ValorJson& capa, std::string_view donde) const {
        const std::string enc = minusculas(cadena(capa, "encoding"));
        if (!enc.empty() && enc != "csv")
            throw ErrorMapa{std::string(donde) + ": codificación «" + enc +
                            "» no admitida; en Tiled elige Mapa → Propiedades → Formato de capa de patrones: CSV"};
        if (capa.buscar("chunks") != nullptr) throw ErrorMapa{std::string(donde) + ": los mapas infinitos no están admitidos"};
        const int w = entero(capa, "width", donde, 1, kMaxLadoTiled);
        const int h = entero(capa, "height", donde, 1, kMaxLadoTiled);
        if (w != ancho || h != alto) throw ErrorMapa{std::string(donde) + ": su tamaño no es el del mapa"};
        const auto& datos = campo(capa, "data", donde).comoArreglo(std::string(donde) + ".«data»");
        if (datos.size() != static_cast<std::size_t>(ancho) * static_cast<std::size_t>(alto))
            throw ErrorMapa{std::string(donde) + ".«data»: tiene " + std::to_string(datos.size()) + " patrones y deberían ser " +
                            std::to_string(ancho * alto)};
        std::vector<std::uint32_t> r;
        r.reserve(datos.size());
        for (std::size_t i = 0; i < datos.size(); ++i) {
            const auto g = datos[i].comoEntero(std::string(donde) + ".«data»[" + std::to_string(i) + "]", 0, 0xFFFFFFFFll);
            r.push_back(static_cast<std::uint32_t>(g) & kMascaraGid);
        }
        return r;
    }

    /// Índice del patrón dentro de su conjunto (gid − firstgid), o -1 para el gid 0.
    [[nodiscard]] int local(std::uint32_t gid) const noexcept {
        if (gid == 0) return -1;
        std::uint32_t primero = 1;
        for (const auto& c : conjuntos)
            if (c.primerGid <= gid) primero = c.primerGid;
        return static_cast<int>(gid - primero);
    }

    /// Celda de un objeto (centro si es un rectángulo), comprobando que cae dentro del mapa.
    [[nodiscard]] Celda celda(const ValorJson& o, std::string_view donde) const {
        const double x = numero(o, "x", donde) + numero(o, "width", donde) / 2.0;
        const double y = numero(o, "y", donde) + numero(o, "height", donde) / 2.0;
        const Celda c{static_cast<int>(std::floor(x / tw)), static_cast<int>(std::floor(y / th))};
        if (c.x < 0 || c.y < 0 || c.x >= ancho || c.y >= alto)
            throw ErrorMapa{std::string(donde) + ": queda fuera del mapa (celda " + std::to_string(c.x) + ", " +
                            std::to_string(c.y) + ")"};
        return c;
    }
};

std::string tipoDe(const ValorJson& o) {
    std::string t = minusculas(cadena(o, "type"));
    if (t.empty()) t = minusculas(cadena(o, "class"));
    if (t.empty()) t = minusculas(cadena(o, "name"));
    return t;
}

std::string descripcion(std::string_view capa, std::size_t i, const ValorJson& o) {
    std::string d = std::string("capa «") + std::string(capa) + "», objeto " + std::to_string(i);
    if (const ValorJson* id = o.buscar("id"); id != nullptr && id->esNumero())
        d += " (id " + std::to_string(static_cast<long long>(id->comoNumero())) + ")";
    return d;
}

Escenario construir(const ValorJson& doc, Lector& lec) {
    const std::string raiz = lec.archivo;
    if (!doc.esObjeto()) throw ErrorMapa{raiz + ": el mapa debe ser un objeto JSON"};
    if (const std::string o = minusculas(cadena(doc, "orientation")); !o.empty() && o != "orthogonal")
        throw ErrorMapa{raiz + ": orientación «" + o + "» no admitida (solo orthogonal)"};
    if (const ValorJson* inf = doc.buscar("infinite"); inf != nullptr && inf->esBool() && inf->comoBool())
        throw ErrorMapa{raiz + ": los mapas infinitos no están admitidos"};
    lec.ancho = entero(doc, "width", raiz, 4, kMaxLadoTiled);
    lec.alto = entero(doc, "height", raiz, 4, kMaxLadoTiled);
    lec.tw = entero(doc, "tilewidth", raiz, 1, 4096);
    lec.th = entero(doc, "tileheight", raiz, 1, 4096);
    if (const ValorJson* ts = doc.buscar("tilesets"); ts != nullptr) {
        const auto& arr = ts->comoArreglo(raiz + ".«tilesets»");
        for (std::size_t i = 0; i < arr.size(); ++i) {
            const std::string d = raiz + ".«tilesets»[" + std::to_string(i) + "]";
            lec.conjuntos.push_back(
                {static_cast<std::uint32_t>(entero(arr[i], "firstgid", d, 1, kMascaraGid)), cadena(arr[i], "name")});
        }
        std::sort(lec.conjuntos.begin(), lec.conjuntos.end(),
                  [](const Conjunto& a, const Conjunto& b) { return a.primerGid < b.primerGid; });
    }

    // Capas por nombre; las de grupo («group») se recorren por dentro.
    const ValorJson* colision = nullptr;
    const ValorJson* costo = nullptr;
    const ValorJson* suelo = nullptr;
    std::vector<std::pair<std::string, const ValorJson*>> grupos;
    std::vector<const ValorJson*> pendientes;
    for (const auto& c : campo(doc, "layers", raiz).comoArreglo(raiz + ".«layers»")) pendientes.push_back(&c);
    for (std::size_t k = 0; k < pendientes.size(); ++k) {
        const ValorJson& capa = *pendientes[k];
        if (!capa.esObjeto()) throw ErrorMapa{raiz + ": cada capa debe ser un objeto"};
        const std::string tipo = minusculas(cadena(capa, "type"));
        const std::string nombre = minusculas(cadena(capa, "name"));
        if (tipo == "group") {
            if (const ValorJson* sub = capa.buscar("layers"); sub != nullptr && sub->esArreglo())
                for (const auto& c : sub->comoArreglo()) pendientes.push_back(&c);
            continue;
        }
        if (tipo == "tilelayer") {
            if (nombre == "colision" || nombre == "colisión") colision = &capa;
            else if (nombre == "costo") costo = &capa;
            else if (nombre == "suelo") suelo = &capa;
        } else if (tipo == "objectgroup") {
            if (nombre == "objetos" || nombre == "spawns" || nombre == "zonas") grupos.emplace_back(nombre, &capa);
        }
    }
    if (colision == nullptr) throw ErrorMapa{raiz + ": falta la capa de patrones «colision»"};

    Escenario e;
    e.mapa = mundo::GrafoRejilla(lec.ancho, lec.alto);
    if (suelo != nullptr) (void)lec.patrones(*suelo, raiz + ": capa «suelo»");
    const auto muros = lec.patrones(*colision, raiz + ": capa «colision»");
    std::vector<std::uint32_t> costos;
    if (costo != nullptr) costos = lec.patrones(*costo, raiz + ": capa «costo»");
    for (int y = 0; y < lec.alto; ++y)
        for (int x = 0; x < lec.ancho; ++x) {
            const auto i = static_cast<std::size_t>(y * lec.ancho + x);
            if (muros[i] != 0) {
                e.mapa.bloquear(x, y);
                continue;
            }
            if (!costos.empty()) {
                const int k = lec.local(costos[i]);
                const int m = std::clamp(k + 1, static_cast<int>(mundo::kCostoMin), static_cast<int>(mundo::kCostoMax));
                e.mapa.fijarMultiplicador(x, y, static_cast<std::uint8_t>(m));
            }
        }

    std::optional<Celda> mara;
    std::optional<Celda> radio;
    std::array<int, kNumTiposObjeto> cuenta{};
    for (const auto& [nombre, capa] : grupos) {
        const auto& objs = campo(*capa, "objects", raiz + ": capa «" + nombre + "»").comoArreglo(nombre);
        for (std::size_t i = 0; i < objs.size(); ++i) {
            const ValorJson& o = objs[i];
            const std::string d = raiz + ": " + descripcion(nombre, i, o);
            if (!o.esObjeto()) throw ErrorMapa{d + ": debe ser un objeto"};
            const std::string t = tipoDe(o);
            const Celda c = lec.celda(o, d);
            const bool libre = e.mapa.transitable(c.x, c.y);
            auto exigirLibre = [&] {
                if (!libre) throw ErrorMapa{d + ": «" + t + "» está sobre un muro"};
            };
            if (nombre == "objetos") {
                if (t == "medicamento" || t == "combustible" || t == "bateria" || t == "batería") {
                    exigirLibre();
                    const TipoObjeto tipo = t == "medicamento"   ? TipoObjeto::Medicamento
                                            : t == "combustible" ? TipoObjeto::Combustible
                                                                 : TipoObjeto::Bateria;
                    e.objetos.push_back({tipo, c, false});
                    ++cuenta[static_cast<std::size_t>(tipo)];
                } else if (t == "radio") {
                    exigirLibre();
                    if (radio) throw ErrorMapa{d + ": hay más de una radio"};
                    radio = c;
                } else if (t == "almacen" || t == "almacén") {
                    exigirLibre();
                    e.almacen = c;
                } else if (t == "puerta" || t == "puerta_metalica" || t == "puerta metálica") {
                    if (e.puertas.size() >= Partida::kMaxPuertas) throw ErrorMapa{d + ": demasiadas puertas"};
                    e.puertas.push_back(c);
                } else {
                    throw ErrorMapa{d + ": tipo «" + t + "» desconocido en «objetos»"};
                }
            } else if (nombre == "spawns") {
                exigirLibre();
                if (t == "mara" || t == "jugadora") {
                    if (mara) throw ErrorMapa{d + ": hay más de un punto de aparición de Mara"};
                    mara = c;
                } else if (t == "superviviente") {
                    e.inicioSuperviviente = agentes::centroDe(c);
                } else if (t == "infectado" || t == "explorador" || t == "perseguidor") {
                    std::string rol = propiedad(o, "rol");
                    if (rol.empty()) rol = t == "infectado" ? "perseguidor" : t;
                    if (rol != "explorador" && rol != "perseguidor")
                        throw ErrorMapa{d + ": rol «" + rol + "» desconocido (explorador o perseguidor)"};
                    if (e.infectados.size() >= static_cast<std::size_t>(kMaxInfectadosTiled))
                        throw ErrorMapa{d + ": más de " + std::to_string(kMaxInfectadosTiled) + " infectados"};
                    e.infectados.push_back(agentes::centroDe(c));
                    e.roles.push_back(rol == "explorador" ? RolInfectado::Explorador : RolInfectado::Perseguidor);
                } else {
                    throw ErrorMapa{d + ": tipo «" + t + "» desconocido en «spawns»"};
                }
            } else {  // zonas
                if (t == "extraccion" || t == "extracción") {
                    exigirLibre();
                    e.extraccion = c;
                } else {
                    throw ErrorMapa{d + ": tipo «" + t + "» desconocido en «zonas»"};
                }
            }
        }
    }
    if (!mara) throw ErrorMapa{raiz + ": falta el punto de aparición «mara» en la capa «spawns»"};
    if (!radio) throw ErrorMapa{raiz + ": falta la «radio» en la capa «objetos»"};
    constexpr const char* kNombres[] = {"medicamento", "combustible", "bateria"};
    for (int t = 0; t < kNumTiposObjeto; ++t)
        if (cuenta[static_cast<std::size_t>(t)] == 0)
            throw ErrorMapa{raiz + ": falta al menos un «" + std::string(kNombres[t]) + "» en la capa «objetos»"};
    for (const Celda p : e.puertas) {
        if (p == *radio || p == *mara) throw ErrorMapa{raiz + ": una puerta no puede estar en la radio ni en el inicio de Mara"};
        if (std::any_of(e.objetos.begin(), e.objetos.end(), [&](const ObjetoMision& o) { return o.celda == p; }))
            throw ErrorMapa{raiz + ": una puerta no puede estar sobre un objeto"};
    }
    e.inicioMara = agentes::centroDe(*mara);
    e.radio = *radio;
    if (e.extraccion.x < 0) e.extraccion = *mara;
    return e;
}

}  // namespace

ResultadoMapaTiled leerMapaTiled(std::string_view texto, std::string_view archivo) {
    ResultadoMapaTiled r;
    try {
        const ValorJson doc = nucleo::leerJson(texto);
        Lector lec;
        lec.archivo = std::string(archivo);
        r.escenario = construir(doc, lec);
    } catch (const ErrorMapa& e) {
        r.error = e.mensaje;
    } catch (const nucleo::ErrorJson& e) {
        r.error = std::string(archivo) + ": " + e.what();
    } catch (const std::exception& e) {
        r.error = std::string(archivo) + ": " + e.what();
    }
    if (!r.valido()) r.escenario = Escenario{};
    return r;
}

}  // namespace refugio::juego
