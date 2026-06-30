#include "knotwork/format.hpp"

#include <cstring>

namespace knotwork {

class BinaryWriter {
 public:
  template <typename T>
  void write(const T& value) {
    const auto* ptr = reinterpret_cast<const std::byte*>(&value);
    bytes_.insert(bytes_.end(), ptr, ptr + sizeof(T));
  }

  void write_bytes(std::span<const std::byte> bytes) {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
  }

  [[nodiscard]] std::vector<std::byte> take() {
    return std::move(bytes_);
  }

 private:
  std::vector<std::byte> bytes_;
};

}  // namespace knotwork
