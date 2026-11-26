#include "app/MedicionRendimiento.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace refugio::app {
namespace {

double numero(std::string_view opcion, const char* texto) {
    char* fin = nullptr;
    const double v = std::strtod(texto, &fin);
    if (fin == texto || *fin != '\0' || !std::isfinite(v) || v < 0.0)
        throw std::runtime_error(std::string(opcion) + " necesita un número no negativo");
    return v;
}

/// Texto JSON entre comillas (los metadatos traen comillas, p. ej. so="Ubuntu 24.04").
std::string cadena(std::string_view s) {
    std::string r = "\"";
    for (const char c : s) {
        if (c == '"' || c == '\\') r += '\\';
        if (static_cast<unsigned char>(c) < 0x20) continue;
        r += c;
    }
    return r + "\"";
}

std::string num(double v, int decimales = 3) {
    if (!std::isfinite(v)) return "null";
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%.*f", decimales, v);
    return buf;
}

std::string objetoEstadistica(const Estadistica& e, int decimales = 3) {
    return "{\"n\": " + std::to_string(e.n) + ", \"media\": " + num(e.media, decimales) + ", \"min\": " + num(e.min, decimales) +
           ", \"p5\": " + num(e.p5, decimales) + ", \"p50\": " + num(e.p50, decimales) + ", \"p95\": " + num(e.p95, decimales) +
           ", \"p99\": " + num(e.p99, decimales) + ", \"max\": " + num(e.max, decimales) + "}";
}

std::ofstream crear(const std::string& ruta) {
    const std::filesystem::path p(ruta);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("No se pudo crear " + ruta);
    return f;
}

constexpr double kMb = 1024.0 * 1024.0;

}  // namespace

bool leerConfigMedicion(int argc, char** argv, ConfigMedicion& cfg) {
    bool activo = false;
    for (int i = 1; i < argc; ++i)
        if (std::string_view(argv[i]) == "--medir-rendimiento") activo = true;
    if (!activo) return false;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string_view a = argv[i];
        const char* v = argv[i + 1];
        if (a == "--segundos") cfg.medicionSeg = numero(a, v);
        else if (a == "--calentamiento") cfg.calentamientoSeg = numero(a, v);
        else if (a == "--salida") cfg.salida = v;
        else if (a == "--escenario") cfg.escenario = v;
        else if (a == "--infectados") {
            const double n = numero(a, v);
            if (n < 1.0 || n > 500.0 || n != std::floor(n)) throw std::runtime_error("--infectados va de 1 a 500");
            cfg.infectados = static_cast<int>(n);
        } else
            continue;
        ++i;
    }
    if (cfg.salida.empty()) throw std::runtime_error("--salida no puede estar vacía");
    return true;
}

Estadistica estadistica(std::vector<double> v) {
    Estadistica e;
    e.n = v.size();
    if (v.empty()) return e;
    std::sort(v.begin(), v.end());
    const auto rango = [&](double p) {
        const auto k = static_cast<std::size_t>(std::ceil(p * static_cast<double>(v.size())));
        return v[std::clamp<std::size_t>(k, 1, v.size()) - 1];
    };
    e.media = std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
    e.min = v.front();
    e.max = v.back();
    e.p5 = rango(0.05);
    e.p50 = rango(0.50);
    e.p95 = rango(0.95);
    e.p99 = rango(0.99);
    return e;
}

juego::EntradaJugador entradaGuion(std::uint64_t tick, double hz) {
    static constexpr std::array<std::array<float, 2>, 8> kDirecciones{
        {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}}};
    const auto tramo = static_cast<std::uint64_t>(static_cast<double>(tick) / (2.0 * std::max(1.0, hz)));
    juego::EntradaJugador e;
    const auto& d = kDirecciones[tramo % kDirecciones.size()];
    e.ejeX = d[0];
    e.ejeY = d[1];
    if (tramo % 3 == 2) e.modo = juego::ModoMovimiento::Correr;
    else if (tramo % 5 == 4) e.modo = juego::ModoMovimiento::Sigilo;
    return e;
}

MedicionRendimiento::MedicionRendimiento(ConfigMedicion cfg) : cfg_(std::move(cfg)) {
    // 60 frames por segundo durante la medición, con margen.
    frames_.reserve(static_cast<std::size_t>(cfg_.medicionSeg * 70.0) + 16);
    tFrames_.reserve(frames_.capacity());
    segundos_.reserve(static_cast<std::size_t>(cfg_.medicionSeg + cfg_.calentamientoSeg) + 8);
}

void MedicionRendimiento::fase(const std::string& nombre, double ms) { fases_.emplace_back(nombre, ms); }

void MedicionRendimiento::primerFrame(double msDesdeMain, double msDesdeProceso) {
    primerFrameMain_ = msDesdeMain;
    primerFrameProceso_ = msDesdeProceso;
}

void MedicionRendimiento::escena(double ms) { escenas_.push_back(ms); }

void MedicionRendimiento::entorno(const std::string& clave, const std::string& valor) {
    for (auto& [k, v] : entorno_)
        if (k == clave) {
            v = valor;
            return;
        }
    entorno_.emplace_back(clave, valor);
}

bool MedicionRendimiento::midiendo(double t) const noexcept {
    return t >= cfg_.calentamientoSeg && t < cfg_.calentamientoSeg + cfg_.medicionSeg;
}

bool MedicionRendimiento::terminado(double t) const noexcept {
    return cfg_.medicionSeg <= 0.0 || t >= cfg_.calentamientoSeg + cfg_.medicionSeg;
}

void MedicionRendimiento::frame(double t, const MuestraFrame& m) {
    if (!midiendo(t)) return;
    frames_.push_back(m);
    tFrames_.push_back(t);
    if (m.reinicio) ++reinicios_;
}

bool MedicionRendimiento::tocaMuestra(double t) const noexcept { return t >= proximaMuestra_; }

void MedicionRendimiento::muestra(double t, double cpuSegundos, std::uint64_t memoriaBytes, std::uint64_t memoriaPicoBytes) {
    proximaMuestra_ = std::floor(t) + 1.0;
    if (midiendo(t)) {
        if (tInicio_ < 0.0) {  // primera muestra del periodo: punto de partida de la CPU
            tInicio_ = t;
            cpuInicio_ = cpuSegundos;
        } else {
            const double dt = t - tUltima_;
            const double pct = dt > 0.0 ? (cpuSegundos - cpuUltima_) / dt * 100.0 : 0.0;
            segundos_.push_back({t, pct, static_cast<double>(memoriaBytes) / kMb});
        }
        tUltima_ = t;
        cpuUltima_ = cpuSegundos;
    }
    memoriaPico_ = std::max({memoriaPico_, memoriaBytes, memoriaPicoBytes});
}

double MedicionRendimiento::cpuMediaPct() const noexcept {
    const double dt = tUltima_ - tInicio_;
    return tInicio_ >= 0.0 && dt > 0.0 ? (cpuUltima_ - cpuInicio_) / dt * 100.0 : 0.0;
}

double MedicionRendimiento::memoriaPicoMb() const noexcept { return static_cast<double>(memoriaPico_) / kMb; }

double MedicionRendimiento::fpsP5() const {
    std::vector<double> fps;
    fps.reserve(frames_.size());
    for (const auto& f : frames_)
        if (f.msFrame > 0.0) fps.push_back(1000.0 / f.msFrame);
    return estadistica(std::move(fps)).p5;
}

std::string MedicionRendimiento::json(const std::vector<std::string>& meta) const {
    std::vector<double> fps, msFrame, msAct, msDib, msTrabajo, msPaso, usIa;
    int lentos = 0;  // frames de más de 25 ms: se notan como tirón
    for (const auto& f : frames_) {
        if (f.msFrame > 0.0) fps.push_back(1000.0 / f.msFrame);
        msFrame.push_back(f.msFrame);
        msAct.push_back(f.msActualizar);
        msDib.push_back(f.msDibujar);
        msTrabajo.push_back(f.msActualizar + f.msDibujar);
        if (f.pasos > 0) msPaso.push_back(f.msPasoMax);
        usIa.push_back(f.usBusquedaIa);
        if (f.msFrame > 25.0) ++lentos;
    }
    std::vector<double> cpu, mem;
    for (const auto& s : segundos_) {
        cpu.push_back(s.cpuPct);
        mem.push_back(s.memoriaMb);
    }

    std::ostringstream o;
    o << "{\n  \"formato\": \"refugio-rendimiento-1\",\n  \"meta\": [";
    for (std::size_t i = 0; i < meta.size(); ++i) o << (i ? ", " : "") << cadena(meta[i]);
    o << "],\n  \"escenario\": " << cadena(cfg_.escenario) << ",\n  \"infectados\": " << cfg_.infectados
      << ",\n  \"calentamiento_s\": " << num(cfg_.calentamientoSeg, 1) << ",\n  \"medicion_s\": " << num(cfg_.medicionSeg, 1)
      << ",\n  \"entorno\": {";
    for (std::size_t i = 0; i < entorno_.size(); ++i)
        o << (i ? ", " : "") << cadena(entorno_[i].first) << ": " << cadena(entorno_[i].second);
    o << "},\n  \"arranque\": {\n    \"fases_ms\": {";
    for (std::size_t i = 0; i < fases_.size(); ++i) o << (i ? ", " : "") << cadena(fases_[i].first) << ": " << num(fases_[i].second);
    o << "},\n    \"primer_frame_desde_main_ms\": " << num(primerFrameMain_)
      << ",\n    \"primer_frame_desde_proceso_ms\": " << (primerFrameProceso_ >= 0.0 ? num(primerFrameProceso_) : "null")
      << ",\n    \"escenas_ms\": [";
    for (std::size_t i = 0; i < escenas_.size(); ++i) o << (i ? ", " : "") << num(escenas_[i]);
    o << "]\n  },\n  \"frames\": " << frames_.size() << ",\n  \"reinicios\": " << reinicios_
      << ",\n  \"frames_mas_de_25ms\": " << lentos << ",\n  \"fps\": " << objetoEstadistica(estadistica(fps), 2)
      << ",\n  \"frame_ms\": " << objetoEstadistica(estadistica(msFrame))
      << ",\n  \"actualizar_ms\": " << objetoEstadistica(estadistica(msAct))
      << ",\n  \"dibujar_ms\": " << objetoEstadistica(estadistica(msDib))
      << ",\n  \"trabajo_ms\": " << objetoEstadistica(estadistica(msTrabajo))
      << ",\n  \"paso_ms\": " << objetoEstadistica(estadistica(msPaso))
      << ",\n  \"busqueda_ia_us\": " << objetoEstadistica(estadistica(usIa), 1)
      << ",\n  \"cpu_pct\": " << objetoEstadistica(estadistica(cpu), 1) << ",\n  \"cpu_media_pct\": " << num(cpuMediaPct(), 2)
      << ",\n  \"memoria_mb\": " << objetoEstadistica(estadistica(mem), 1) << ",\n  \"memoria_pico_mb\": " << num(memoriaPicoMb(), 1)
      << "\n}\n";
    return o.str();
}

void MedicionRendimiento::escribir(const std::vector<std::string>& meta) const {
    {
        auto f = crear(cfg_.salida + ".json");
        f << json(meta);
    }
    const auto cabecera = [&](std::ofstream& f) {
        for (const auto& m : meta) f << "# " << m << '\n';
        f << "# escenario=" << cfg_.escenario << " infectados=" << cfg_.infectados;
        for (const auto& [k, v] : entorno_) f << ' ' << k << '=' << v;
        f << '\n';
    };
    {
        auto f = crear(cfg_.salida + "_frames.csv");
        cabecera(f);
        f << "t_s,ms_frame,ms_actualizar,ms_dibujar,pasos,ms_paso_max,us_busqueda_ia,reinicio\n";
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            const auto& m = frames_[i];
            f << num(tFrames_[i]) << ',' << num(m.msFrame) << ',' << num(m.msActualizar) << ',' << num(m.msDibujar) << ','
              << m.pasos << ',' << num(m.msPasoMax) << ',' << num(m.usBusquedaIa, 1) << ',' << (m.reinicio ? 1 : 0) << '\n';
        }
    }
    {
        auto f = crear(cfg_.salida + "_segundos.csv");
        cabecera(f);
        f << "t_s,cpu_pct,memoria_mb\n";
        for (const auto& s : segundos_) f << num(s.t) << ',' << num(s.cpuPct, 2) << ',' << num(s.memoriaMb, 2) << '\n';
    }
}

std::string MedicionRendimiento::resumenTexto() const {
    char buf[512];
    const double arranque = primerFrameProceso_ >= 0.0 ? primerFrameProceso_ : primerFrameMain_;
    const double esc = escenas_.empty() ? 0.0 : escenas_.front();
    if (frames_.empty()) {
        std::snprintf(buf, sizeof(buf), "Arranque: primer frame a %.0f ms (escena %.0f ms) -> %s.json\n", arranque, esc,
                      cfg_.salida.c_str());
        return buf;
    }
    std::vector<double> paso;
    for (const auto& f : frames_)
        if (f.pasos > 0) paso.push_back(f.msPasoMax);
    std::snprintf(buf, sizeof(buf),
                  "%s (%d infectados): %zu frames, FPS p5 %.1f, tick p99 %.2f ms, CPU media %.1f %% de un nucleo, "
                  "memoria pico %.1f MB, arranque %.0f ms -> %s.json\n",
                  cfg_.escenario.c_str(), cfg_.infectados, frames_.size(), fpsP5(), estadistica(std::move(paso)).p99,
                  cpuMediaPct(), memoriaPicoMb(), arranque, cfg_.salida.c_str());
    return buf;
}

}  // namespace refugio::app
