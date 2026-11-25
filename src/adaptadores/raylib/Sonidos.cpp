#include "adaptadores/raylib/Sonidos.hpp"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace refugio::adaptadores {

namespace {
constexpr int kFrecuencia = 22050;
constexpr float kFrecuenciaF = 22050.0f;
constexpr float kPi = 3.14159265f;

struct Nota {
    float hz;
    float segundos;
};

/// Tono (o varios seguidos) con ataque y caída cortos para que no chasquee. @p ruido mezcla
/// ruido blanco (golpe de barricada).
Sound sintetizar(std::initializer_list<Nota> notas, float amplitud, float ruido = 0.0f) {
    std::vector<std::int16_t> m;
    std::uint32_t semilla = 12345u;
    for (const auto& n : notas) {
        const int total = static_cast<int>(n.segundos * kFrecuenciaF);
        for (int i = 0; i < total; ++i) {
            const float t = static_cast<float>(i) / kFrecuenciaF;
            const float env = std::fmin(1.0f, static_cast<float>(i) / 200.0f) *
                              std::fmin(1.0f, static_cast<float>(total - i) / (0.6f * static_cast<float>(total)));
            semilla = semilla * 1664525u + 1013904223u;
            const float blanco = static_cast<float>(semilla >> 9) / 8388608.0f * 2.0f - 1.0f;
            const float v = (1.0f - ruido) * std::sin(2.0f * kPi * n.hz * t) + ruido * blanco;
            m.push_back(static_cast<std::int16_t>(v * env * amplitud * 32767.0f));
        }
    }
    Wave w{};
    w.frameCount = static_cast<unsigned int>(m.size());
    w.sampleRate = kFrecuencia;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = m.data();
    return LoadSoundFromWave(w);  // copia las muestras: m puede liberarse
}
}  // namespace

Sonidos::Sonidos() {
    if (!IsAudioDeviceReady()) return;
    sonidos_[static_cast<int>(Efecto::Recoger)] = sintetizar({{880.0f, 0.07f}, {1320.0f, 0.10f}}, 0.35f);
    sonidos_[static_cast<int>(Efecto::Senuelo)] = sintetizar({{520.0f, 0.12f}, {390.0f, 0.12f}}, 0.35f);
    sonidos_[static_cast<int>(Efecto::Barricada)] = sintetizar({{110.0f, 0.18f}}, 0.5f, 0.6f);
    sonidos_[static_cast<int>(Efecto::Menu)] = sintetizar({{660.0f, 0.03f}}, 0.2f);
    sonidos_[static_cast<int>(Efecto::Guardado)] = sintetizar({{660.0f, 0.08f}, {880.0f, 0.08f}, {1100.0f, 0.12f}}, 0.3f);
    sonidos_[static_cast<int>(Efecto::Error)] = sintetizar({{180.0f, 0.25f}}, 0.35f, 0.2f);
    sonidos_[static_cast<int>(Efecto::Tutorial)] = sintetizar({{990.0f, 0.06f}, {1480.0f, 0.09f}}, 0.25f);
    listo_ = true;
}

Sonidos::~Sonidos() {
    if (!listo_) return;
    for (auto& s : sonidos_) UnloadSound(s);
}

void Sonidos::reproducir(Efecto e) const {
    if (listo_) PlaySound(sonidos_[static_cast<int>(e)]);
}

void Sonidos::volumen(float v) const {
    if (listo_) SetMasterVolume(v);
}

}  // namespace refugio::adaptadores
