#include "dominio/persistencia/Crc32.hpp"

#include <array>

namespace refugio::persistencia {

namespace {
constexpr std::array<std::uint32_t, 256> crearTabla() noexcept {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t i = 0; i < 256; ++i) {
        std::uint32_t c = i;
        for (int k = 0; k < 8; ++k) c = (c & 1u) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        t[i] = c;
    }
    return t;
}
constexpr auto kTabla = crearTabla();
}  // namespace

std::uint32_t crc32(std::span<const std::uint8_t> datos) noexcept {
    std::uint32_t c = 0xFFFFFFFFu;
    for (const std::uint8_t b : datos) c = kTabla[(c ^ b) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

}  // namespace refugio::persistencia
