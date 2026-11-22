#include "contador_asignaciones.hpp"

#include <atomic>
#include <cstdlib>
#include <new>

namespace {
std::atomic<bool> g_activo{false};
std::atomic<std::uint64_t> g_conteo{0};

void* asignar(std::size_t n) {
    if (g_activo.load(std::memory_order_relaxed)) g_conteo.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
    throw std::bad_alloc();
}
}  // namespace

namespace refugio::pruebas {
void activarConteoAsignaciones(bool activo) noexcept { g_activo.store(activo); }
std::uint64_t asignacionesContadas() noexcept { return g_conteo.load(); }
}  // namespace refugio::pruebas

// Reemplazo de los operadores globales (permitido por el estándar, [replacement.functions]).
void* operator new(std::size_t n) { return asignar(n); }
void* operator new[](std::size_t n) { return asignar(n); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
    try {
        return asignar(n);
    } catch (...) {
        return nullptr;
    }
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
    try {
        return asignar(n);
    } catch (...) {
        return nullptr;
    }
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
