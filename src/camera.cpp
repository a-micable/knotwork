#include "knotwork/camera.hpp"
#include "knotwork/flatten.hpp"
#include "knotwork/mesh.hpp"

#include <cmath>

namespace knotwork {
namespace {

float radians(float degrees) {
  return degrees * 3.14159265358979323846f / 180.0f;
}

Plane normalize_plane(Plane plane) {
  const float len = length(plane.normal);
  if (len <= 0.000001f) {
    return plane;
  }
  plane.normal = plane.normal / len;
  plane.distance /= len;
  return plane;
}

float plane_distance(const Plane& plane, Vec3 point) {
  return dot(plane.normal, point) + plane.distance;
}

}  // namespace

Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) {
  const Vec3 f = normalize(target - eye);
  const Vec3 s = normalize(cross(f, up));
  const Vec3 u = cross(s, f);
  Mat4 out = Mat4::identity();
  out.at(0, 0) = s.x;
  out.at(0, 1) = s.y;
  out.at(0, 2) = s.z;
  out.at(0, 3) = -dot(s, eye);
  out.at(1, 0) = u.x;
  out.at(1, 1) = u.y;
  out.at(1, 2) = u.z;
  out.at(1, 3) = -dot(u, eye);
  out.at(2, 0) = -f.x;
  out.at(2, 1) = -f.y;
  out.at(2, 2) = -f.z;
  out.at(2, 3) = dot(f, eye);
  return out;
}

Mat4 perspective(float fov_y_degrees, float aspect, float near_clip, float far_clip) {
  const float f = 1.0f / std::tan(radians(fov_y_degrees) * 0.5f);
  Mat4 out;
  out.at(0, 0) = f / aspect;
  out.at(1, 1) = f;
  out.at(2, 2) = (far_clip + near_clip) / (near_clip - far_clip);
  out.at(2, 3) = (2.0f * far_clip * near_clip) / (near_clip - far_clip);
  out.at(3, 2) = -1.0f;
  return out;
}

Mat4 view_projection(const CameraView& camera) {
  return perspective(camera.fov_y_degrees, camera.aspect, camera.near_clip, camera.far_clip) *
         look_at(camera.eye, camera.target, camera.up);
}

Frustum extract_frustum(const Mat4& matrix) {
  Frustum frustum;
  frustum.planes[0] = normalize_plane({{matrix.at(3, 0) + matrix.at(0, 0),
                                        matrix.at(3, 1) + matrix.at(0, 1),
                                        matrix.at(3, 2) + matrix.at(0, 2)},
                                       matrix.at(3, 3) + matrix.at(0, 3)});
  frustum.planes[1] = normalize_plane({{matrix.at(3, 0) - matrix.at(0, 0),
                                        matrix.at(3, 1) - matrix.at(0, 1),
                                        matrix.at(3, 2) - matrix.at(0, 2)},
                                       matrix.at(3, 3) - matrix.at(0, 3)});
  frustum.planes[2] = normalize_plane({{matrix.at(3, 0) + matrix.at(1, 0),
                                        matrix.at(3, 1) + matrix.at(1, 1),
                                        matrix.at(3, 2) + matrix.at(1, 2)},
                                       matrix.at(3, 3) + matrix.at(1, 3)});
  frustum.planes[3] = normalize_plane({{matrix.at(3, 0) - matrix.at(1, 0),
                                        matrix.at(3, 1) - matrix.at(1, 1),
                                        matrix.at(3, 2) - matrix.at(1, 2)},
                                       matrix.at(3, 3) - matrix.at(1, 3)});
  frustum.planes[4] = normalize_plane({{matrix.at(3, 0) + matrix.at(2, 0),
                                        matrix.at(3, 1) + matrix.at(2, 1),
                                        matrix.at(3, 2) + matrix.at(2, 2)},
                                       matrix.at(3, 3) + matrix.at(2, 3)});
  frustum.planes[5] = normalize_plane({{matrix.at(3, 0) - matrix.at(2, 0),
                                        matrix.at(3, 1) - matrix.at(2, 1),
                                        matrix.at(3, 2) - matrix.at(2, 2)},
                                       matrix.at(3, 3) - matrix.at(2, 3)});
  return frustum;
}

bool inside_or_intersecting(const Frustum& frustum, Aabb bounds) {
  for (const Plane& plane : frustum.planes) {
    Vec3 positive = bounds.min;
    if (plane.normal.x >= 0.0f) {
      positive.x = bounds.max.x;
    }
    if (plane.normal.y >= 0.0f) {
      positive.y = bounds.max.y;
    }
    if (plane.normal.z >= 0.0f) {
      positive.z = bounds.max.z;
    }
    if (plane_distance(plane, positive) < 0.0f) {
      return false;
    }
  }
  return true;
}

bool contains_point(const Frustum& frustum, Vec3 point) {
  for (const Plane& plane : frustum.planes) {
    if (plane_distance(plane, point) < 0.0f) {
      return false;
    }
  }
  return true;
}

std::vector<std::uint32_t> visible_nodes(const Scene& scene,
                                         std::span<const LocalBounds> local_bounds,
                                         const CameraView& camera) {
  std::vector<std::uint32_t> out;
  auto flattened = flatten_scene(scene);
  if (!flattened.ok()) {
    return out;
  }
  const Frustum frustum = extract_frustum(view_projection(camera));
  for (const LocalBounds& local : local_bounds) {
    if (local.node >= flattened.value().size()) {
      continue;
    }
    if (inside_or_intersecting(frustum, transform_bounds(local.bounds, flattened.value()[local.node].world))) {
      out.push_back(local.node);
    }
  }
  return out;
}

CameraView camera_from_node(const Scene& scene, std::uint32_t node, float aspect, Vec3 fallback_forward) {
  CameraView camera;
  camera.aspect = aspect;
  if (node >= scene.nodes.size()) {
    camera.target = camera.eye + fallback_forward;
    return camera;
  }
  auto flattened = flatten_scene(scene);
  if (!flattened.ok()) {
    camera.target = camera.eye + fallback_forward;
    return camera;
  }
  const Mat4 world = flattened.value()[node].world;
  camera.eye = {world.at(0, 3), world.at(1, 3), world.at(2, 3)};
  camera.target = camera.eye + fallback_forward;
  camera.fov_y_degrees = scene.nodes[node].camera_fov_y;
  return camera;
}

}  // namespace knotwork
