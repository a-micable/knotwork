#pragma once

#include "knotwork/scene.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace knotwork {

enum class SceneErrorCode {
  None,
  ShortRead,
  BadMagic,
  UnsupportedVersion,
  CountLimitExceeded,
  InvalidString,
  InvalidEnum,
  InvalidReference,
  DuplicateNodeId,
  Cycle,
  Io,
};

struct SceneError {
  SceneErrorCode code = SceneErrorCode::None;
  std::string message;
};

template <typename T>
class Result {
 public:
  Result(T value) : value_(std::move(value)) {}
  Result(SceneError error) : error_(std::move(error)) {}

  [[nodiscard]] bool ok() const { return value_.has_value(); }
  [[nodiscard]] const T& value() const { return *value_; }
  [[nodiscard]] T& value() { return *value_; }
  [[nodiscard]] const SceneError& error() const { return *error_; }

 private:
  std::optional<T> value_;
  std::optional<SceneError> error_;
};

[[nodiscard]] Result<Scene> load_scene(std::span<const std::byte> bytes);
[[nodiscard]] Result<Scene> load_scene_file(const std::filesystem::path& path);
[[nodiscard]] std::vector<std::byte> save_scene(const Scene& scene);
[[nodiscard]] bool save_scene_file(const std::filesystem::path& path, const Scene& scene, SceneError& error);
[[nodiscard]] const char* scene_error_name(SceneErrorCode code);

}  // namespace knotwork
