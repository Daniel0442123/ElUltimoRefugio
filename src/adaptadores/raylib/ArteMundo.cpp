#include "adaptadores/raylib/ArteMundo.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <string_view>

namespace refugio::adaptadores {
namespace {
using agentes::EstadoInfectado;
using vista::Suelo;

constexpr int kPx = 16;  ///< Píxeles de arte por celda (se dibujan al doble).

// ------------------------------------------------------------------------- sprites
// Cada sprite es una rejilla de 16 x 16 caracteres; cada carácter es un color de la paleta y
// '.' es transparente. Los sprites miran al este y se giran con la orientación del agente.
using Sprite = std::array<std::string_view, 16>;
struct Paleta {
    char clave;
    Color color;
};
constexpr Paleta kPaleta[] = {
    {'K', {16, 18, 16, 255}},   {'S', {128, 158, 96, 255}},  {'s', {90, 116, 70, 255}},   {'C', {100, 86, 124, 255}},
    {'c', {66, 56, 86, 255}},   {'H', {62, 44, 30, 255}},    {'h', {92, 64, 40, 255}},    {'B', {150, 22, 22, 255}},
    {'b', {100, 14, 14, 255}},  {'E', {220, 40, 30, 255}},   {'M', {214, 162, 120, 255}}, {'m', {160, 112, 84, 255}},
    {'J', {52, 104, 128, 255}}, {'j', {34, 70, 88, 255}},    {'R', {140, 70, 36, 255}},   {'Y', {255, 236, 150, 255}},
    {'G', {80, 80, 86, 255}},   {'W', {214, 214, 204, 255}}, {'w', {150, 150, 146, 255}}, {'O', {226, 168, 32, 255}},
    {'o', {146, 98, 12, 255}},  {'V', {52, 168, 92, 255}},   {'v', {30, 108, 60, 255}},   {'T', {40, 150, 140, 255}},
    {'t', {24, 98, 92, 255}},   {'P', {150, 104, 58, 255}},  {'p', {92, 60, 30, 255}},    {'L', {96, 106, 124, 255}},
    {'l', {60, 66, 80, 255}},   {'A', {130, 200, 255, 255}}, {'F', {255, 120, 40, 255}},  {'f', {200, 40, 20, 255}},
    {'N', {40, 40, 44, 255}},   {'n', {60, 58, 56, 255}},    {'X', {120, 24, 24, 255}},   {'x', {80, 12, 12, 255}},
};
constexpr Sprite kSprZombiA = {
    "................", "....KKKK........", "...KCCCCKKKKKKK.", "...KCcCSSSSSSSSK",
    "...KCCCKKKKKKKK.", "..KKCbCKKK......", "..KCKKHHSSK.....", "..KCKHHSSEK.....",
    "..KCKHSSSsK.....", "..KCKHHSSEK.....", "..KCKKHHSSK.....", "..KKCCCKKK......",
    "...KCcCKKKKKKK..", "...KCBCSSSSSSSK.", "...KCCCCKKKKKK..", "....KKKK........",
};
constexpr Sprite kSprZombiB = {
    "................", "....KKKK........", "...KCCCCKKKKK...", "...KCcCSSSSSK...",
    "...KCCCKKKKK....", "..KKCbCKKK......", "..KCKKHHSSK.....", "..KCKHHSSEK.....",
    "..KCKHSSSsK.....", "..KCKHHSSEK.....", "..KCKKHHSSK.....", "..KKCCCKKK......",
    "...KCcCKKKKKKKK.", "...KCBCSSSSSSSSK", "...KCCCCKKKKKKK.", "....KKKK........",
};
constexpr Sprite kSprMaraA = {
    "................", "................", "....KKKK........", "...KJJJJKK......",
    "...KJjJJMMKK....", "...KJJKKKMMMK...", "..KKKHHHKKMMKKK.", ".KRKHHHHMKKGGYYK",
    ".KRKHHHHMKKGGYYK", "..KKKHHHKKMMKKK.", "...KJJKKKMMMK...", "...KJjJJMMKK....",
    "...KJJJJKK......", "....KKKK........", "................", "................",
};
constexpr Sprite kSprMaraB = {
    "................", "................", "...KKKK.........", "...KJJJJKK......",
    "...KJjJJMMKK....", "...KJJKKKMMMK...", "..KKKHHHKKMMKKK.", ".KRKHHHHMKKGGYYK",
    ".KRKHHHHMKKGGYYK", "..KKKHHHKKMMKKK.", "...KJJKKKMMMK...", "...KJjJJMMKK....",
    "...KJJJJKK......", ".....KKKK.......", "................", "................",
};
constexpr Sprite kSprBotiquin = {
    "................", "................", "...KKKKKKKKKK...", "..KWWWWWWWWWWK..",
    "..KWWWWBBWWWWK..", "..KWWWWBBWWWWK..", "..KWWBBBBBBWWK..", "..KWWBBBBBBWWK..",
    "..KWWWWBBWWWWK..", "..KWWWWBBWWWWK..", "..KwwwwwwwwwwK..", "...KKKKKKKKKK...",
    "................", "................", "................", "................",
};
constexpr Sprite kSprBidon = {
    "................", ".....KKKK.......", "....KOOOK.......", "...KKKKKKKKKK...",
    "...KOOOOOOOOK...", "...KOooOOooOK...", "...KOOoOOoOOK...", "...KOOOooOOOK...",
    "...KOOoOOoOOK...", "...KOooOOooOK...", "...KOOOOOOOOK...", "...KooooooooK...",
    "...KKKKKKKKKK...", "................", "................", "................",
};
constexpr Sprite kSprBateria = {
    "................", "................", "................", "....KKK..KKK....",
    "....KGK..KGK....", "..KKKKKKKKKKKK..", "..KVVVVVVVVVVK..", "..KVVWVVVVwVVK..",
    "..KVWWWVVwwwVK..", "..KVVWVVVVVVVK..", "..KvvvvvvvvvvK..", "..KKKKKKKKKKKK..",
    "................", "................", "................", "................",
};
constexpr Sprite kSprRadio = {
    "...........K....", "...........wK...", "............K...", "..KKKKKKKKKKKK..",
    "..KLLLLLLLLLLK..", "..KLKKKKKLnnLK..", "..KLKAAAKLnnLK..", "..KLKAAAKLLLLK..",
    "..KLKKKKKLFLfK..", "..KLLLLLLLLLLK..", "..KllllllllllK..", "..KKKKKKKKKKKK..",
    "................", "................", "................", "................",
};
constexpr Sprite kSprAlmacen = {
    "................", ".KKKKKKKKKK.....", ".KPPPPPPPPK.....", ".KpPpPpPpPKKKKK.",
    ".KPPPPPPPPKPPPK.", ".KKKKKKKKKKpPpK.", ".KPPPPPPPPKPPPK.", ".KpPpPpPpPKKKKK.",
    ".KPPPPPPPPKPPPK.", ".KKKKKKKKKKpPpK.", "..KPPPPPPPPPPPK.", "..KpPpPpPpPpPpK.",
    "..KPPPPPPPPPPPK.", "..KKKKKKKKKKKKK.", "................", "................",
};
constexpr Sprite kSprBarricada = {
    "KKKKKKKKKKKKKKKK", "KPPPPPPPPPPPPPPK", "KppppppppppppppK", "KKKKKKKKKKKKKKKK",
    "..KPK......KPK..", "KKKKKKKKKKKKKKKK", "KPPPPPPPPPPPPPPK", "KppppppppppppppK",
    "KKKKKKKKKKKKKKKK", "..KPK......KPK..", "KKKKKKKKKKKKKKKK", "KPPPPPPPPPPPPPPK",
    "KppppppppppppppK", "KKKKKKKKKKKKKKKK", "..KPK......KPK..", "................",
};
constexpr Sprite kSprSenuelo = {
    "................", "................", "................", "................",
    "......KKKK......", ".....KfFFfK.....", "....KfFYYFfK....", "....KFYYYYFK....",
    "....KFYYYYFK....", "....KfFYYFfK....", ".....KfFFfK.....", "......KKKK......",
    "................", "................", "................", "................",
};

/// Índices en el atlas.
enum Indice : int {
    kZombi = 0,  // 4 variantes x 2 fotogramas: kZombi + 2 * variante + fotograma
    kMara = 8,
    kSuperviviente = 10,
    kBotiquin = 12,
    kBidon,
    kBateria,
    kRadio,
    kAlmacen,
    kBarricada,
    kSenuelo,
    kSangre,  // 3 variantes
    kNumSprites = kSangre + 3,
};
constexpr int kVariantesZombi = 4;

/// Cambios de paleta: camisa (C, c) y piel (S, s) de cada variante de infectado.
struct Variante {
    Color camisa, camisaSombra, piel, pielSombra;
};
constexpr Variante kVariantes[kVariantesZombi] = {
    {{100, 86, 124, 255}, {66, 56, 86, 255}, {128, 158, 96, 255}, {90, 116, 70, 255}},
    {{84, 104, 122, 255}, {56, 70, 84, 255}, {142, 152, 118, 255}, {100, 110, 84, 255}},
    {{132, 72, 60, 255}, {90, 46, 40, 255}, {118, 150, 90, 255}, {84, 108, 64, 255}},
    {{104, 106, 72, 255}, {70, 72, 46, 255}, {150, 160, 128, 255}, {108, 118, 92, 255}},
};

Color colorDe(char c) {
    for (const auto& p : kPaleta)
        if (p.clave == c) return p.color;
    return MAGENTA;
}

/// Copia @p s en la casilla @p indice del atlas, con los reemplazos de color de @p cambios.
template <typename F>
void pintarSprite(Image& atlas, int indice, const Sprite& s, F&& cambio) {
    for (int y = 0; y < kPx; ++y)
        for (int x = 0; x < kPx; ++x) {
            const char c = s[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            if (c == '.') continue;
            ImageDrawPixel(&atlas, indice * kPx + x, y, cambio(c));
        }
}

/// Generador pequeño para la decoración (no es el de la simulación).
struct Azar {
    std::uint32_t estado;
    std::uint32_t siguiente() noexcept {
        estado ^= estado << 13;
        estado ^= estado >> 17;
        estado ^= estado << 5;
        return estado;
    }
    int entre(int a, int b) noexcept {
        return a + static_cast<int>(siguiente() % static_cast<std::uint32_t>(b - a + 1));
    }
    bool prob(int porcentaje) noexcept { return static_cast<int>(siguiente() % 100u) < porcentaje; }
};

void pintarSangre(Image& atlas, int indice, std::uint32_t semilla) {
    Azar az{semilla | 1u};
    const Color tonos[3] = {{112, 14, 16, 230}, {84, 8, 10, 230}, {140, 22, 22, 230}};
    for (int k = 0; k < 70; ++k) {
        // Gotas concentradas en el centro y salpicaduras hacia fuera.
        const float r = static_cast<float>(az.entre(0, 100)) / 100.0f;
        const float a = static_cast<float>(az.entre(0, 628)) / 100.0f;
        const int x = 8 + static_cast<int>(std::cos(a) * r * r * 7.5f);
        const int y = 8 + static_cast<int>(std::sin(a) * r * r * 7.5f);
        const int t = r < 0.5f ? az.entre(1, 2) : 1;
        ImageDrawRectangle(&atlas, indice * kPx + std::clamp(x, 0, 15), std::clamp(y, 0, 15), t, t,
                           tonos[az.entre(0, 2)]);
    }
}

Color variar(Color c, Azar& az, int n) {
    const auto v = [&](unsigned char x) {
        return static_cast<unsigned char>(std::clamp(static_cast<int>(x) + az.entre(-n, n), 0, 255));
    };
    return {v(c.r), v(c.g), v(c.b), 255};
}
Color escalar(Color c, float f) {
    const auto v = [f](unsigned char x) {
        return static_cast<unsigned char>(std::clamp(static_cast<float>(x) * f, 0.0f, 255.0f));
    };
    return {v(c.r), v(c.g), v(c.b), c.a};
}

/// Pinta un píxel del fondo (en píxeles de arte).
struct Lienzo {
    std::vector<Color>& px;
    int ancho;
    void poner(int x, int y, Color c) const {
        if (x < 0 || y < 0 || x >= ancho || static_cast<std::size_t>(y) * static_cast<std::size_t>(ancho) >= px.size())
            return;
        px[static_cast<std::size_t>(y) * static_cast<std::size_t>(ancho) + static_cast<std::size_t>(x)] = c;
    }
    [[nodiscard]] Color leer(int x, int y) const {
        return px[static_cast<std::size_t>(y) * static_cast<std::size_t>(ancho) + static_cast<std::size_t>(x)];
    }
    void caja(int x0, int y0, int w, int h, Color relleno, Color borde) const {
        for (int y = y0; y < y0 + h; ++y)
            for (int x = x0; x < x0 + w; ++x)
                poner(x, y, (x == x0 || y == y0 || x == x0 + w - 1 || y == y0 + h - 1) ? borde : relleno);
    }
};

constexpr Color kTechos[] = {
    {62, 58, 60, 255}, {80, 60, 52, 255}, {56, 62, 72, 255}, {66, 68, 56, 255}, {74, 70, 66, 255}};

float grados(navegacion::Vec2 o) {
    return std::atan2(o.y, o.x) * RAD2DEG;
}

Vector2 aPantalla(navegacion::Vec2 p, const EstiloVista& e) {
    return {static_cast<float>(e.origenX) + p.x * static_cast<float>(e.tamCelda),
            static_cast<float>(e.origenY) + p.y * static_cast<float>(e.tamCelda)};
}
Vector2 centroCelda(mundo::Celda c, const EstiloVista& e) {
    return aPantalla({static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.5f}, e);
}

/// Luz con sombras: un abanico de rayos que se detienen en los muros. El color baja de forma
/// lineal desde el foco hasta el alcance; un cono (@p semiApertura < PI) se difumina en sus bordes.
void abanico(const vista::Escenario& esc, float ox, float oy, float radio, Color c, float centro, float semiApertura,
             int rayos) {
    const float escala = static_cast<float>(kPx);
    const bool cono = semiApertura < PI - 0.01f;
    rlCheckRenderBatchLimit(rayos * 3 + 3);
    rlBegin(RL_TRIANGLES);
    float px0 = 0, py0 = 0;
    Color c0{};
    for (int i = 0; i <= rayos; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(rayos);
        const float a = centro - semiApertura + 2.0f * semiApertura * t;
        const float dx = std::cos(a);
        const float dy = std::sin(a);
        float d = vista::distanciaRayo(esc, ox, oy, dx, dy, radio);
        if (d < radio) d = std::fmin(radio, d + 0.35f);  // ilumina el borde del muro que toca
        float f = 1.0f - d / radio;
        if (cono) f *= std::clamp((1.0f - std::fabs(2.0f * t - 1.0f)) / 0.3f, 0.0f, 1.0f);
        const Color ci = escalar(c, f);
        const float px = (ox + dx * d) * escala;
        const float py = (oy + dy * d) * escala;
        if (i > 0) {
            rlColor4ub(c.r, c.g, c.b, 255);
            rlVertex2f(ox * escala, oy * escala);
            rlColor4ub(c0.r, c0.g, c0.b, 255);
            rlVertex2f(px0, py0);
            rlColor4ub(ci.r, ci.g, ci.b, 255);
            rlVertex2f(px, py);
        }
        px0 = px;
        py0 = py;
        c0 = ci;
    }
    rlEnd();
}

}  // namespace

ArteMundo::ArteMundo() {
    Image atlas = GenImageColor(kNumSprites * kPx, kPx, BLANK);
    for (int v = 0; v < kVariantesZombi; ++v) {
        const auto& var = kVariantes[v];
        const auto cambio = [&var](char c) {
            switch (c) {
                case 'C':
                    return var.camisa;
                case 'c':
                    return var.camisaSombra;
                case 'S':
                    return var.piel;
                case 's':
                    return var.pielSombra;
                default:
                    return colorDe(c);
            }
        };
        pintarSprite(atlas, kZombi + 2 * v, kSprZombiA, cambio);
        pintarSprite(atlas, kZombi + 2 * v + 1, kSprZombiB, cambio);
    }
    const auto igual = [](char c) { return colorDe(c); };
    pintarSprite(atlas, kMara, kSprMaraA, igual);
    pintarSprite(atlas, kMara + 1, kSprMaraB, igual);
    // El superviviente: chaqueta verde azulada, pelo oscuro y una herramienta en vez de linterna.
    const auto superv = [](char c) {
        switch (c) {
            case 'J':
                return colorDe('T');
            case 'j':
                return colorDe('t');
            case 'R':
                return colorDe('H');
            case 'Y':
                return colorDe('w');
            default:
                return colorDe(c);
        }
    };
    pintarSprite(atlas, kSuperviviente, kSprMaraA, superv);
    pintarSprite(atlas, kSuperviviente + 1, kSprMaraB, superv);
    pintarSprite(atlas, kBotiquin, kSprBotiquin, igual);
    pintarSprite(atlas, kBidon, kSprBidon, igual);
    pintarSprite(atlas, kBateria, kSprBateria, igual);
    pintarSprite(atlas, kRadio, kSprRadio, igual);
    pintarSprite(atlas, kAlmacen, kSprAlmacen, igual);
    pintarSprite(atlas, kBarricada, kSprBarricada, igual);
    pintarSprite(atlas, kSenuelo, kSprSenuelo, igual);
    for (int k = 0; k < 3; ++k) pintarSangre(atlas, kSangre + k, 0x5A9u * static_cast<std::uint32_t>(k + 3));
    atlas_ = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    SetTextureFilter(atlas_, TEXTURE_FILTER_POINT);
}

ArteMundo::~ArteMundo() {
    if (atlas_.id != 0) UnloadTexture(atlas_);
    if (fondo_.id != 0) UnloadTexture(fondo_);
    if (luz_.id != 0) UnloadRenderTexture(luz_);
}

void ArteMundo::rehacerFondo(const juego::Partida& p) {
    escenario_ = vista::construirEscenario(p);
    firma_ = escenario_.firma;
    semilla_ = p.semilla();
    const auto& esc = escenario_;
    const int W = esc.ancho;
    const int H = esc.alto;
    std::vector<Color> px(static_cast<std::size_t>(W * kPx) * static_cast<std::size_t>(H * kPx), BLACK);
    const Lienzo l{px, W * kPx};
    ventanas_.clear();

    for (int cy = 0; cy < H; ++cy)
        for (int cx = 0; cx < W; ++cx) {
            Azar az{vista::hashCelda(semilla_, cx, cy) | 1u};
            const int x0 = cx * kPx;
            const int y0 = cy * kPx;
            const Suelo s = esc.en(cx, cy);
            if (s == Suelo::Muro) {
                // Azotea con grava y pretil; fachada de ladrillo si da a la calle por el sur.
                const auto id = esc.edificio[static_cast<std::size_t>(cy * W + cx)];
                const Color techo = kTechos[vista::hashCelda(semilla_, id, 0, 7) % std::size(kTechos)];
                const Color pretil = escalar(techo, 1.45f);
                const bool fachada = esc.dentro(cx, cy + 1) && !esc.muro(cx, cy + 1);
                for (int j = 0; j < kPx; ++j)
                    for (int i = 0; i < kPx; ++i) {
                        const bool borde = (i <= 1 && !esc.muro(cx - 1, cy)) ||
                                           (i >= kPx - 2 && !esc.muro(cx + 1, cy)) || (j <= 1 && !esc.muro(cx, cy - 1));
                        Color c = borde ? variar(pretil, az, 4) : variar(techo, az, 5);
                        if (!borde && az.prob(4)) c = escalar(techo, 0.75f);  // grava
                        if (fachada && j >= kPx - 6) {
                            const int f = j - (kPx - 6);  // 0 = cornisa, luego dos hiladas de ladrillo
                            const bool junta = f % 3 == 0 || (i + ((f / 3) % 2) * 3) % 6 == 0;
                            c = junta ? Color{72, 38, 32, 255} : variar({128, 64, 50, 255}, az, 7);
                        }
                        l.poner(x0 + i, y0 + j, c);
                    }
                if (fachada && az.prob(55)) {  // ventana, a veces con luz dentro
                    const int vx = x0 + az.entre(3, 8);
                    const bool encendida = az.prob(22);
                    l.caja(vx, y0 + kPx - 5, 5, 4, encendida ? Color{230, 190, 100, 255} : Color{30, 38, 58, 255},
                           Color{40, 26, 22, 255});
                    if (encendida)
                        ventanas_.push_back({static_cast<float>(vx + 1) / kPx, static_cast<float>(y0 + kPx - 4) / kPx});
                }
                const bool interior =
                    esc.muro(cx - 1, cy) && esc.muro(cx + 1, cy) && esc.muro(cx, cy - 1) && esc.muro(cx, cy + 1);
                if (interior && az.prob(18)) {  // aire acondicionado
                    const int ax = x0 + az.entre(2, 7);
                    const int ay = y0 + az.entre(2, 6);
                    l.caja(ax, ay, 7, 6, {130, 132, 136, 255}, {40, 40, 44, 255});
                    for (int k = 1; k < 5; k += 2) l.caja(ax + 1, ay + k, 5, 1, {80, 82, 86, 255}, {80, 82, 86, 255});
                } else if (interior && az.prob(12)) {  // claraboya
                    l.caja(x0 + 4, y0 + 4, 8, 8, {34, 44, 60, 255}, {120, 120, 116, 255});
                }
                continue;
            }
            // Suelo transitable.
            for (int j = 0; j < kPx; ++j)
                for (int i = 0; i < kPx; ++i) {
                    Color c;
                    if (s == Suelo::Acera) {
                        const bool junta = (i % 8 == 0) || (j % 8 == 0);
                        c = junta ? Color{58, 58, 62, 255} : variar({76, 77, 81, 255}, az, 3);
                    } else if (s == Suelo::Escombros) {
                        c = variar({76, 70, 64, 255}, az, 6);
                    } else {
                        c = variar({56, 58, 63, 255}, az, 5);
                    }
                    l.poner(x0 + i, y0 + j, c);
                }
            if (s == Suelo::Escombros) {  // piedras y tablas: terreno lento
                const Color piedras[3] = {{116, 106, 96, 255}, {88, 80, 74, 255}, {136, 124, 110, 255}};
                for (int k = 0; k < 5; ++k) {  // piedras con luz arriba a la izquierda y sombra abajo
                    const int w = az.entre(2, 4);
                    const int h = az.entre(2, 3);
                    const int sx = x0 + az.entre(1, 11);
                    const int sy = y0 + az.entre(1, 11);
                    const Color base = piedras[az.entre(0, 2)];
                    for (int j = 0; j <= h; ++j)
                        for (int i = 0; i <= w; ++i) {
                            if ((i == 0 || i == w) && (j == 0 || j == h)) continue;  // esquinas redondeadas
                            const Color c = (j == h || i == w)   ? escalar(base, 0.55f)
                                            : (j == 0 || i == 0) ? escalar(base, 1.2f)
                                                                 : base;
                            l.poner(sx + i, sy + j, c);
                        }
                }
                for (int k = 0; k < 1; ++k)
                    l.caja(x0 + az.entre(0, 6), y0 + az.entre(1, 13), 9, 2, {124, 82, 46, 255}, {54, 34, 18, 255});
            } else if (s == Suelo::Calle) {
                const int r = az.entre(0, 99);
                if (r < 2) {  // alcantarilla
                    for (int j = -5; j <= 5; ++j)
                        for (int i = -5; i <= 5; ++i) {
                            const int d2 = i * i + j * j;
                            if (d2 > 26) continue;
                            l.poner(x0 + 8 + i, y0 + 8 + j,
                                    d2 > 18 ? Color{26, 26, 26, 255}
                                            : (j % 2 ? Color{34, 34, 36, 255} : Color{52, 50, 48, 255}));
                        }
                } else if (r < 6) {  // mancha de aceite
                    for (int k = 0; k < 30; ++k)
                        l.poner(x0 + 8 + az.entre(-4, 4), y0 + 8 + az.entre(-3, 3), {40, 42, 46, 255});
                }
            } else if (az.prob(10)) {  // bolsas de basura en la acera
                for (int k = 0; k < 2; ++k) {
                    const int bx = x0 + az.entre(2, 10);
                    const int by = y0 + az.entre(2, 10);
                    l.caja(bx, by, 5, 4, {30, 32, 34, 255}, {14, 14, 16, 255});
                    l.poner(bx + 1, by + 1, {60, 62, 66, 255});
                }
            }
            if (s != Suelo::Escombros && az.prob(22)) {  // grietas
                int gx = az.entre(0, 15);
                int gy = az.entre(0, 15);
                for (int k = az.entre(4, 9); k > 0; --k) {
                    l.poner(x0 + gx, y0 + gy, {34, 34, 38, 255});
                    gx = std::clamp(gx + az.entre(-1, 1), 0, 15);
                    gy = std::clamp(gy + az.entre(0, 1), 0, 15);
                }
            }
            if (az.prob(4)) {  // papeles
                for (int k = 0; k < 3; ++k)
                    l.caja(x0 + az.entre(1, 13), y0 + az.entre(1, 13), 2, 2, {170, 168, 156, 255},
                           {130, 128, 120, 255});
            }
            if (az.prob(3)) {  // sangre seca
                for (int k = 0; k < 18; ++k) {
                    const int bx = x0 + 8 + az.entre(-5, 5);
                    const int by = y0 + 8 + az.entre(-4, 4);
                    l.poner(bx, by, az.prob(50) ? Color{86, 16, 18, 255} : Color{66, 12, 14, 255});
                }
            }
        }
    // Sombras de los edificios hacia el sureste (la luz de la luna viene del noroeste).
    for (int cy = 0; cy < H; ++cy)
        for (int cx = 0; cx < W; ++cx) {
            if (esc.muro(cx, cy)) continue;
            const int x0 = cx * kPx;
            const int y0 = cy * kPx;
            if (esc.muro(cx, cy - 1))
                for (int j = 0; j < 5; ++j)
                    for (int i = 0; i < kPx; ++i)
                        l.poner(x0 + i, y0 + j, escalar(l.leer(x0 + i, y0 + j), 0.45f + 0.11f * static_cast<float>(j)));
            if (esc.muro(cx - 1, cy))
                for (int j = 0; j < kPx; ++j)
                    for (int i = 0; i < 3; ++i)
                        l.poner(x0 + i, y0 + j, escalar(l.leer(x0 + i, y0 + j), 0.6f + 0.13f * static_cast<float>(i)));
        }

    Image img{px.data(), W * kPx, H * kPx, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    if (fondo_.id != 0) UnloadTexture(fondo_);
    fondo_ = LoadTextureFromImage(img);
    SetTextureFilter(fondo_, TEXTURE_FILTER_POINT);
    if (luz_.id == 0 || luz_.texture.width != W * kPx || luz_.texture.height != H * kPx) {
        if (luz_.id != 0) UnloadRenderTexture(luz_);
        luz_ = LoadRenderTexture(W * kPx, H * kPx);
        SetTextureFilter(luz_.texture, TEXTURE_FILTER_BILINEAR);
    }
    hayFondo_ = true;
    numManchas_ = siguienteMancha_ = 0;
    previas_.clear();
    recorrido_.clear();
}

void ArteMundo::preparar(const juego::Partida& p, bool iluminacion) {
    if (!hayFondo_ || p.semilla() != semilla_ || vista::firmaEscenario(p) != firma_) rehacerFondo(p);

    // Animación de paso: distancia recorrida por cada agente desde que se ve este mapa.
    const std::size_t n = p.infectados().size() + 2;
    if (previas_.size() != n || p.tick() < ultimoTick_) {
        previas_.assign(n, {});
        recorrido_.assign(n, 0.0f);
        for (std::size_t i = 0; i + 2 < n; ++i) previas_[i] = p.infectados()[i].pos;
        previas_[n - 2] = p.mara().pos;
        previas_[n - 1] = p.superviviente().pos;
        numManchas_ = siguienteMancha_ = 0;
    }
    const auto avanzar = [this](std::size_t i, navegacion::Vec2 pos) {
        const float dx = pos.x - previas_[i].x;
        const float dy = pos.y - previas_[i].y;
        const float d = std::sqrt(dx * dx + dy * dy);
        if (d < 2.0f) recorrido_[i] += d;  // un salto grande es una carga o un reinicio
        previas_[i] = pos;
    };
    for (std::size_t i = 0; i + 2 < n; ++i) avanzar(i, p.infectados()[i].pos);
    avanzar(n - 2, p.mara().pos);
    avanzar(n - 1, p.superviviente().pos);

    // Sangre en el suelo cuando hieren a Mara (como mucho una mancha cada 20 ticks).
    if (p.mara().recibioDanio && p.tick() >= tickUltimaMancha_ + 20) {
        tickUltimaMancha_ = p.tick();
        const auto h = vista::hashCelda(semilla_, static_cast<int>(p.tick()), 0, 11);
        auto& m = manchas_[siguienteMancha_];
        m.x = p.mara().pos.x + static_cast<float>(static_cast<int>(h % 41u) - 20) / 60.0f;
        m.y = p.mara().pos.y + static_cast<float>(static_cast<int>((h >> 8) % 41u) - 20) / 60.0f;
        m.angulo = static_cast<float>((h >> 16) % 360u);
        m.variante = static_cast<std::uint8_t>((h >> 4) % 3u);
        siguienteMancha_ = (siguienteMancha_ + 1) % manchas_.size();
        numManchas_ = std::min(numManchas_ + 1, manchas_.size());
    }
    ultimoTick_ = p.tick();

    conLuz_ = iluminacion;
    if (iluminacion) pintarLuz(p);
}

void ArteMundo::pintarLuz(const juego::Partida& p) {
    const float t = static_cast<float>(GetTime());
    BeginTextureMode(luz_);
    ClearBackground({92, 102, 146, 255});  // luna: ambiente azulado
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableBackfaceCulling();
    const auto& esc = escenario_;
    const auto& m = p.mara();
    // Linterna de Mara: un cono que los muros cortan, con un parpadeo leve, y un halo cercano.
    const float parpadeo = 0.95f + 0.05f * std::sin(t * 23.0f) * std::sin(t * 7.0f);
    const float alcance = m.modo == juego::ModoMovimiento::Sigilo ? 7.0f : 9.5f;
    abanico(esc, m.pos.x, m.pos.y, alcance, escalar({255, 232, 176, 255}, parpadeo),
            std::atan2(m.orientacion.y, m.orientacion.x), 0.55f, 72);
    abanico(esc, m.pos.x, m.pos.y, 2.8f, {120, 108, 92, 255}, 0.0f, PI, 48);
    // Radio: luz fría que late cuando ya se puede activar.
    const float pulso = p.tieneTodosLosObjetos() ? 0.75f + 0.25f * std::sin(t * 4.0f) : 0.55f;
    abanico(esc, static_cast<float>(p.radio().x) + 0.5f, static_cast<float>(p.radio().y) + 0.5f, 4.0f,
            escalar({70, 140, 230, 255}, pulso), 0.0f, PI, 48);
    if (const auto& s = p.superviviente(); s.activo)  // farol del superviviente
        abanico(esc, s.pos.x, s.pos.y, 2.6f, {150, 110, 60, 255}, 0.0f, PI, 40);
    if (p.zonaAbierta())
        abanico(esc, static_cast<float>(p.extraccion().x) + 0.5f, static_cast<float>(p.extraccion().y) + 0.5f,
                p.config().radioExtraccion + 1.5f, {50, 170, 70, 255}, 0.0f, PI, 48);
    for (const auto& sn : p.senuelos())  // bengala: luz roja que vibra
        if (sn.activo)
            abanico(esc, static_cast<float>(sn.celda.x) + 0.5f, static_cast<float>(sn.celda.y) + 0.5f,
                    3.2f + 0.3f * std::sin(t * 17.0f), {210, 90, 40, 255}, 0.0f, PI, 40);
    for (const auto& v : ventanas_)  // las ventanas encendidas iluminan un poco la acera
        abanico(esc, v.x + 0.15f, v.y + 0.6f, 1.6f, {70, 56, 28, 255}, PI / 2.0f, PI / 2.0f, 16);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    EndBlendMode();
    EndTextureMode();
}

void ArteMundo::sprite(int indice, Vector2 centro, float lado, float grados_, Color tinte) const {
    DrawTexturePro(atlas_, {static_cast<float>(indice * kPx), 0.0f, static_cast<float>(kPx), static_cast<float>(kPx)},
                   {centro.x, centro.y, lado, lado}, {lado / 2.0f, lado / 2.0f}, grados_, tinte);
}

void ArteMundo::dibujarFondo(const EstiloVista& e) const {
    const float tc = static_cast<float>(e.tamCelda);
    DrawTexturePro(fondo_, {0, 0, static_cast<float>(fondo_.width), static_cast<float>(fondo_.height)},
                   {static_cast<float>(e.origenX), static_cast<float>(e.origenY),
                    static_cast<float>(escenario_.ancho) * tc, static_cast<float>(escenario_.alto) * tc},
                   {0, 0}, 0.0f, WHITE);
    for (std::size_t i = 0; i < numManchas_; ++i) {
        const auto& m = manchas_[i];
        sprite(kSangre + m.variante, aPantalla({m.x, m.y}, e), tc * 1.1f, m.angulo);
    }
}

void ArteMundo::dibujarActores(const juego::Partida& p, const EstiloVista& e) const {
    const float tc = static_cast<float>(e.tamCelda);
    for (const auto& o : p.objetos()) {
        if (o.recogido) continue;
        const int ind = o.tipo == juego::TipoObjeto::Medicamento   ? kBotiquin
                        : o.tipo == juego::TipoObjeto::Combustible ? kBidon
                                                                   : kBateria;
        sprite(ind, centroCelda(o.celda, e), tc * 0.9f, 0.0f);
    }
    if (p.almacen().x >= 0) sprite(kAlmacen, centroCelda(p.almacen(), e), tc, 0.0f);
    sprite(kRadio, centroCelda(p.radio(), e), tc, 0.0f);
    for (const auto& b : p.barricadas())
        if (b.activa) sprite(kBarricada, centroCelda(b.celda, e), tc, 0.0f);
    for (const auto& sn : p.senuelos())
        if (sn.activo) sprite(kSenuelo, centroCelda(sn.celda, e), tc * 0.8f, 0.0f);

    const auto fotograma = [this](std::size_t i, float largoPaso) {
        return i < recorrido_.size() ? static_cast<int>(recorrido_[i] / largoPaso) % 2 : 0;
    };
    const auto& zs = p.infectados();
    for (std::size_t i = 0; i < zs.size(); ++i) {
        const auto& z = zs[i];
        const int v = static_cast<int>(i % kVariantesZombi);
        sprite(kZombi + 2 * v + fotograma(i, 0.3f), aPantalla(z.pos, e), tc * 1.05f, grados(z.orientacion));
    }
    if (const auto& s = p.superviviente(); s.activo)
        sprite(kSuperviviente + fotograma(zs.size() + 1, 0.3f), aPantalla(s.pos, e), tc, grados(s.orientacion));
    const auto& m = p.mara();
    const float lado = m.modo == juego::ModoMovimiento::Sigilo ? tc * 0.9f : tc;
    sprite(kMara + fotograma(zs.size(), m.modo == juego::ModoMovimiento::Correr ? 0.4f : 0.28f), aPantalla(m.pos, e),
           lado, grados(m.orientacion));
}

void ArteMundo::dibujarLuz(const EstiloVista& e) const {
    if (!conLuz_) return;
    const float tc = static_cast<float>(e.tamCelda);
    BeginBlendMode(BLEND_MULTIPLIED);
    DrawTexturePro(luz_.texture,
                   {0, 0, static_cast<float>(luz_.texture.width), -static_cast<float>(luz_.texture.height)},
                   {static_cast<float>(e.origenX), static_cast<float>(e.origenY),
                    static_cast<float>(escenario_.ancho) * tc, static_cast<float>(escenario_.alto) * tc},
                   {0, 0}, 0.0f, WHITE);
    EndBlendMode();
}

void ArteMundo::dibujarEmisivos(const juego::Partida& p, const EstiloVista& e) const {
    const float tc = static_cast<float>(e.tamCelda);
    const float px = tc / static_cast<float>(kPx);  // un píxel de arte en pantalla
    const float t = static_cast<float>(GetTime());
    if (conLuz_) {
        for (const auto& v : ventanas_) {
            const Vector2 c = aPantalla({v.x + 0.1f, v.y + 0.1f}, e);
            DrawCircleGradient(static_cast<int>(c.x), static_cast<int>(c.y), tc * 0.35f,
                               Fade({255, 200, 110, 255}, 0.35f), BLANK);
        }
        // Ojos rojos: dos puntos delante de la cabeza, visibles aunque el infectado esté a oscuras.
        for (const auto& z : p.infectados()) {
            const Vector2 c = aPantalla(z.pos, e);
            const float ox = z.orientacion.x;
            const float oy = z.orientacion.y;
            for (const float lado : {-1.0f, 1.0f}) {
                const float ex = c.x + ox * 2.2f * px - oy * lado * 1.1f * px;
                const float ey = c.y + oy * 2.2f * px + ox * lado * 1.1f * px;
                DrawRectangleRec({ex - px * 0.5f, ey - px * 0.5f, px, px}, {255, 60, 40, 210});
            }
        }
        const Vector2 r = centroCelda(p.radio(), e);
        DrawRectangleRec({r.x - 3.0f * px, r.y - 2.0f * px, 3.0f * px, 2.0f * px}, {150, 215, 255, 220});
    }
    // Destello sobre los objetos que faltan, para encontrarlos en la oscuridad.
    for (const auto& o : p.objetos()) {
        if (o.recogido) continue;
        const Vector2 c = centroCelda(o.celda, e);
        const float fase = std::sin(t * 3.0f + static_cast<float>(o.celda.x + o.celda.y));
        if (fase < 0.2f) continue;
        const float l = tc * 0.18f * fase;
        const Vector2 d{c.x + tc * 0.3f, c.y - tc * 0.3f};
        DrawLineEx({d.x - l, d.y}, {d.x + l, d.y}, 2.0f, Fade(RAYWHITE, fase));
        DrawLineEx({d.x, d.y - l}, {d.x, d.y + l}, 2.0f, Fade(RAYWHITE, fase));
    }
}

}  // namespace refugio::adaptadores
