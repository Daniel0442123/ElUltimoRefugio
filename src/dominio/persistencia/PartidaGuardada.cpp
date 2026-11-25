#include "dominio/persistencia/PartidaGuardada.hpp"

#include <bit>
#include <cmath>
#include <exception>
#include <utility>

#include "dominio/persistencia/Crc32.hpp"

#ifndef REFUGIO_VERSION
#define REFUGIO_VERSION "desconocida"
#endif

namespace refugio::persistencia {

using juego::EntradaJugador;
using juego::Partida;

namespace {

constexpr std::uint8_t kMagia[4] = {'R', 'E', 'F', 'G'};
constexpr std::size_t kMaxVersionJuego = 32;
// Bits de EntradaTick
constexpr std::uint8_t kBitInteractuar = 1u << 0;
constexpr std::uint8_t kBitSenuelo = 1u << 1;
constexpr std::uint8_t kBitBarricada = 1u << 2;
constexpr std::uint8_t kBitDijkstra = 1u << 3;
constexpr std::uint8_t kBitCoordinado = 1u << 4;
constexpr std::uint8_t kBitsConocidos = 0x1F;
constexpr float kMaxApuntar = 4096.0f;

// ------------------------------------------------------------------------- Escritura
class Escritor {
public:
    std::vector<std::uint8_t> b;
    void u8(std::uint8_t v) { b.push_back(v); }
    void u16(std::uint16_t v) {
        for (int i = 0; i < 2; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    }
    void u32(std::uint32_t v) {
        for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    }
    void u64(std::uint64_t v) {
        for (int i = 0; i < 8; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    }
    void f32(float v) { u32(std::bit_cast<std::uint32_t>(v)); }
    /// Entero sin signo de longitud variable (LEB128): 1 byte hasta 127, 2 hasta 16383…
    void varint(std::uint32_t v) {
        while (v >= 0x80u) {
            b.push_back(static_cast<std::uint8_t>(v | 0x80u));
            v >>= 7;
        }
        b.push_back(static_cast<std::uint8_t>(v));
    }
};

// --------------------------------------------------------------------------- Lectura
/// Lector que nunca lee fuera del búfer: al quedarse sin bytes marca ok = false y devuelve 0.
class Lector {
public:
    explicit Lector(std::span<const std::uint8_t> d) : d_(d) {}
    bool ok{true};
    [[nodiscard]] std::size_t restantes() const noexcept { return d_.size() - pos_; }
    std::uint8_t u8() { return static_cast<std::uint8_t>(leer(1)); }
    std::uint16_t u16() { return static_cast<std::uint16_t>(leer(2)); }
    std::uint32_t u32() { return static_cast<std::uint32_t>(leer(4)); }
    std::uint64_t u64() { return leer(8); }
    float f32() { return std::bit_cast<float>(u32()); }
    std::uint32_t varint() {
        std::uint32_t v = 0;
        for (int i = 0; i < 5; ++i) {
            const std::uint8_t c = u8();
            if (!ok) return 0;
            if (i == 4 && c > 0x0Fu) break;  // no cabe en 32 bits
            v |= static_cast<std::uint32_t>(c & 0x7Fu) << (7 * i);
            if ((c & 0x80u) == 0) return v;
        }
        ok = false;
        return 0;
    }

private:
    std::uint64_t leer(std::size_t n) {
        if (!ok || restantes() < n) {
            ok = false;
            return 0;
        }
        std::uint64_t v = 0;
        for (std::size_t i = 0; i < n; ++i) v |= static_cast<std::uint64_t>(d_[pos_ + i]) << (8 * i);
        pos_ += n;
        return v;
    }
    std::span<const std::uint8_t> d_;
    std::size_t pos_{0};
};

std::uint32_t leerU32(std::span<const std::uint8_t> d, std::size_t pos) noexcept {
    std::uint32_t v = 0;
    for (std::size_t i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(d[pos + i]) << (8 * i);
    return v;
}

struct Fnv {
    std::uint64_t h{1469598103934665603ULL};
    void mezclar(std::uint64_t v) noexcept {
        for (int i = 0; i < 8; ++i) {
            h ^= (v >> (8 * i)) & 0xFFU;
            h *= 1099511628211ULL;
        }
    }
    void mezclar(float f) noexcept { mezclar(static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(f))); }
    void mezclar(bool b) noexcept { mezclar(static_cast<std::uint64_t>(b ? 1u : 0u)); }
    void mezclar(int v) noexcept { mezclar(static_cast<std::uint64_t>(static_cast<std::uint32_t>(v))); }
    void mezclar(mundo::Celda c) noexcept {
        mezclar(c.x);
        mezclar(c.y);
    }
    void mezclar(navegacion::Vec2 v) noexcept {
        mezclar(v.x);
        mezclar(v.y);
    }
};

bool finito(float v, float limite) noexcept { return std::isfinite(v) && std::fabs(v) <= limite; }

void escribirIa(Escritor& w, const config::ParametrosIa& p) {
    w.u16(static_cast<std::uint16_t>(p.tickSimHz));
    w.u16(static_cast<std::uint16_t>(p.tickIaHz));
    for (const float v : {p.velSigilo, p.velCaminar, p.velCorrer, p.ruidoSigilo, p.ruidoCaminar, p.ruidoCorrer,
                          p.tInstalacion, p.tExtraccion, p.saludMax, p.integridadMax, p.senueloAlcance, p.senueloRadio,
                          p.senueloDuracion, p.senueloPeriodo, p.semividaMemoria, p.radioComunicacion, p.atenuacion,
                          p.factorDesvioBarricada})
        w.f32(v);
    for (const int v : {p.profundidadMax, p.maxAgentesPorCelda, p.flowfieldUmbral, p.presupuestoNodosTick})
        w.u32(static_cast<std::uint32_t>(v));
}

config::ParametrosIa leerIa(Lector& r) {
    config::ParametrosIa p;
    p.tickSimHz = r.u16();
    p.tickIaHz = r.u16();
    for (float* v : {&p.velSigilo, &p.velCaminar, &p.velCorrer, &p.ruidoSigilo, &p.ruidoCaminar, &p.ruidoCorrer,
                     &p.tInstalacion, &p.tExtraccion, &p.saludMax, &p.integridadMax, &p.senueloAlcance, &p.senueloRadio,
                     &p.senueloDuracion, &p.senueloPeriodo, &p.semividaMemoria, &p.radioComunicacion, &p.atenuacion,
                     &p.factorDesvioBarricada})
        *v = r.f32();
    for (int* v : {&p.profundidadMax, &p.maxAgentesPorCelda, &p.flowfieldUmbral, &p.presupuestoNodosTick}) {
        const std::uint32_t x = r.u32();
        *v = x > 0x7FFFFFFFu ? -1 : static_cast<int>(x);
    }
    return p;
}

ResultadoLecturaPartida fallo(ErrorCarga e, std::string detalle) {
    ResultadoLecturaPartida r;
    r.error = e;
    r.detalle = std::move(detalle);
    return r;
}

}  // namespace

// ------------------------------------------------------------------------------ Inicio
juego::ConfigPartida configDe(const InicioPartida& inicio) {
    juego::ConfigPartida cfg;
    config::aplicar(inicio.ia, cfg);
    cfg.flowfieldUmbral = inicio.flowfieldUmbral;
    cfg.algoritmo = inicio.algoritmo;
    cfg.coordinado = inicio.coordinado;
    return cfg;
}

std::unique_ptr<Partida> crearPartida(const InicioPartida& inicio) {
    return std::make_unique<Partida>(
        juego::crearEscenarioAleatorio(inicio.ancho, inicio.alto, inicio.infectados, inicio.semilla), configDe(inicio),
        inicio.semilla);
}

bool iguales(const EntradaTick& a, const EntradaTick& b) noexcept {
    const auto bits = [](float f) { return std::bit_cast<std::uint32_t>(f); };
    const auto& x = a.entrada;
    const auto& y = b.entrada;
    return bits(x.ejeX) == bits(y.ejeX) && bits(x.ejeY) == bits(y.ejeY) && x.modo == y.modo &&
           x.interactuar == y.interactuar && x.lanzarSenuelo == y.lanzarSenuelo &&
           x.colocarBarricada == y.colocarBarricada && bits(x.apuntar.x) == bits(y.apuntar.x) &&
           bits(x.apuntar.y) == bits(y.apuntar.y) && a.algoritmo == b.algoritmo && a.coordinado == b.coordinado;
}

// ------------------------------------------------------------------------------ Huella
std::uint64_t huellaPartida(const Partida& p) noexcept {
    Fnv f;
    f.mezclar(p.tick());
    f.mezclar(static_cast<int>(p.estado()));
    f.mezclar(static_cast<int>(p.motivoFin()));
    const auto& m = p.mara();
    f.mezclar(m.pos);
    f.mezclar(m.orientacion);
    f.mezclar(m.salud);
    f.mezclar(m.progresoInstalacion);
    f.mezclar(m.senuelos);
    f.mezclar(m.barricadas);
    for (const bool b : m.inventario) f.mezclar(b);
    f.mezclar(p.integridad());
    f.mezclar(p.restanteExtraccion());
    f.mezclar(p.activacionesRadio());
    f.mezclar(p.cambiosMapa());
    for (const auto& o : p.objetos()) f.mezclar(o.recogido);
    for (const auto& z : p.infectados()) {
        f.mezclar(z.pos);
        f.mezclar(z.orientacion);
        f.mezclar(z.vel);
        f.mezclar(static_cast<int>(z.estado));
        f.mezclar(static_cast<int>(z.tactica));
        f.mezclar(z.objetivo);
        f.mezclar(z.tieneObjetivo);
        f.mezclar(static_cast<std::uint64_t>(z.ruta.size()));
        f.mezclar(static_cast<std::uint64_t>(z.indiceRuta));
    }
    for (const auto& s : p.senuelos()) {
        f.mezclar(s.activo);
        f.mezclar(s.celda);
        f.mezclar(s.ruidosRestantes);
    }
    for (const auto& b : p.barricadas()) {
        f.mezclar(b.activa);
        f.mezclar(b.celda);
        f.mezclar(b.durabilidad);
    }
    const auto& s = p.superviviente();
    f.mezclar(s.activo);
    f.mezclar(s.pos);
    f.mezclar(static_cast<int>(s.meta));
    f.mezclar(static_cast<int>(s.actividad));
    f.mezclar(s.tablas);
    f.mezclar(p.totalTransiciones());
    f.mezclar(p.totalCambiosPlan());
    f.mezclar(p.totalMensajes());
    f.mezclar(p.busquedasTotales());
    return f.h;
}

// ---------------------------------------------------------------------------- Grabador
GrabadorPartida::GrabadorPartida(const InicioPartida& inicio) : inicio_(inicio) {
    tramos_.reserve(4096);  // ~10 minutos de juego sin volver a pedir memoria
}

EntradaJugador GrabadorPartida::registrar(const EntradaJugador& e, const Partida& p) {
    EntradaTick t{e, p.algoritmo(), p.config().coordinado};
    if (!e.lanzarSenuelo && !e.colocarBarricada) t.entrada.apuntar = {};
    if (!tramos_.empty() && tramos_.back().repeticiones < 0xFFFFFFFFu && iguales(tramos_.back().tick, t))
        ++tramos_.back().repeticiones;
    else
        tramos_.push_back({t, 1});
    ++ticks_;
    return t.entrada;
}

void GrabadorPartida::anadir(const Tramo& t) {
    tramos_.push_back(t);
    ticks_ += t.repeticiones;
}

PartidaGuardada instantanea(const GrabadorPartida& g, const Partida& p) {
    PartidaGuardada s;
    s.versionJuego = REFUGIO_VERSION;
    s.inicio = g.inicio();
    s.tramos = g.tramos();
    s.ticks = static_cast<std::uint32_t>(g.ticks());
    s.huella = huellaPartida(p);
    return s;
}

// --------------------------------------------------------------------------- Codificar
std::vector<std::uint8_t> codificar(const PartidaGuardada& g) {
    Escritor c;
    const std::size_t lv = std::min(g.versionJuego.size(), kMaxVersionJuego);
    c.u8(static_cast<std::uint8_t>(lv));
    for (std::size_t i = 0; i < lv; ++i) c.u8(static_cast<std::uint8_t>(g.versionJuego[i]));
    const auto& in = g.inicio;
    c.u64(in.semilla);
    c.u16(static_cast<std::uint16_t>(in.ancho));
    c.u16(static_cast<std::uint16_t>(in.alto));
    c.u16(static_cast<std::uint16_t>(in.infectados));
    c.u16(static_cast<std::uint16_t>(in.flowfieldUmbral));
    c.u8(in.algoritmo == navegacion::Algoritmo::Dijkstra ? 0 : 1);
    c.u8(in.coordinado ? 1 : 0);
    escribirIa(c, in.ia);
    c.u32(g.ticks);
    c.u64(g.huella);
    c.u32(static_cast<std::uint32_t>(g.tramos.size()));
    for (const auto& t : g.tramos) {
        const auto& e = t.tick.entrada;
        std::uint8_t bits = 0;
        if (e.interactuar) bits |= kBitInteractuar;
        if (e.lanzarSenuelo) bits |= kBitSenuelo;
        if (e.colocarBarricada) bits |= kBitBarricada;
        if (t.tick.algoritmo == navegacion::Algoritmo::Dijkstra) bits |= kBitDijkstra;
        if (t.tick.coordinado) bits |= kBitCoordinado;
        c.u8(bits);
        c.u8(static_cast<std::uint8_t>(e.modo));
        c.f32(e.ejeX);
        c.f32(e.ejeY);
        if (e.lanzarSenuelo || e.colocarBarricada) {
            c.f32(e.apuntar.x);
            c.f32(e.apuntar.y);
        }
        c.varint(t.repeticiones);
    }

    Escritor a;
    for (const auto m : kMagia) a.u8(m);
    a.u16(kVersionFormato);
    a.u16(0);
    a.u32(static_cast<std::uint32_t>(c.b.size()));
    a.u32(crc32(c.b));
    a.b.insert(a.b.end(), c.b.begin(), c.b.end());
    return std::move(a.b);
}

// ------------------------------------------------------------------------- Decodificar
std::string mensajeError(ErrorCarga e, int ranura) {
    switch (e) {
        case ErrorCarga::Ninguno: return "Partida cargada";
        case ErrorCarga::Vacia: return "La ranura " + std::to_string(ranura) + " está vacía";
        case ErrorCarga::Lectura: return "No se pudo leer la ranura " + std::to_string(ranura);
        case ErrorCarga::Version: return "La partida guardada es de una versión más nueva del juego";
        case ErrorCarga::OtraVersionJuego: return "La partida guardada es de otra versión del juego y no se puede continuar";
        case ErrorCarga::Grande:
        case ErrorCarga::Cabecera:
        case ErrorCarga::Longitud:
        case ErrorCarga::Crc:
        case ErrorCarga::Formato:
        case ErrorCarga::Huella: return "La partida guardada está dañada";
    }
    return "La partida guardada está dañada";
}

ResultadoLecturaPartida decodificar(std::span<const std::uint8_t> bytes) {
    if (bytes.size() > kMaxBytesArchivo) return fallo(ErrorCarga::Grande, "más de 10 MB");
    if (bytes.size() < kTamCabecera) return fallo(ErrorCarga::Cabecera, "archivo más corto que la cabecera");
    for (std::size_t i = 0; i < 4; ++i)
        if (bytes[i] != kMagia[i]) return fallo(ErrorCarga::Cabecera, "no empieza por REFG");
    const auto version = static_cast<std::uint16_t>(bytes[4] | (bytes[5] << 8));
    if (version == 0 || version > kVersionFormato)
        return fallo(ErrorCarga::Version, "versión de formato " + std::to_string(version));
    if (bytes[6] != 0 || bytes[7] != 0) return fallo(ErrorCarga::Cabecera, "campo reservado distinto de 0");
    const std::uint32_t longitud = leerU32(bytes, 8);
    if (longitud != bytes.size() - kTamCabecera)
        return fallo(ErrorCarga::Longitud, "la cabecera dice " + std::to_string(longitud) + " bytes y hay " +
                                               std::to_string(bytes.size() - kTamCabecera));
    const auto carga = bytes.subspan(kTamCabecera);
    if (crc32(carga) != leerU32(bytes, 12)) return fallo(ErrorCarga::Crc, "CRC-32 incorrecto");

    ResultadoLecturaPartida res;
    auto& g = res.datos;
    Lector r(carga);
    const std::uint8_t lv = r.u8();
    if (lv > kMaxVersionJuego) return fallo(ErrorCarga::Formato, "versión del juego demasiado larga");
    for (std::uint8_t i = 0; i < lv && r.ok; ++i) g.versionJuego.push_back(static_cast<char>(r.u8()));
    auto& in = g.inicio;
    in.semilla = r.u64();
    in.ancho = r.u16();
    in.alto = r.u16();
    in.infectados = r.u16();
    in.flowfieldUmbral = r.u16();
    const std::uint8_t alg = r.u8();
    const std::uint8_t coord = r.u8();
    in.ia = leerIa(r);
    g.ticks = r.u32();
    g.huella = r.u64();
    const std::uint32_t numTramos = r.u32();
    if (!r.ok) return fallo(ErrorCarga::Formato, "cabecera de la partida truncada");
    if (in.ancho < 16 || in.ancho > 256 || in.alto < 16 || in.alto > 256)
        return fallo(ErrorCarga::Formato, "tamaño de mapa fuera de rango");
    if (in.infectados > 256) return fallo(ErrorCarga::Formato, "demasiados infectados");
    if (in.flowfieldUmbral < 1 || in.flowfieldUmbral > 1000) return fallo(ErrorCarga::Formato, "flowfield_umbral fuera de rango");
    if (alg > 1 || coord > 1) return fallo(ErrorCarga::Formato, "algoritmo o modo inválido");
    in.algoritmo = alg == 0 ? navegacion::Algoritmo::Dijkstra : navegacion::Algoritmo::AEstrella;
    in.coordinado = coord == 1;
    if (const std::string e = config::validar(in.ia); !e.empty()) return fallo(ErrorCarga::Formato, "parámetros de IA: " + e);
    if (g.ticks > kMaxTicks) return fallo(ErrorCarga::Formato, "demasiados ticks");
    // Cada tramo ocupa al menos 11 bytes: así un número enorme no reserva memoria de más.
    if (numTramos > g.ticks || numTramos > r.restantes() / 11) return fallo(ErrorCarga::Formato, "número de tramos inválido");

    g.tramos.reserve(numTramos);
    std::uint64_t suma = 0;
    for (std::uint32_t i = 0; i < numTramos; ++i) {
        Tramo t;
        const std::uint8_t bits = r.u8();
        const std::uint8_t modo = r.u8();
        auto& e = t.tick.entrada;
        e.ejeX = r.f32();
        e.ejeY = r.f32();
        e.interactuar = (bits & kBitInteractuar) != 0;
        e.lanzarSenuelo = (bits & kBitSenuelo) != 0;
        e.colocarBarricada = (bits & kBitBarricada) != 0;
        if (e.lanzarSenuelo || e.colocarBarricada) {
            e.apuntar.x = r.f32();
            e.apuntar.y = r.f32();
        }
        t.repeticiones = r.varint();
        if (!r.ok) return fallo(ErrorCarga::Formato, "tramo " + std::to_string(i) + " truncado");
        if ((bits & ~kBitsConocidos) != 0 || modo > 2 || !finito(e.ejeX, 1.0f) || !finito(e.ejeY, 1.0f) ||
            !finito(e.apuntar.x, kMaxApuntar) || !finito(e.apuntar.y, kMaxApuntar) || t.repeticiones == 0)
            return fallo(ErrorCarga::Formato, "tramo " + std::to_string(i) + " fuera de rango");
        e.modo = static_cast<juego::ModoMovimiento>(modo);
        t.tick.algoritmo = (bits & kBitDijkstra) != 0 ? navegacion::Algoritmo::Dijkstra : navegacion::Algoritmo::AEstrella;
        t.tick.coordinado = (bits & kBitCoordinado) != 0;
        suma += t.repeticiones;
        g.tramos.push_back(t);
    }
    if (suma != g.ticks) return fallo(ErrorCarga::Formato, "los tramos no suman los ticks");
    if (r.restantes() != 0) return fallo(ErrorCarga::Formato, "bytes sobrantes al final");
    return res;
}

// --------------------------------------------------------------------------- Restaurar
PartidaRestaurada restaurar(const PartidaGuardada& g) {
    PartidaRestaurada r;
    try {
        r.partida = crearPartida(g.inicio);
        r.grabador = GrabadorPartida(g.inicio);
        for (const auto& t : g.tramos) {
            for (std::uint32_t k = 0; k < t.repeticiones; ++k) {
                r.partida->fijarAlgoritmo(t.tick.algoritmo);
                r.partida->fijarCoordinado(t.tick.coordinado);
                r.partida->paso(t.tick.entrada);
            }
            r.grabador.anadir(t);
        }
    } catch (const std::exception& e) {
        r.partida.reset();
        r.error = ErrorCarga::Formato;
        r.detalle = std::string("no se pudo reconstruir: ") + e.what();
        return r;
    }
    if (huellaPartida(*r.partida) != g.huella) {
        r.partida.reset();
        const bool otraVersion = g.versionJuego != REFUGIO_VERSION;
        r.error = otraVersion ? ErrorCarga::OtraVersionJuego : ErrorCarga::Huella;
        r.detalle = otraVersion ? "guardada con la versión " + g.versionJuego + " y este juego es la " REFUGIO_VERSION
                                : "la huella del estado no coincide";
    }
    return r;
}

PartidaRestaurada cargar(std::span<const std::uint8_t> bytes) {
    auto l = decodificar(bytes);
    if (!l.valida()) {
        PartidaRestaurada r;
        r.error = l.error;
        r.detalle = std::move(l.detalle);
        return r;
    }
    return restaurar(l.datos);
}

}  // namespace refugio::persistencia
