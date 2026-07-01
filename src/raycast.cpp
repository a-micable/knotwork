#include "knotwork/raycast.hpp"

#include <algorithm>
#include <limits>

namespace knotwork {
namespace {

Vec3 transform_point_for_ray(const Mat4& matrix, Vec3 point) {
  return {matrix.at(0, 0) * point.x + matrix.at(0, 1) * point.y + matrix.at(0, 2) * point.z +
              matrix.at(0, 3),
          matrix.at(1, 0) * point.x + matrix.at(1, 1) * point.y + matrix.at(1, 2) * point.z +
              matrix.at(1, 3),
          matrix.at(2, 0) * point.x + matrix.at(2, 1) * point.y + matrix.at(2, 2) * point.z +
              matrix.at(2, 3)};
}

Vec3 transform_vector_for_ray(const Mat4& matrix, Vec3 vector) {
  return {matrix.at(0, 0) * vector.x + matrix.at(0, 1) * vector.y + matrix.at(0, 2) * vector.z,
          matrix.at(1, 0) * vector.x + matrix.at(1, 1) * vector.y + matrix.at(1, 2) * vector.z,
          matrix.at(2, 0) * vector.x + matrix.at(2, 1) * vector.y + matrix.at(2, 2) * vector.z};
}

bool node_uses_mesh(const Node& node, MeshHandle& handle) {
  if (node.kind != NodeKind::Mesh && node.kind != NodeKind::Instance) {
    return false;
  }
  if (node.material == kNoIndex) {
    return false;
  }
  handle.index = node.material;
  return true;
}

}  // namespace

Ray normalize_ray(Ray ray) {
  ray.direction = normalize(ray.direction);
  return ray;
}

std::optional<float> intersect_aabb(const Ray& ray, Aabb bounds, float max_distance) {
  float t_min = 0.0f;
  float t_max = max_distance;
  const auto test_axis = [&](float origin, float direction, float min_value, float max_value) {
    if (std::fabs(direction) < 0.000001f) {
      return origin >= min_value && origin <= max_value;
    }
    const float inv = 1.0f / direction;
    float near_t = (min_value - origin) * inv;
    float far_t = (max_value - origin) * inv;
    if (near_t > far_t) {
      std::swap(near_t, far_t);
    }
    t_min = std::max(t_min, near_t);
    t_max = std::min(t_max, far_t);
    return t_min <= t_max;
  };
  if (!test_axis(ray.origin.x, ray.direction.x, bounds.min.x, bounds.max.x)) {
    return std::nullopt;
  }
  if (!test_axis(ray.origin.y, ray.direction.y, bounds.min.y, bounds.max.y)) {
    return std::nullopt;
  }
  if (!test_axis(ray.origin.z, ray.direction.z, bounds.min.z, bounds.max.z)) {
    return std::nullopt;
  }
  return t_min;
}

std::optional<RayHit> intersect_triangle(const Ray& ray, Vec3 a, Vec3 b, Vec3 c, bool backface_cull) {
  constexpr float epsilon = 0.000001f;
  const Vec3 edge1 = b - a;
  const Vec3 edge2 = c - a;
  const Vec3 p = cross(ray.direction, edge2);
  const float determinant = dot(edge1, p);
  if (backface_cull) {
    if (determinant < epsilon) {
      return std::nullopt;
    }
  } else if (std::fabs(determinant) < epsilon) {
    return std::nullopt;
  }
  const float inv_det = 1.0f / determinant;
  const Vec3 t = ray.origin - a;
  const float u = dot(t, p) * inv_det;
  if (u < 0.0f || u > 1.0f) {
    return std::nullopt;
  }
  const Vec3 q = cross(t, edge1);
  const float v = dot(ray.direction, q) * inv_det;
  if (v < 0.0f || u + v > 1.0f) {
    return std::nullopt;
  }
  const float distance = dot(edge2, q) * inv_det;
  if (distance < 0.0f) {
    return std::nullopt;
  }
  RayHit hit;
  hit.distance = distance;
  hit.position = ray.origin + ray.direction * distance;
  hit.normal = normalize(cross(edge1, edge2));
  return hit;
}

std::vector<RayHit> raycast_mesh(const MeshAsset& mesh,
                                 MeshHandle handle,
                                 const Mat4& world,
                                 const PickRequest& request,
                                 std::uint32_t node_index) {
  std::vector<RayHit> hits;
  const Ray ray = normalize_ray(request.ray);
  const auto local_bounds = mesh_bounds(mesh);
  if (local_bounds) {
    const Aabb world_bounds = transform_bounds(*local_bounds, world);
    if (!intersect_aabb(ray, world_bounds, request.max_distance)) {
      return hits;
    }
  }
  for (std::uint32_t i = 0; i < mesh.triangles.size(); ++i) {
    const Triangle triangle = mesh.triangles[i];
    if (!valid_triangle(mesh, triangle)) {
      continue;
    }
    const Vec3 a = transform_point_for_ray(world, mesh.vertices[triangle.a].position);
    const Vec3 b = transform_point_for_ray(world, mesh.vertices[triangle.b].position);
    const Vec3 c = transform_point_for_ray(world, mesh.vertices[triangle.c].position);
    auto hit = intersect_triangle(ray, a, b, c, request.backface_cull);
    if (!hit || hit->distance > request.max_distance) {
      continue;
    }
    hit->node = node_index;
    hit->mesh = handle;
    hit->triangle = i;
    hit->normal = normalize(transform_vector_for_ray(world, hit->normal));
    hits.push_back(*hit);
  }
  std::sort(hits.begin(), hits.end(), [](const RayHit& lhs, const RayHit& rhs) {
    return lhs.distance < rhs.distance;
  });
  return hits;
}

std::optional<RayHit> first_hit(std::vector<RayHit> hits) {
  if (hits.empty()) {
    return std::nullopt;
  }
  std::sort(hits.begin(), hits.end(), [](const RayHit& lhs, const RayHit& rhs) {
    return lhs.distance < rhs.distance;
  });
  return hits.front();
}

std::vector<RayHit> raycast_scene(const Scene& scene, const AssetLibrary& assets, const PickRequest& request) {
  std::vector<RayHit> hits;
  auto flattened = flatten_scene(scene, {request.include_instances, true});
  if (!flattened.ok()) {
    return hits;
  }
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    MeshHandle handle;
    if (!node_uses_mesh(scene.nodes[i], handle)) {
      continue;
    }
    const MeshAsset* mesh = assets.get(handle);
    if (mesh == nullptr) {
      continue;
    }
    std::vector<RayHit> mesh_hits = raycast_mesh(*mesh, handle, flattened.value()[i].world, request, i);
    hits.insert(hits.end(), mesh_hits.begin(), mesh_hits.end());
  }
  std::sort(hits.begin(), hits.end(), [](const RayHit& lhs, const RayHit& rhs) {
    return lhs.distance < rhs.distance;
  });
  return hits;
}

}  // namespace knotwork
