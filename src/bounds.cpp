#include "knotwork/bounds.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace knotwork {

Vec3 Aabb::center() const {
  return {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f};
}

Vec3 Aabb::extent() const {
  return {(max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f, (max.z - min.z) * 0.5f};
}

bool Aabb::contains(Vec3 point) const {
  return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y &&
         point.z >= min.z && point.z <= max.z;
}

Aabb empty_bounds() {
  const float inf = std::numeric_limits<float>::infinity();
  return {{inf, inf, inf}, {-inf, -inf, -inf}};
}

Aabb point_bounds(Vec3 point) {
  return {point, point};
}

Aabb merge(Aabb lhs, Aabb rhs) {
  return {{std::min(lhs.min.x, rhs.min.x), std::min(lhs.min.y, rhs.min.y), std::min(lhs.min.z, rhs.min.z)},
          {std::max(lhs.max.x, rhs.max.x), std::max(lhs.max.y, rhs.max.y), std::max(lhs.max.z, rhs.max.z)}};
}

Aabb transform_bounds(Aabb bounds, const Mat4& matrix) {
  const std::array<Vec3, 8> corners{{
      {bounds.min.x, bounds.min.y, bounds.min.z},
      {bounds.max.x, bounds.min.y, bounds.min.z},
      {bounds.min.x, bounds.max.y, bounds.min.z},
      {bounds.max.x, bounds.max.y, bounds.min.z},
      {bounds.min.x, bounds.min.y, bounds.max.z},
      {bounds.max.x, bounds.min.y, bounds.max.z},
      {bounds.min.x, bounds.max.y, bounds.max.z},
      {bounds.max.x, bounds.max.y, bounds.max.z},
  }};
  Aabb out = empty_bounds();
  for (Vec3 corner : corners) {
    Vec3 transformed;
    transformed.x = matrix.at(0, 0) * corner.x + matrix.at(0, 1) * corner.y +
                    matrix.at(0, 2) * corner.z + matrix.at(0, 3);
    transformed.y = matrix.at(1, 0) * corner.x + matrix.at(1, 1) * corner.y +
                    matrix.at(1, 2) * corner.z + matrix.at(1, 3);
    transformed.z = matrix.at(2, 0) * corner.x + matrix.at(2, 1) * corner.y +
                    matrix.at(2, 2) * corner.z + matrix.at(2, 3);
    out = merge(out, point_bounds(transformed));
  }
  return out;
}

std::optional<Aabb> scene_bounds(const Scene& scene, std::span<const LocalBounds> local_bounds) {
  auto flattened = flatten_scene(scene);
  if (!flattened.ok()) {
    return std::nullopt;
  }
  bool any = false;
  Aabb out = empty_bounds();
  for (const LocalBounds& local : local_bounds) {
    if (local.node >= flattened.value().size()) {
      continue;
    }
    out = merge(out, transform_bounds(local.bounds, flattened.value()[local.node].world));
    any = true;
  }
  if (!any) {
    return std::nullopt;
  }
  return out;
}

std::vector<LocalBounds> default_unit_mesh_bounds(const Scene& scene) {
  std::vector<LocalBounds> out;
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    if (scene.nodes[i].kind == NodeKind::Mesh || scene.nodes[i].kind == NodeKind::Instance) {
      out.push_back({i, {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}});
    }
  }
  return out;
}

}  // namespace knotwork
