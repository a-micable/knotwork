#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace knotwork {

struct Checksums {
  std::uint32_t crc32 = 0;
  std::uint32_t adler32 = 1;
  std::uint64_t fnv1a64 = 1469598103934665603ull;
};

[[nodiscard]] std::uint32_t crc32(std::span<const std::byte> bytes);
[[nodiscard]] std::uint32_t adler32(std::span<const std::byte> bytes);
[[nodiscard]] std::uint64_t fnv1a64(std::span<const std::byte> bytes);
[[nodiscard]] Checksums checksums(std::span<const std::byte> bytes);
[[nodiscard]] std::string hex(std::uint64_t value, int width = 16);
[[nodiscard]] std::string checksum_text(const Checksums& sums);
[[nodiscard]] std::vector<std::byte> bytes_from_string(std::string_view text);

}  // namespace knotwork
