#pragma once

#include "knotwork/raycast.hpp"

#include <memory>

namespace knotwork {

struct IndexedTriangle {
  std::uint32_t triangle = kNoIndex;
  Aabb bounds{};
  Vec3 centroid{};
};

struct BvhNode {
  Aabb bounds{};
  std::uint32_t first = 0;
  std::uint32_t count = 0;
  std::int32_t left = -1;
  std::int32_t right = -1;
  [[nodiscard]] bool leaf() const { return left < 0 && right < 0; }
};

struct MeshBvh {
  MeshHandle mesh{};
  std::vector<IndexedTriangle> triangles;
  std::vector<BvhNode> nodes;
};

struct BvhBuildOptions {
  std::uint32_t max_leaf_triangles = 4;
  std::uint32_t max_depth = 32;
};

[[nodiscard]] MeshBvh build_mesh_bvh(const MeshAsset& mesh,
                                     MeshHandle handle = {},
                                     BvhBuildOptions options = {});
[[nodiscard]] std::vector<RayHit> raycast_bvh(const MeshAsset& mesh,
                                              const MeshBvh& bvh,
                                              const Mat4& world,
                                              const PickRequest& request,
                                              std::uint32_t node_index = kNoIndex);
[[nodiscard]] std::uint32_t bvh_depth(const MeshBvh& bvh);
[[nodiscard]] std::uint32_t bvh_leaf_count(const MeshBvh& bvh);

}  // namespace knotwork
