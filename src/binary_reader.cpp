#include "knotwork/format.hpp"

#include <cstring>

namespace knotwork {

class BinaryReader {
 public:
  explicit BinaryReader(std::span<const std::byte> bytes) : bytes_(bytes) {}

  template <typename T>
  bool read(T& value) {
    if (remaining() < sizeof(T)) {
      return false;
    }
    std::memcpy(&value, bytes_.data() + offset_, sizeof(T));
    offset_ += sizeof(T);
    return true;
  }

  bool read_bytes(std::span<std::byte> out) {
    if (remaining() < out.size()) {
      return false;
    }
    std::memcpy(out.data(), bytes_.data() + offset_, out.size());
    offset_ += out.size();
    return true;
  }

  [[nodiscard]] std::size_t remaining() const {
    return bytes_.size() - offset_;
  }

 private:
  std::span<const std::byte> bytes_;
  std::size_t offset_ = 0;
};

}  // namespace knotwork
