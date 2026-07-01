#include "knotwork/spatial_index.hpp"

#include <algorithm>

namespace knotwork {
namespace {

Aabb triangle_bounds(const MeshAsset& mesh, Triangle triangle) {
  Aabb bounds = point_bounds(mesh.vertices[triangle.a].position);
  bounds = merge(bounds, point_bounds(mesh.vertices[triangle.b].position));
  bounds = merge(bounds, point_bounds(mesh.vertices[triangle.c].position));
  return bounds;
}

Vec3 centroid(Aabb bounds) {
  return bounds.center();
}

int widest_axis(Aabb bounds) {
  const Vec3 e = bounds.extent();
  if (e.x >= e.y && e.x >= e.z) {
    return 0;
  }
  if (e.y >= e.z) {
    return 1;
  }
  return 2;
}

float axis_value(Vec3 value, int axis) {
  if (axis == 0) {
    return value.x;
  }
  if (axis == 1) {
    return value.y;
  }
  return value.z;
}

Aabb range_bounds(const std::vector<IndexedTriangle>& triangles, std::uint32_t first, std::uint32_t count) {
  Aabb bounds = empty_bounds();
  for (std::uint32_t i = 0; i < count; ++i) {
    bounds = merge(bounds, triangles[first + i].bounds);
  }
  return bounds;
}

std::int32_t build_node(MeshBvh& bvh,
                        std::uint32_t first,
                        std::uint32_t count,
                        std::uint32_t depth,
                        const BvhBuildOptions& options) {
  BvhNode node;
  node.first = first;
  node.count = count;
  node.bounds = range_bounds(bvh.triangles, first, count);
  const std::int32_t node_index = static_cast<std::int32_t>(bvh.nodes.size());
  bvh.nodes.push_back(node);

  if (count <= options.max_leaf_triangles || depth >= options.max_depth) {
    return node_index;
  }

  const int axis = widest_axis(node.bounds);
  const auto begin = bvh.triangles.begin() + static_cast<std::ptrdiff_t>(first);
  const auto end = begin + static_cast<std::ptrdiff_t>(count);
  std::sort(begin, end, [axis](const IndexedTriangle& lhs, const IndexedTriangle& rhs) {
    return axis_value(lhs.centroid, axis) < axis_value(rhs.centroid, axis);
  });

  const std::uint32_t left_count = count / 2;
  const std::uint32_t right_count = count - left_count;
  if (left_count == 0 || right_count == 0) {
    return node_index;
  }

  bvh.nodes[node_index].left = build_node(bvh, first, left_count, depth + 1, options);
  bvh.nodes[node_index].right = build_node(bvh, first + left_count, right_count, depth + 1, options);
  bvh.nodes[node_index].count = 0;
  return node_index;
}

std::uint32_t depth_from(const MeshBvh& bvh, std::int32_t node) {
  if (node < 0 || static_cast<std::size_t>(node) >= bvh.nodes.size()) {
    return 0;
  }
  const BvhNode& item = bvh.nodes[node];
  if (item.leaf()) {
    return 1;
  }
  return 1 + std::max(depth_from(bvh, item.left), depth_from(bvh, item.right));
}

std::uint32_t leaves_from(const MeshBvh& bvh, std::int32_t node) {
  if (node < 0 || static_cast<std::size_t>(node) >= bvh.nodes.size()) {
    return 0;
  }
  const BvhNode& item = bvh.nodes[node];
  if (item.leaf()) {
    return 1;
  }
  return leaves_from(bvh, item.left) + leaves_from(bvh, item.right);
}

void raycast_node(const MeshAsset& mesh,
                  const MeshBvh& bvh,
                  std::int32_t node_index,
                  const Mat4& world,
                  const PickRequest& request,
                  std::uint32_t scene_node,
                  std::vector<RayHit>& hits) {
  if (node_index < 0 || static_cast<std::size_t>(node_index) >= bvh.nodes.size()) {
    return;
  }
  const BvhNode& node = bvh.nodes[node_index];
  if (!intersect_aabb(normalize_ray(request.ray), transform_bounds(node.bounds, world), request.max_distance)) {
    return;
  }
  if (node.leaf()) {
    for (std::uint32_t i = 0; i < node.count; ++i) {
      const std::uint32_t triangle_index = bvh.triangles[node.first + i].triangle;
      if (triangle_index >= mesh.triangles.size()) {
        continue;
      }
      MeshAsset one;
      one.vertices = mesh.vertices;
      one.triangles = {mesh.triangles[triangle_index]};
      std::vector<RayHit> local = raycast_mesh(one, bvh.mesh, world, request, scene_node);
      for (RayHit& hit : local) {
        hit.triangle = triangle_index;
        hits.push_back(hit);
      }
    }
  } else {
    raycast_node(mesh, bvh, node.left, world, request, scene_node, hits);
    raycast_node(mesh, bvh, node.right, world, request, scene_node, hits);
  }
}

}  // namespace

MeshBvh build_mesh_bvh(const MeshAsset& mesh, MeshHandle handle, BvhBuildOptions options) {
  MeshBvh bvh;
  bvh.mesh = handle;
  for (std::uint32_t i = 0; i < mesh.triangles.size(); ++i) {
    const Triangle triangle = mesh.triangles[i];
    if (!valid_triangle(mesh, triangle) || degenerate_triangle(mesh, triangle)) {
      continue;
    }
    const Aabb bounds = triangle_bounds(mesh, triangle);
    bvh.triangles.push_back({i, bounds, centroid(bounds)});
  }
  if (!bvh.triangles.empty()) {
    build_node(bvh, 0, static_cast<std::uint32_t>(bvh.triangles.size()), 0, options);
  }
  return bvh;
}

std::vector<RayHit> raycast_bvh(const MeshAsset& mesh,
                                const MeshBvh& bvh,
                                const Mat4& world,
                                const PickRequest& request,
                                std::uint32_t node_index) {
  std::vector<RayHit> hits;
  if (!bvh.nodes.empty()) {
    raycast_node(mesh, bvh, 0, world, request, node_index, hits);
  }
  std::sort(hits.begin(), hits.end(), [](const RayHit& lhs, const RayHit& rhs) {
    return lhs.distance < rhs.distance;
  });
  return hits;
}

std::uint32_t bvh_depth(const MeshBvh& bvh) {
  return depth_from(bvh, 0);
}

std::uint32_t bvh_leaf_count(const MeshBvh& bvh) {
  return leaves_from(bvh, 0);
}

}  // namespace knotwork
