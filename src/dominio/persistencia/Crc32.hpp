#pragma once
/// @file Crc32.hpp
/// @brief CRC-32 (IEEE 802.3, polinomio reflejado 0xEDB88320) para las partidas guardadas (ADR-0011, RNF-021).
///
/// Detecta cualquier error de 1 a 4 bytes seguidos (y por tanto todas las mutaciones de 1 byte
/// de CP-121). No es una protección contra manipulación intencionada: solo de integridad.

#include <cstddef>
#include <cstdint>
#include <span>

namespace refugio::persistencia {

/// CRC-32 de @p datos. crc32(«123456789») == 0xCBF43926.
[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> datos) noexcept;

}  // namespace refugio::persistencia
