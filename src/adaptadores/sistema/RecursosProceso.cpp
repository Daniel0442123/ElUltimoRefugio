/// @file RecursosProceso.cpp
/// @brief Implementación por sistema operativo de RecursosProceso.hpp.

#include "adaptadores/sistema/RecursosProceso.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// psapi.h necesita windows.h antes.
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#endif

namespace refugio::adaptadores {

#if defined(_WIN32)
namespace {
/// FILETIME en unidades de 100 ns.
std::uint64_t cien(const FILETIME& f) noexcept {
    return (static_cast<std::uint64_t>(f.dwHighDateTime) << 32) | f.dwLowDateTime;
}
}  // namespace

RecursosProceso leerRecursosProceso() noexcept {
    RecursosProceso r;
    FILETIME creacion{}, salida{}, nucleo{}, usuario{};
    if (GetProcessTimes(GetCurrentProcess(), &creacion, &salida, &nucleo, &usuario))
        r.cpuSegundos = static_cast<double>(cien(nucleo) + cien(usuario)) * 1e-7;
    PROCESS_MEMORY_COUNTERS m{};
    m.cb = sizeof(m);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof(m))) {
        r.memoriaBytes = static_cast<std::uint64_t>(m.WorkingSetSize);
        r.memoriaPicoBytes = static_cast<std::uint64_t>(m.PeakWorkingSetSize);
    }
    return r;
}

double segundosDesdeCreacionProceso() noexcept {
    FILETIME creacion{}, salida{}, nucleo{}, usuario{};
    if (!GetProcessTimes(GetCurrentProcess(), &creacion, &salida, &nucleo, &usuario)) return -1.0;
    FILETIME ahora{};
    GetSystemTimePreciseAsFileTime(&ahora);
    return static_cast<double>(cien(ahora) - cien(creacion)) * 1e-7;
}

#else
RecursosProceso leerRecursosProceso() noexcept {
    RecursosProceso r;
    rusage u{};
    if (getrusage(RUSAGE_SELF, &u) == 0) {
        r.cpuSegundos = static_cast<double>(u.ru_utime.tv_sec + u.ru_stime.tv_sec) +
                        static_cast<double>(u.ru_utime.tv_usec + u.ru_stime.tv_usec) * 1e-6;
#if defined(__APPLE__)
        r.memoriaPicoBytes = static_cast<std::uint64_t>(u.ru_maxrss);  // bytes en macOS
#else
        r.memoriaPicoBytes = static_cast<std::uint64_t>(u.ru_maxrss) * 1024u;  // KiB en Linux
#endif
    }
#if defined(__linux__)
    // /proc/self/statm: tamaño total y residente, en páginas.
    if (std::FILE* f = std::fopen("/proc/self/statm", "r")) {
        unsigned long long total = 0, residente = 0;
        if (std::fscanf(f, "%llu %llu", &total, &residente) == 2)
            r.memoriaBytes = residente * static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
        std::fclose(f);
    }
#endif
    return r;
}

double segundosDesdeCreacionProceso() noexcept {
#if defined(__linux__)
    // Campo 22 de /proc/self/stat: inicio del proceso en ticks de reloj desde el arranque del
    // sistema; /proc/uptime da los segundos desde el arranque.
    try {
        std::ifstream st("/proc/self/stat");
        std::string linea;
        if (!std::getline(st, linea)) return -1.0;
        const auto cierre = linea.rfind(')');  // el nombre del proceso puede tener espacios
        if (cierre == std::string::npos) return -1.0;
        std::istringstream campos(linea.substr(cierre + 2));
        std::string c;
        unsigned long long inicioTicks = 0;
        for (int i = 3; i <= 22 && (campos >> c); ++i)
            if (i == 22) inicioTicks = std::stoull(c);
        std::ifstream up("/proc/uptime");
        double encendido = 0.0;
        if (!(up >> encendido)) return -1.0;
        return encendido - static_cast<double>(inicioTicks) / static_cast<double>(sysconf(_SC_CLK_TCK));
    } catch (...) {
        return -1.0;
    }
#else
    return -1.0;
#endif
}
#endif

}  // namespace refugio::adaptadores
