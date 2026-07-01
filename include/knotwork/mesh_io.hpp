#pragma once

#include "knotwork/mesh.hpp"

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace knotwork {

struct MeshIoError {
  std::string message;
};

template <typename T>
class MeshIoResult {
 public:
  MeshIoResult(T value) : value_(std::move(value)) {}
  MeshIoResult(MeshIoError error) : error_(std::move(error)) {}
  [[nodiscard]] bool ok() const { return value_.has_value(); }
  [[nodiscard]] const T& value() const { return *value_; }
  [[nodiscard]] T& value() { return *value_; }
  [[nodiscard]] const MeshIoError& error() const { return *error_; }

 private:
  std::optional<T> value_;
  std::optional<MeshIoError> error_;
};

[[nodiscard]] std::vector<std::byte> save_mesh_binary(const MeshAsset& mesh);
[[nodiscard]] MeshIoResult<MeshAsset> load_mesh_binary(std::span<const std::byte> bytes);
[[nodiscard]] bool save_mesh_binary_file(const std::filesystem::path& path,
                                         const MeshAsset& mesh,
                                         MeshIoError& error);
[[nodiscard]] MeshIoResult<MeshAsset> load_mesh_binary_file(const std::filesystem::path& path);
[[nodiscard]] std::uint64_t mesh_content_hash(const MeshAsset& mesh);

}  // namespace knotwork
