#include "adaptadores/archivo/RegistroArchivo.hpp"

#include <system_error>

#ifndef REFUGIO_VERSION
#define REFUGIO_VERSION "dev"
#endif

namespace refugio::adaptadores {

std::string RegistroArchivo::nombreArchivo(std::uint64_t semilla) {
    return "ia_" + std::to_string(semilla) + ".log";
}

RegistroArchivo::RegistroArchivo(const std::filesystem::path& carpeta, std::uint64_t semilla)
    : ruta_(carpeta / nombreArchivo(semilla)) {
    std::error_code ec;
    std::filesystem::create_directories(carpeta, ec);
    if (ec) return;
#ifdef _WIN32
    if (_wfopen_s(&archivo_, ruta_.c_str(), L"wb") != 0) archivo_ = nullptr;
#else
    archivo_ = std::fopen(ruta_.c_str(), "wb");
#endif
    if (archivo_ == nullptr) return;
    std::fprintf(archivo_, "# El ultimo refugio %s - registro de IA, semilla %llu\n# tick\ttipo\tagente\tcampos\n",
                 REFUGIO_VERSION, static_cast<unsigned long long>(semilla));
}

RegistroArchivo::~RegistroArchivo() {
    if (archivo_ != nullptr) std::fclose(archivo_);
}

void RegistroArchivo::escribir(std::uint64_t tick, std::string_view tipo, std::string_view linea) {
    if (archivo_ == nullptr) return;
    std::fprintf(archivo_, "%llu\t%.*s\t%.*s\n", static_cast<unsigned long long>(tick), static_cast<int>(tipo.size()),
                 tipo.data(), static_cast<int>(linea.size()), linea.data());
    ++lineas_;
}

void RegistroArchivo::vaciar() noexcept {
    if (archivo_ != nullptr) std::fflush(archivo_);
}

}  // namespace refugio::adaptadores
