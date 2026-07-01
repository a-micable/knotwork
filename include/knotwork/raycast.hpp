#pragma once

#include "knotwork/asset_library.hpp"
#include "knotwork/flatten.hpp"

#include <optional>
#include <vector>

namespace knotwork {

struct Ray {
  Vec3 origin{};
  Vec3 direction{0.0f, 0.0f, -1.0f};
};

struct RayHit {
  float distance = 0.0f;
  Vec3 position{};
  Vec3 normal{};
  std::uint32_t node = kNoIndex;
  MeshHandle mesh{};
  std::uint32_t triangle = kNoIndex;
};

struct PickRequest {
  Ray ray{};
  bool include_instances = true;
  bool backface_cull = false;
  float max_distance = 1000000.0f;
};

[[nodiscard]] Ray normalize_ray(Ray ray);
[[nodiscard]] std::optional<float> intersect_aabb(const Ray& ray, Aabb bounds, float max_distance);
[[nodiscard]] std::optional<RayHit> intersect_triangle(const Ray& ray,
                                                       Vec3 a,
                                                       Vec3 b,
                                                       Vec3 c,
                                                       bool backface_cull);
[[nodiscard]] std::vector<RayHit> raycast_mesh(const MeshAsset& mesh,
                                               MeshHandle handle,
                                               const Mat4& world,
                                               const PickRequest& request,
                                               std::uint32_t node_index = kNoIndex);
[[nodiscard]] std::optional<RayHit> first_hit(std::vector<RayHit> hits);
[[nodiscard]] std::vector<RayHit> raycast_scene(const Scene& scene,
                                                const AssetLibrary& assets,
                                                const PickRequest& request);

}  // namespace knotwork
