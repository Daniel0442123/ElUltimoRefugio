/// @file CarpetaEjecutable.cpp
/// @brief Implementación de CarpetaEjecutable.hpp.

#include "adaptadores/sistema/CarpetaEjecutable.hpp"

#include <cstdint>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <vector>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>

#include <vector>
#endif

namespace fs = std::filesystem;

namespace refugio::adaptadores {

fs::path rutaEjecutable() {
#if defined(_WIN32)
    std::vector<wchar_t> buf(MAX_PATH);
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
        if (n == 0) return {};
        if (n < buf.size()) return fs::path(std::wstring(buf.data(), n));
        if (buf.size() >= 32768) return {};
        buf.resize(buf.size() * 2);
    }
#elif defined(__APPLE__)
    std::error_code ec;
    std::uint32_t n = 0;
    _NSGetExecutablePath(nullptr, &n);
    std::vector<char> buf(n + 1, '\0');
    if (_NSGetExecutablePath(buf.data(), &n) != 0) return {};
    auto p = fs::weakly_canonical(fs::path(buf.data()), ec);
    return ec ? fs::path(buf.data()) : p;
#else
    std::error_code ec;
    auto p = fs::read_symlink("/proc/self/exe", ec);
    return ec ? fs::path{} : p;
#endif
}

std::optional<fs::path> carpetaDatos(const fs::path& actual, const fs::path& ejecutable) {
    if (ejecutable.empty()) return std::nullopt;
    std::error_code ec;
    if (fs::exists(actual / kMarcaCarpetaDatos, ec)) return std::nullopt;
    const fs::path carpeta = ejecutable.parent_path();
    if (carpeta.empty() || !fs::exists(carpeta / kMarcaCarpetaDatos, ec)) return std::nullopt;
    if (fs::equivalent(actual, carpeta, ec)) return std::nullopt;
    return carpeta;
}

std::optional<fs::path> usarCarpetaDelEjecutable() {
    std::error_code ec;
    const fs::path actual = fs::current_path(ec);
    if (ec) return std::nullopt;
    auto destino = carpetaDatos(actual, rutaEjecutable());
    if (!destino) return std::nullopt;
    fs::current_path(*destino, ec);
    if (ec) return std::nullopt;
    return destino;
}

}  // namespace refugio::adaptadores
