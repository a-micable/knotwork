#include "knotwork/checksum.hpp"

#include <cstddef>
#include <iomanip>
#include <sstream>

namespace knotwork {

std::uint32_t crc32(std::span<const std::byte> bytes) {
  std::uint32_t crc = 0xffffffffu;
  for (std::byte byte : bytes) {
    crc ^= std::to_integer<std::uint32_t>(byte);
    for (int i = 0; i < 8; ++i) {
      const std::uint32_t mask = -(crc & 1u);
      crc = (crc >> 1u) ^ (0xedb88320u & mask);
    }
  }
  return ~crc;
}

std::uint32_t adler32(std::span<const std::byte> bytes) {
  constexpr std::uint32_t mod = 65521u;
  std::uint32_t a = 1;
  std::uint32_t b = 0;
  for (std::byte byte : bytes) {
    a = (a + std::to_integer<std::uint32_t>(byte)) % mod;
    b = (b + a) % mod;
  }
  return (b << 16u) | a;
}

std::uint64_t fnv1a64(std::span<const std::byte> bytes) {
  std::uint64_t hash = 1469598103934665603ull;
  for (std::byte byte : bytes) {
    hash ^= std::to_integer<std::uint64_t>(byte);
    hash *= 1099511628211ull;
  }
  return hash;
}

Checksums checksums(std::span<const std::byte> bytes) {
  return {crc32(bytes), adler32(bytes), fnv1a64(bytes)};
}

std::string hex(std::uint64_t value, int width) {
  std::ostringstream out;
  out << std::hex << std::setw(width) << std::setfill('0') << value;
  return out.str();
}

std::string checksum_text(const Checksums& sums) {
  std::ostringstream out;
  out << "crc32=" << hex(sums.crc32, 8) << "\n";
  out << "adler32=" << hex(sums.adler32, 8) << "\n";
  out << "fnv1a64=" << hex(sums.fnv1a64, 16) << "\n";
  return out.str();
}

std::vector<std::byte> bytes_from_string(std::string_view text) {
  const auto* first = reinterpret_cast<const std::byte*>(text.data());
  return {first, first + text.size()};
}

}  // namespace knotwork
