#include "adaptadores/raylib/Interfaz.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace refugio::adaptadores {
namespace embebido {
// Generados por cmake/Embeber.cmake a partir de assets/fuentes/.
extern const unsigned char kInterMedium[];
extern const int kInterMediumTam;
extern const unsigned char kInterBold[];
extern const int kInterBoldTam;
}  // namespace embebido

namespace {
/// Inter tiene el ojo medio más grande que la fuente de raylib, pero su altura de línea incluye
/// más aire: con un 10 % más el texto ocupa lo mismo que antes y se lee mejor.
constexpr float kAjusteTam = 1.1f;
constexpr int kPxMinimo = 6;
constexpr int kPxMaximo = 256;
constexpr std::size_t kMaxFuentes = 48;  ///< Tamaños rasterizados a la vez (al redimensionar cambian).

struct Entrada {
    Peso peso{Peso::Normal};
    int px{0};
    Font fuente{};
    std::uint64_t usado{0};  ///< Último frame en que se dibujó con ella.
};

struct Estado {
    bool cargadas{false};
    std::vector<int> codigos;  ///< Caracteres que se rasterizan.
    std::vector<Entrada> cache;
    float escalaUi{1.0f};
    float escalaTexto{1.0f};
    std::uint64_t frame{1};
};

Estado& estado() {
    static Estado e;
    return e;
}

const Font& fuente(Peso peso, int px) {
    auto& e = estado();
    if (!e.cargadas) {
        static Font defecto{};
        defecto = GetFontDefault();
        return defecto;
    }
    for (auto& c : e.cache)
        if (c.peso == peso && c.px == px) {
            c.usado = e.frame;
            return c.fuente;
        }
    // Al redimensionar aparecen tamaños nuevos: se descarta el menos usado que no se haya
    // dibujado en este frame (su textura aún puede estar en el lote pendiente de raylib).
    if (e.cache.size() >= kMaxFuentes) {
        auto viejo = e.cache.end();
        for (auto it = e.cache.begin(); it != e.cache.end(); ++it)
            if (it->usado < e.frame && (viejo == e.cache.end() || it->usado < viejo->usado)) viejo = it;
        if (viejo != e.cache.end()) {
            UnloadFont(viejo->fuente);
            e.cache.erase(viejo);
        }
    }
    const bool negrita = peso == Peso::Negrita;
    Entrada nueva;
    nueva.peso = peso;
    nueva.px = px;
    nueva.usado = e.frame;
    nueva.fuente = LoadFontFromMemory(".ttf", negrita ? embebido::kInterBold : embebido::kInterMedium,
                                      negrita ? embebido::kInterBoldTam : embebido::kInterMediumTam, px,
                                      e.codigos.data(), static_cast<int>(e.codigos.size()));
    if (nueva.fuente.texture.id == 0) nueva.fuente = GetFontDefault();
    else SetTextureFilter(nueva.fuente.texture, TEXTURE_FILTER_BILINEAR);
    e.cache.push_back(nueva);
    return e.cache.back().fuente;
}

/// Píxeles de pantalla que ocupa la línea de @p tam unidades con la escala actual.
int pixeles(float tam) {
    const float px = tam * kAjusteTam * estado().escalaTexto;
    return std::clamp(static_cast<int>(std::lround(px)), kPxMinimo, kPxMaximo);
}

/// Ajusta una coordenada en unidades al píxel de pantalla más cercano: el glifo rasterizado cae
/// justo sobre la rejilla de píxeles y no se emborrona.
float alPixel(float v) {
    const float s = estado().escalaTexto;
    return std::round(v * s) / s;
}
}  // namespace

void cargarFuentes() {
    auto& e = estado();
    if (e.cargadas || !IsWindowReady()) return;
    e.codigos.clear();
    for (int c = 0x20; c <= 0x7E; ++c) e.codigos.push_back(c);
    for (int c = 0xA0; c <= 0xFF; ++c) e.codigos.push_back(c);
    for (const int c : {0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x2190, 0x2191, 0x2192, 0x2193,
                        0x2212, 0x2264, 0x2265})
        e.codigos.push_back(c);
    e.cargadas = true;
}

void liberarFuentes() {
    auto& e = estado();
    for (auto& c : e.cache)
        if (c.fuente.texture.id != GetFontDefault().texture.id) UnloadFont(c.fuente);
    e.cache.clear();
    e.cargadas = false;
}

void fijarEscalaInterfaz(float escala) noexcept {
    auto& e = estado();
    e.escalaUi = escala > 0.0f ? escala : 1.0f;
    e.escalaTexto = e.escalaUi;
    ++e.frame;
}

float escalaInterfaz() noexcept { return estado().escalaUi; }

Vector2 ratonInterfaz() {
    const Vector2 r = GetMousePosition();
    const float s = estado().escalaUi;
    return {r.x / s, r.y / s};
}

AmbitoEscalaTexto::AmbitoEscalaTexto(float escala) noexcept : previa_(estado().escalaTexto) {
    estado().escalaTexto = escala > 0.0f ? escala : 1.0f;
}

AmbitoEscalaTexto::~AmbitoEscalaTexto() { estado().escalaTexto = previa_; }

void dibujarTexto(const char* texto, Vector2 pos, float tam, Color color, Peso peso) {
    if (texto == nullptr || texto[0] == '\0') return;
    const Font& f = fuente(peso, pixeles(tam));
    DrawTextEx(f, texto, {alPixel(pos.x), alPixel(pos.y)}, tam * kAjusteTam, 0.0f, color);
}

void dibujarTexto(const char* texto, int x, int y, int tam, Color color, Peso peso) {
    dibujarTexto(texto, {static_cast<float>(x), static_cast<float>(y)}, static_cast<float>(tam), color, peso);
}

float medirTextoF(const char* texto, float tam, Peso peso) {
    if (texto == nullptr || texto[0] == '\0') return 0.0f;
    const Font& f = fuente(peso, pixeles(tam));
    return MeasureTextEx(f, texto, tam * kAjusteTam, 0.0f).x;
}

int medirTexto(const char* texto, int tam, Peso peso) {
    return static_cast<int>(std::lround(medirTextoF(texto, static_cast<float>(tam), peso)));
}

}  // namespace refugio::adaptadores
