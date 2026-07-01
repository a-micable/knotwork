#pragma once

#include "knotwork/bounds.hpp"

#include <array>
#include <vector>

namespace knotwork {

struct Plane {
  Vec3 normal{};
  float distance = 0.0f;
};

struct Frustum {
  std::array<Plane, 6> planes{};
};

struct CameraView {
  Vec3 eye{0.0f, 0.0f, 1.0f};
  Vec3 target{0.0f, 0.0f, 0.0f};
  Vec3 up{0.0f, 1.0f, 0.0f};
  float fov_y_degrees = 60.0f;
  float aspect = 1.0f;
  float near_clip = 0.1f;
  float far_clip = 1000.0f;
};

[[nodiscard]] Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up);
[[nodiscard]] Mat4 perspective(float fov_y_degrees, float aspect, float near_clip, float far_clip);
[[nodiscard]] Mat4 view_projection(const CameraView& camera);
[[nodiscard]] Frustum extract_frustum(const Mat4& view_projection);
[[nodiscard]] bool inside_or_intersecting(const Frustum& frustum, Aabb bounds);
[[nodiscard]] bool contains_point(const Frustum& frustum, Vec3 point);
[[nodiscard]] std::vector<std::uint32_t> visible_nodes(const Scene& scene,
                                                       std::span<const LocalBounds> local_bounds,
                                                       const CameraView& camera);
[[nodiscard]] CameraView camera_from_node(const Scene& scene,
                                          std::uint32_t node,
                                          float aspect,
                                          Vec3 fallback_forward = {0.0f, 0.0f, -1.0f});

}  // namespace knotwork
