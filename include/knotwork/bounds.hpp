#pragma once

#include "knotwork/flatten.hpp"

#include <optional>
#include <vector>

namespace knotwork {

struct Aabb {
  Vec3 min{};
  Vec3 max{};

  [[nodiscard]] Vec3 center() const;
  [[nodiscard]] Vec3 extent() const;
  [[nodiscard]] bool contains(Vec3 point) const;
};

struct LocalBounds {
  std::uint32_t node = kNoIndex;
  Aabb bounds{};
};

[[nodiscard]] Aabb empty_bounds();
[[nodiscard]] Aabb point_bounds(Vec3 point);
[[nodiscard]] Aabb merge(Aabb lhs, Aabb rhs);
[[nodiscard]] Aabb transform_bounds(Aabb bounds, const Mat4& matrix);
[[nodiscard]] std::optional<Aabb> scene_bounds(const Scene& scene,
                                               std::span<const LocalBounds> local_bounds);
[[nodiscard]] std::vector<LocalBounds> default_unit_mesh_bounds(const Scene& scene);

}  // namespace knotwork
