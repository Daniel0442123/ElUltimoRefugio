#include "dominio/config/Configuracion.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

#include "dominio/juego/Partida.hpp"
#include "dominio/nucleo/Json.hpp"

namespace refugio::config {

using nucleo::ValorJson;

namespace {

/// Analiza el texto completo. Si no es JSON válido o la raíz no es un objeto, anota el error y
/// devuelve nullptr (el que llama se queda con los valores por defecto).
template <class T>
const ValorJson::Objeto* raiz(const ValorJson& doc, Lectura<T>& l, std::string_view archivo) {
    if (doc.esObjeto()) return &doc.comoObjeto();
    l.errores.push_back(std::string(archivo) + ": la raíz debe ser un objeto; se usan los valores por defecto");
    return nullptr;
}

template <class T>
bool analizar(std::string_view texto, std::string_view archivo, Lectura<T>& l, ValorJson& doc) {
    try {
        doc = nucleo::leerJson(texto);
        return true;
    } catch (const nucleo::ErrorJson& e) {
        l.errores.push_back(std::string(archivo) + ": JSON mal formado (" + e.what() + "); se usan los valores por defecto");
        return false;
    }
}

/// Lee un número real en [min, max] de @p obj[clave]; si falta, deja @p destino como está; si el
/// tipo o el rango no valen, anota el error y también lo deja como está (valor por defecto).
template <class T>
void numero(const ValorJson& obj, std::string_view clave, std::string_view ruta, double min, double max,
            float& destino, Lectura<T>& l, std::string_view archivo) {
    const ValorJson* v = obj.buscar(clave);
    if (v == nullptr) return;
    if (!v->esNumero() || !std::isfinite(v->comoNumero()) || v->comoNumero() < min || v->comoNumero() > max) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), " debe ser un número entre %g y %g", min, max);
        l.errores.push_back(std::string(archivo) + ": «" + std::string(ruta) + "»" + buf + "; se usa el valor por defecto");
        return;
    }
    destino = static_cast<float>(v->comoNumero());
}

template <class T>
void entero(const ValorJson& obj, std::string_view clave, std::string_view ruta, long long min, long long max,
            int& destino, Lectura<T>& l, std::string_view archivo) {
    const ValorJson* v = obj.buscar(clave);
    if (v == nullptr) return;
    const bool ok = v->esNumero() && std::isfinite(v->comoNumero()) && std::floor(v->comoNumero()) == v->comoNumero() &&
                    v->comoNumero() >= static_cast<double>(min) && v->comoNumero() <= static_cast<double>(max);
    if (!ok) {
        l.errores.push_back(std::string(archivo) + ": «" + std::string(ruta) + "» debe ser un entero entre " +
                            std::to_string(min) + " y " + std::to_string(max) + "; se usa el valor por defecto");
        return;
    }
    destino = static_cast<int>(v->comoNumero());
}

template <class T>
const ValorJson* seccion(const ValorJson& obj, std::string_view clave, Lectura<T>& l, std::string_view archivo) {
    const ValorJson* v = obj.buscar(clave);
    if (v == nullptr) return nullptr;
    if (!v->esObjeto()) {
        l.errores.push_back(std::string(archivo) + ": «" + std::string(clave) + "» debe ser un objeto; se usan sus valores por defecto");
        return nullptr;
    }
    return v;
}

std::string numeroTexto(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(v));
    return buf;
}

struct NombreTecla {
    int codigo;
    std::string_view nombre;
};
constexpr NombreTecla kTeclasEspeciales[] = {
    {tecla::kEspacio, "SPACE"},        {tecla::kTab, "TAB"},
    {tecla::kRetroceso, "BACKSPACE"},  {tecla::kShiftIzq, "LEFT_SHIFT"},
    {tecla::kCtrlIzq, "LEFT_CONTROL"}, {tecla::kAltIzq, "LEFT_ALT"},
    {tecla::kShiftDer, "RIGHT_SHIFT"}, {tecla::kCtrlDer, "RIGHT_CONTROL"},
    {tecla::kAltDer, "RIGHT_ALT"},
};

struct DatosAccion {
    std::string_view clave;
    std::string_view texto;
    int porDefecto;
};
constexpr std::array<DatosAccion, kNumAcciones> kAcciones{{
    {"arriba", "Mover arriba", 'W'},
    {"abajo", "Mover abajo", 'S'},
    {"izquierda", "Mover a la izquierda", 'A'},
    {"derecha", "Mover a la derecha", 'D'},
    {"correr", "Correr (mantener)", tecla::kShiftIzq},
    {"sigilo", "Sigilo (mantener)", tecla::kCtrlIzq},
    {"interactuar", "Recoger / activar radio", 'E'},
    {"senuelo", "Lanzar señuelo", 'Q'},
    {"barricada", "Poner barricada", 'B'},
    {"guardado_rapido", "Guardado rápido (ranura 1)", tecla::kF1 + 4},
    {"carga_rapida", "Carga rápida (ranura 1)", tecla::kF1 + 8},
    {"saltar_tutorial", "Saltar tutorial", 'T'},
    {"depuracion", "Depuración", tecla::kF1},
    {"algoritmo", "Alternar Dijkstra / A*", tecla::kF1 + 1},
    {"comparar", "Comparar Dijkstra y A*", tecla::kF1 + 2},
    {"modo_horda", "Horda aislada / coordinada", tecla::kF1 + 3},
    {"nodos", "Nodos expandidos", 'N'},
    {"consola", "Consola de IA", 'L'},
    {"nueva_partida", "Nueva partida", 'R'},
    {"laboratorio", "Laboratorio de rutas", tecla::kTab},
    {"pausa_simulacion", "Pausar la simulación", 'P'},
    {"paso_simulacion", "Avanzar un tick (en pausa)", tecla::kF1 + 5},
    {"mas_lento", "Simulación más lenta", tecla::kF1 + 6},
    {"mas_rapido", "Simulación más rápida", tecla::kF1 + 7},
    {"camara", "Cámara: seguir a Mara / mapa completo", 'Z'},
}};

/// Teclas que reciben las acciones nuevas cuya tecla por defecto ya está ocupada (leerControles).
constexpr int kTeclasDeReserva[] = {
    tecla::kF1 + 9, tecla::kF1 + 10, tecla::kF1 + 11, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    'Z', 'X', 'C', 'V', 'F', 'G', 'H', 'J', 'K', 'M', 'O', 'I', 'U', 'Y'};

}  // namespace

// ----------------------------------------------------------------------------- juego.json
Lectura<OpcionesJuego> leerOpciones(std::string_view texto, std::string_view archivo) {
    Lectura<OpcionesJuego> l;
    ValorJson doc;
    if (!analizar(texto, archivo, l, doc) || raiz(doc, l, archivo) == nullptr) return l;
    numero(doc, "volumen", "volumen", 0.0, 1.0, l.valor.volumen, l, archivo);
    if (const ValorJson* t = doc.buscar("tutorial")) {
        if (t->esBool())
            l.valor.tutorial = t->comoBool();
        else
            l.errores.push_back(std::string(archivo) + ": «tutorial» debe ser true o false; se usa el valor por defecto");
    }
    return l;
}

std::string escribirOpciones(const OpcionesJuego& o) {
    return "{\n  \"volumen\": " + numeroTexto(o.volumen) + ",\n  \"tutorial\": " + (o.tutorial ? "true" : "false") + "\n}\n";
}

// ------------------------------------------------------------------------- controles.json
std::string_view claveAccion(Accion a) noexcept { return kAcciones[static_cast<std::size_t>(a)].clave; }
std::string_view textoAccion(Accion a) noexcept { return kAcciones[static_cast<std::size_t>(a)].texto; }

std::string nombreTecla(int codigo) {
    if ((codigo >= 'A' && codigo <= 'Z') || (codigo >= '0' && codigo <= '9')) return std::string(1, static_cast<char>(codigo));
    if (codigo >= tecla::kF1 && codigo <= tecla::kF1 + 11) return "F" + std::to_string(codigo - tecla::kF1 + 1);
    for (const auto& t : kTeclasEspeciales)
        if (t.codigo == codigo) return std::string(t.nombre);
    return {};
}

std::string textoTecla(int codigo) {
    switch (codigo) {
        case tecla::kEspacio: return "Espacio";
        case tecla::kRetroceso: return "Retroceso";
        case tecla::kTab: return "Tab";
        case tecla::kShiftIzq: return "Shift";
        case tecla::kCtrlIzq: return "Ctrl";
        case tecla::kAltIzq: return "Alt";
        case tecla::kShiftDer: return "Shift der.";
        case tecla::kCtrlDer: return "Ctrl der.";
        case tecla::kAltDer: return "Alt der.";
        default: break;
    }
    const std::string n = nombreTecla(codigo);
    return n.empty() ? "?" : n;
}

int teclaDesdeNombre(std::string_view nombre) {
    std::string n(nombre);
    for (auto& c : n)
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    if (n.size() == 1 && ((n[0] >= 'A' && n[0] <= 'Z') || (n[0] >= '0' && n[0] <= '9'))) return n[0];
    if (n.size() >= 2 && n.size() <= 3 && n[0] == 'F') {
        int f = 0;
        for (std::size_t i = 1; i < n.size(); ++i) {
            if (n[i] < '0' || n[i] > '9') return tecla::kNinguna;
            f = f * 10 + (n[i] - '0');
        }
        return f >= 1 && f <= 12 ? tecla::kF1 + f - 1 : tecla::kNinguna;
    }
    for (const auto& t : kTeclasEspeciales)
        if (t.nombre == n) return t.codigo;
    return tecla::kNinguna;
}

bool teclaReservada(int codigo) noexcept {
    return codigo == tecla::kEscape || codigo == tecla::kEnter || (codigo >= tecla::kDerecha && codigo <= tecla::kArriba);
}

bool teclaAsignable(int codigo) { return !teclaReservada(codigo) && !nombreTecla(codigo).empty(); }

Controles::Controles() {
    for (std::size_t i = 0; i < kNumAcciones; ++i) teclas_[i] = kAcciones[i].porDefecto;
}

bool Controles::asignar(Accion a, int codigo) {
    if (!teclaAsignable(codigo)) return false;
    const auto i = static_cast<std::size_t>(a);
    const int otra = accionDe(codigo);
    if (otra >= 0) teclas_[static_cast<std::size_t>(otra)] = teclas_[i];  // intercambio
    teclas_[i] = codigo;
    return true;
}

int Controles::accionDe(int codigo) const noexcept {
    for (std::size_t i = 0; i < kNumAcciones; ++i)
        if (teclas_[i] == codigo) return static_cast<int>(i);
    return -1;
}

Lectura<Controles> leerControles(std::string_view texto, std::string_view archivo) {
    Lectura<Controles> l;
    ValorJson doc;
    if (!analizar(texto, archivo, l, doc) || raiz(doc, l, archivo) == nullptr) return l;
    // Primero se leen todas; luego se comprueba que no se repitan. Si hay repetidas, se vuelve a
    // los controles por defecto completos: arreglar solo una podría dejar otra acción sin tecla.
    std::array<int, kNumAcciones> leidas{};
    std::array<bool, kNumAcciones> enArchivo{};
    const Controles porDefecto;
    for (std::size_t i = 0; i < kNumAcciones; ++i) {
        const auto a = static_cast<Accion>(i);
        leidas[i] = porDefecto.tecla(a);
        const ValorJson* v = doc.buscar(claveAccion(a));
        if (v == nullptr) continue;
        enArchivo[i] = true;
        const int codigo = v->esCadena() ? teclaDesdeNombre(v->comoCadena()) : tecla::kNinguna;
        if (codigo == tecla::kNinguna) {
            l.errores.push_back(std::string(archivo) + ": «" + std::string(claveAccion(a)) +
                                "» no es una tecla conocida; se usa " + nombreTecla(leidas[i]));
        } else if (teclaReservada(codigo)) {
            l.errores.push_back(std::string(archivo) + ": «" + std::string(claveAccion(a)) +
                                "» usa una tecla reservada para el menú; se usa " + nombreTecla(leidas[i]));
        } else {
            leidas[i] = codigo;
        }
    }
    // Un controles.json de la 0.13.0 o anterior no trae las acciones de HU-22. Si la tecla por
    // defecto de una de ellas ya la usa otra acción del archivo, la nueva recibe la primera tecla
    // libre en vez de descartar todos los controles de la jugadora. Con las acciones antiguas no
    // se hace: un archivo que repite una de ellas sigue volviendo a los valores por defecto.
    const auto usada = [&leidas](int codigo) {
        for (const int t : leidas)
            if (t == codigo) return true;
        return false;
    };
    for (auto i = static_cast<std::size_t>(Accion::PausaSimulacion); i < kNumAcciones; ++i) {
        if (enArchivo[i]) continue;
        bool choca = false;
        for (std::size_t j = 0; j < kNumAcciones; ++j) choca = choca || (j != i && enArchivo[j] && leidas[j] == leidas[i]);
        if (!choca) continue;
        for (const int libre : kTeclasDeReserva) {
            if (!usada(libre)) {
                leidas[i] = libre;
                break;
            }
        }
    }
    for (std::size_t i = 0; i < kNumAcciones; ++i)
        for (std::size_t j = i + 1; j < kNumAcciones; ++j)
            if (leidas[i] == leidas[j]) {
                l.errores.push_back(std::string(archivo) + ": la tecla " + nombreTecla(leidas[i]) + " está en «" +
                                    std::string(kAcciones[i].clave) + "» y en «" + std::string(kAcciones[j].clave) +
                                    "»; se usan los controles por defecto");
                return l;
            }
    for (std::size_t i = 0; i < kNumAcciones; ++i) {
        const auto a = static_cast<Accion>(i);
        if (l.valor.tecla(a) != leidas[i]) l.valor.asignar(a, leidas[i]);
    }
    return l;
}

std::string escribirControles(const Controles& c) {
    std::string s = "{\n";
    for (std::size_t i = 0; i < kNumAcciones; ++i) {
        const auto a = static_cast<Accion>(i);
        s += "  \"" + std::string(claveAccion(a)) + "\": \"" + nombreTecla(c.tecla(a)) + "\"";
        s += i + 1 < kNumAcciones ? ",\n" : "\n";
    }
    return s + "}\n";
}

// ------------------------------------------------------------------------------- ia.json
std::string validar(const ParametrosIa& p) {
    const auto en = [](float v, float min, float max) { return std::isfinite(v) && v >= min && v <= max; };
    if (p.tickSimHz < 10 || p.tickSimHz > 240) return "tick_sim_hz debe estar entre 10 y 240";
    if (p.tickIaHz < 1 || p.tickIaHz > p.tickSimHz || p.tickSimHz % p.tickIaHz != 0)
        return "tick_ia_hz debe dividir a tick_sim_hz";
    if (!en(p.velSigilo, 0.1f, 20.0f) || !en(p.velCaminar, 0.1f, 20.0f) || !en(p.velCorrer, 0.1f, 20.0f))
        return "las velocidades de movimiento deben estar entre 0,1 y 20";
    if (!en(p.ruidoSigilo, 0.0f, 30.0f) || !en(p.ruidoCaminar, 0.0f, 30.0f) || !en(p.ruidoCorrer, 0.0f, 30.0f))
        return "los radios de ruido deben estar entre 0 y 30";
    if (!en(p.tInstalacion, 0.1f, 600.0f)) return "mision.t_instalacion_s debe estar entre 0,1 y 600";
    if (!en(p.tExtraccion, 1.0f, 3600.0f)) return "mision.t_extraccion_s debe estar entre 1 y 3600";
    if (!en(p.saludMax, 1.0f, 10000.0f)) return "mision.salud_max debe estar entre 1 y 10000";
    if (!en(p.integridadMax, 1.0f, 10000.0f)) return "mision.integridad_refugio_max debe estar entre 1 y 10000";
    if (!en(p.senueloAlcance, 1.0f, 64.0f) || !en(p.senueloRadio, 1.0f, 64.0f))
        return "senuelo.alcance y senuelo.radio deben estar entre 1 y 64";
    if (!en(p.senueloPeriodo, 0.1f, 60.0f) || !en(p.senueloDuracion, p.senueloPeriodo, 600.0f))
        return "senuelo.periodo_s debe estar entre 0,1 y 60 y duracion_s entre el periodo y 600";
    if (!en(p.semividaMemoria, 0.1f, 600.0f)) return "memoria.semivida_s debe estar entre 0,1 y 600";
    if (!en(p.radioComunicacion, 0.0f, 64.0f)) return "alertas.radio_comunicacion debe estar entre 0 y 64";
    if (p.profundidadMax < 0 || p.profundidadMax > 16) return "alertas.profundidad_max debe estar entre 0 y 16";
    if (!en(p.atenuacion, 0.01f, 1.0f)) return "alertas.atenuacion debe estar entre 0,01 y 1";
    if (p.maxAgentesPorCelda < 1 || p.maxAgentesPorCelda > 8) return "coordinacion.max_agentes_por_celda debe estar entre 1 y 8";
    if (p.flowfieldUmbral < 1 || p.flowfieldUmbral > 1000) return "coordinacion.flowfield_umbral debe estar entre 1 y 1000";
    if (p.presupuestoNodosTick < 100 || p.presupuestoNodosTick > 1000000)
        return "navegacion.presupuesto_nodos_tick debe estar entre 100 y 1000000";
    if (!en(p.factorDesvioBarricada, 1.0f, 100.0f)) return "navegacion.factor_desvio_barricada debe estar entre 1 y 100";
    for (const auto& c : camposPesos())
        if (!en(p.pesos.*c.campo, c.minimo, c.maximo)) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), " debe estar entre %g y %g", static_cast<double>(c.minimo),
                          static_cast<double>(c.maximo));
            return "pesos." + std::string(c.seccion) + "." + std::string(c.clave) + buf;
        }
    return {};
}

Lectura<ParametrosIa> leerParametrosIa(std::string_view texto, std::string_view archivo) {
    Lectura<ParametrosIa> l;
    ValorJson doc;
    if (!analizar(texto, archivo, l, doc) || raiz(doc, l, archivo) == nullptr) return l;
    auto& p = l.valor;
    entero(doc, "tick_sim_hz", "tick_sim_hz", 10, 240, p.tickSimHz, l, archivo);
    entero(doc, "tick_ia_hz", "tick_ia_hz", 1, 240, p.tickIaHz, l, archivo);
    if (const auto* s = seccion(doc, "movimiento", l, archivo)) {
        numero(*s, "vel_sigilo", "movimiento.vel_sigilo", 0.1, 20, p.velSigilo, l, archivo);
        numero(*s, "vel_caminar", "movimiento.vel_caminar", 0.1, 20, p.velCaminar, l, archivo);
        numero(*s, "vel_correr", "movimiento.vel_correr", 0.1, 20, p.velCorrer, l, archivo);
    }
    if (const auto* s = seccion(doc, "ruido", l, archivo)) {
        numero(*s, "sigilo", "ruido.sigilo", 0, 30, p.ruidoSigilo, l, archivo);
        numero(*s, "caminar", "ruido.caminar", 0, 30, p.ruidoCaminar, l, archivo);
        numero(*s, "correr", "ruido.correr", 0, 30, p.ruidoCorrer, l, archivo);
    }
    if (const auto* s = seccion(doc, "mision", l, archivo)) {
        numero(*s, "t_instalacion_s", "mision.t_instalacion_s", 0.1, 600, p.tInstalacion, l, archivo);
        numero(*s, "t_extraccion_s", "mision.t_extraccion_s", 1, 3600, p.tExtraccion, l, archivo);
        numero(*s, "salud_max", "mision.salud_max", 1, 10000, p.saludMax, l, archivo);
        numero(*s, "integridad_refugio_max", "mision.integridad_refugio_max", 1, 10000, p.integridadMax, l, archivo);
    }
    if (const auto* s = seccion(doc, "senuelo", l, archivo)) {
        numero(*s, "alcance", "senuelo.alcance", 1, 64, p.senueloAlcance, l, archivo);
        numero(*s, "radio", "senuelo.radio", 1, 64, p.senueloRadio, l, archivo);
        numero(*s, "duracion_s", "senuelo.duracion_s", 0.1, 600, p.senueloDuracion, l, archivo);
        numero(*s, "periodo_s", "senuelo.periodo_s", 0.1, 60, p.senueloPeriodo, l, archivo);
    }
    if (const auto* s = seccion(doc, "memoria", l, archivo))
        numero(*s, "semivida_s", "memoria.semivida_s", 0.1, 600, p.semividaMemoria, l, archivo);
    if (const auto* s = seccion(doc, "alertas", l, archivo)) {
        numero(*s, "radio_comunicacion", "alertas.radio_comunicacion", 0, 64, p.radioComunicacion, l, archivo);
        entero(*s, "profundidad_max", "alertas.profundidad_max", 0, 16, p.profundidadMax, l, archivo);
        numero(*s, "atenuacion", "alertas.atenuacion", 0.01, 1, p.atenuacion, l, archivo);
    }
    if (const auto* s = seccion(doc, "coordinacion", l, archivo)) {
        entero(*s, "max_agentes_por_celda", "coordinacion.max_agentes_por_celda", 1, 8, p.maxAgentesPorCelda, l, archivo);
        entero(*s, "flowfield_umbral", "coordinacion.flowfield_umbral", 1, 1000, p.flowfieldUmbral, l, archivo);
    }
    if (const auto* s = seccion(doc, "navegacion", l, archivo)) {
        entero(*s, "presupuesto_nodos_tick", "navegacion.presupuesto_nodos_tick", 100, 1000000, p.presupuestoNodosTick, l, archivo);
        numero(*s, "factor_desvio_barricada", "navegacion.factor_desvio_barricada", 1, 100, p.factorDesvioBarricada, l, archivo);
    }
    if (const auto* s = seccion(doc, "pesos", l, archivo)) {
        for (const auto& c : camposPesos()) {
            const ValorJson* sub = s->buscar(c.seccion);
            if (sub == nullptr) continue;
            const std::string ruta = "pesos." + std::string(c.seccion);
            if (!sub->esObjeto()) {
                if (&c == &camposPesos()[0] || c.seccion != (&c - 1)->seccion)
                    l.errores.push_back(std::string(archivo) + ": «" + ruta + "» debe ser un objeto; se usan sus valores por defecto");
                continue;
            }
            numero(*sub, c.clave, ruta + "." + std::string(c.clave), c.minimo, c.maximo, p.pesos.*c.campo, l, archivo);
        }
    }
    // Reglas entre campos (tick_ia_hz divide a tick_sim_hz, duración >= periodo): si fallan, todo
    // ia.json vuelve a los valores por defecto para no mezclar un archivo a medias.
    if (const std::string e = validar(p); !e.empty()) {
        l.errores.push_back(std::string(archivo) + ": " + e + "; se usan los valores por defecto");
        p = ParametrosIa{};
    }
    return l;
}

void aplicar(const ParametrosIa& p, juego::ConfigPartida& cfg) {
    cfg.tickSimHz = p.tickSimHz;
    cfg.tickIaHz = p.tickIaHz;
    cfg.velSigilo = p.velSigilo;
    cfg.velCaminar = p.velCaminar;
    cfg.velCorrer = p.velCorrer;
    cfg.ruidoSigilo = p.ruidoSigilo;
    cfg.ruidoCaminar = p.ruidoCaminar;
    cfg.ruidoCorrer = p.ruidoCorrer;
    cfg.tInstalacion = p.tInstalacion;
    cfg.tExtraccion = p.tExtraccion;
    cfg.saludMax = p.saludMax;
    cfg.integridadMax = p.integridadMax;
    cfg.integridadInicial = p.integridadMax;
    cfg.senueloAlcance = p.senueloAlcance;
    cfg.senueloRadio = p.senueloRadio;
    cfg.senueloPeriodo = p.senueloPeriodo;
    cfg.senueloRuidos = std::max(1, static_cast<int>(std::lround(p.senueloDuracion / p.senueloPeriodo)));
    cfg.semividaMemoria = p.semividaMemoria;
    cfg.alertas.radioComunicacion = p.radioComunicacion;
    cfg.alertas.profundidadMax = p.profundidadMax;
    cfg.alertas.atenuacion = p.atenuacion;
    cfg.maxAgentesPorCelda = p.maxAgentesPorCelda;
    cfg.flowfieldUmbral = p.flowfieldUmbral;
    cfg.presupuestoNodosTick = p.presupuestoNodosTick;
    cfg.factorDesvioBarricada = p.factorDesvioBarricada;
    aplicar(p.pesos, cfg);
}

const std::array<CampoPesos, kNumPesos>& camposPesos() noexcept {
    static const std::array<CampoPesos, kNumPesos> campos{{
        {"steering", "seguir", 0.0f, 100.0f, &PesosIa::steeringSeguir},
        {"steering", "separacion", 0.0f, 100.0f, &PesosIa::steeringSeparacion},
        {"steering", "alineacion", 0.0f, 10.0f, &PesosIa::steeringAlineacion},
        {"steering", "cohesion", 0.0f, 10.0f, &PesosIa::steeringCohesion},
        {"steering", "anticipacion", 0.0f, 200.0f, &PesosIa::steeringAnticipacion},
        {"tactica", "utilidad_explorar", 0.0f, 1.0f, &PesosIa::utilidadExplorar},
        {"tactica", "peso_interceptar", 0.0f, 5.0f, &PesosIa::pesoInterceptar},
        {"tactica", "histeresis", 1.0f, 3.0f, &PesosIa::histeresisTactica},
        {"infectados", "vision_radio", 0.5f, 30.0f, &PesosIa::visionRadio},
        {"infectados", "vel_perseguir", 0.1f, 20.0f, &PesosIa::velPerseguir},
        {"infectados", "danio_por_segundo", 0.0f, 1000.0f, &PesosIa::danioPorSegundo},
    }};
    return campos;
}

void aplicar(const PesosIa& p, juego::ConfigPartida& cfg) {
    cfg.steering.pesoSeguir = p.steeringSeguir;
    cfg.steering.pesoSeparacion = p.steeringSeparacion;
    cfg.steering.pesoAlineacion = p.steeringAlineacion;
    cfg.steering.pesoCohesion = p.steeringCohesion;
    cfg.steering.pesoAnticipacion = p.steeringAnticipacion;
    cfg.tactica.utilidadExplorar = p.utilidadExplorar;
    cfg.tactica.pesoInterceptar = p.pesoInterceptar;
    cfg.histeresisTactica = p.histeresisTactica;
    cfg.visionRadio = p.visionRadio;
    cfg.velPerseguir = p.velPerseguir;
    cfg.danioPorSegundo = p.danioPorSegundo;
}

std::string escribirPesos(const PesosIa& p, int sangria) {
    const std::string s0(static_cast<std::size_t>(std::max(0, sangria)), ' ');
    std::string t = "{";
    std::string_view seccionActual;
    for (const auto& c : camposPesos()) {
        if (c.seccion != seccionActual) {
            if (!seccionActual.empty()) t += " },";
            t += "\n" + s0 + "  \"" + std::string(c.seccion) + "\": {";
            seccionActual = c.seccion;
        } else {
            t += ",";
        }
        char buf[48];
        std::snprintf(buf, sizeof(buf), "%.6g", static_cast<double>(p.*c.campo));
        t += " \"" + std::string(c.clave) + "\": " + buf;
    }
    t += " }\n" + s0 + "}";
    return t;
}

}  // namespace refugio::config
