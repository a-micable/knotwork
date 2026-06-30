#pragma once

#include "knotwork/math.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace knotwork {

constexpr std::uint32_t kNoIndex = 0xffffffffu;

enum class NodeKind : std::uint8_t {
  Empty = 0,
  Mesh = 1,
  Light = 2,
  Camera = 3,
  Instance = 4,
};

enum class LightKind : std::uint8_t {
  Point = 0,
  Directional = 1,
  Spot = 2,
};

struct Transform {
  Vec3 translation{};
  Quat rotation{};
  Vec3 scale{1.0f, 1.0f, 1.0f};

  [[nodiscard]] Mat4 matrix() const;
};

struct Material {
  std::uint32_t name = kNoIndex;
  Vec3 base_color{1.0f, 1.0f, 1.0f};
  float roughness = 0.5f;
  float metallic = 0.0f;
};

struct Node {
  std::uint32_t id = 0;
  std::uint32_t name = kNoIndex;
  NodeKind kind = NodeKind::Empty;
  std::uint32_t parent = kNoIndex;
  std::uint32_t material = kNoIndex;
  std::uint32_t instance_target = kNoIndex;
  Transform local{};
  LightKind light_kind = LightKind::Point;
  float light_intensity = 1.0f;
  float camera_fov_y = 60.0f;
};

struct Scene {
  std::vector<std::string> strings;
  std::vector<Material> materials;
  std::vector<Node> nodes;

  [[nodiscard]] std::optional<std::string_view> string_at(std::uint32_t index) const;
  [[nodiscard]] std::optional<std::uint32_t> node_index_by_id(std::uint32_t id) const;
  [[nodiscard]] std::vector<std::uint32_t> children_of(std::uint32_t parent_index) const;
};

struct WorldNode {
  std::uint32_t id = 0;
  NodeKind kind = NodeKind::Empty;
  Mat4 world = Mat4::identity();
};

[[nodiscard]] const char* node_kind_name(NodeKind kind);
[[nodiscard]] const char* light_kind_name(LightKind kind);

}  // namespace knotwork
