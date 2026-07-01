#include "knotwork/mesh.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace knotwork {
namespace {

struct WeldKey {
  int x = 0;
  int y = 0;
  int z = 0;

  bool operator==(const WeldKey& other) const {
    return x == other.x && y == other.y && z == other.z;
  }
};

struct WeldHash {
  std::size_t operator()(const WeldKey& key) const {
    std::size_t seed = static_cast<std::size_t>(key.x) * 73856093u;
    seed ^= static_cast<std::size_t>(key.y) * 19349663u;
    seed ^= static_cast<std::size_t>(key.z) * 83492791u;
    return seed;
  }
};

WeldKey weld_key(Vec3 position, float epsilon) {
  const float inv = epsilon > 0.0f ? 1.0f / epsilon : 1.0f;
  return {static_cast<int>(std::round(position.x * inv)),
          static_cast<int>(std::round(position.y * inv)),
          static_cast<int>(std::round(position.z * inv))};
}

Vertex vertex(Vec3 position, Vec3 normal, Vec2 texcoord) {
  Vertex out;
  out.position = position;
  out.normal = normal;
  out.texcoord = texcoord;
  return out;
}

Vec3 transform_point(const Mat4& matrix, Vec3 point) {
  return {matrix.at(0, 0) * point.x + matrix.at(0, 1) * point.y + matrix.at(0, 2) * point.z +
              matrix.at(0, 3),
          matrix.at(1, 0) * point.x + matrix.at(1, 1) * point.y + matrix.at(1, 2) * point.z +
              matrix.at(1, 3),
          matrix.at(2, 0) * point.x + matrix.at(2, 1) * point.y + matrix.at(2, 2) * point.z +
              matrix.at(2, 3)};
}

Vec3 transform_vector(const Mat4& matrix, Vec3 vector) {
  return {matrix.at(0, 0) * vector.x + matrix.at(0, 1) * vector.y + matrix.at(0, 2) * vector.z,
          matrix.at(1, 0) * vector.x + matrix.at(1, 1) * vector.y + matrix.at(1, 2) * vector.z,
          matrix.at(2, 0) * vector.x + matrix.at(2, 1) * vector.y + matrix.at(2, 2) * vector.z};
}

}  // namespace

Vec3 operator+(Vec3 lhs, Vec3 rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

Vec3 operator-(Vec3 lhs, Vec3 rhs) {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

Vec3 operator*(Vec3 lhs, float rhs) {
  return {lhs.x * rhs, lhs.y * rhs, lhs.z * rhs};
}

Vec3 operator/(Vec3 lhs, float rhs) {
  return {lhs.x / rhs, lhs.y / rhs, lhs.z / rhs};
}

float dot(Vec3 lhs, Vec3 rhs) {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

Vec3 cross(Vec3 lhs, Vec3 rhs) {
  return {lhs.y * rhs.z - lhs.z * rhs.y,
          lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x};
}

float length(Vec3 value) {
  return std::sqrt(dot(value, value));
}

Vec3 normalize(Vec3 value) {
  const float len = length(value);
  if (len <= std::numeric_limits<float>::epsilon()) {
    return {};
  }
  return value / len;
}

float triangle_area(Vec3 a, Vec3 b, Vec3 c) {
  return length(cross(b - a, c - a)) * 0.5f;
}

bool valid_triangle(const MeshAsset& mesh, Triangle triangle) {
  return triangle.a < mesh.vertices.size() && triangle.b < mesh.vertices.size() &&
         triangle.c < mesh.vertices.size();
}

bool degenerate_triangle(const MeshAsset& mesh, Triangle triangle, float epsilon) {
  if (!valid_triangle(mesh, triangle)) {
    return true;
  }
  if (triangle.a == triangle.b || triangle.b == triangle.c || triangle.a == triangle.c) {
    return true;
  }
  const Vec3 a = mesh.vertices[triangle.a].position;
  const Vec3 b = mesh.vertices[triangle.b].position;
  const Vec3 c = mesh.vertices[triangle.c].position;
  return triangle_area(a, b, c) <= epsilon;
}

std::optional<Aabb> mesh_bounds(const MeshAsset& mesh) {
  if (mesh.vertices.empty()) {
    return std::nullopt;
  }
  Aabb bounds = point_bounds(mesh.vertices.front().position);
  for (const Vertex& vertex : mesh.vertices) {
    bounds = merge(bounds, point_bounds(vertex.position));
  }
  return bounds;
}

MeshStats mesh_stats(const MeshAsset& mesh) {
  MeshStats stats;
  stats.vertex_count = static_cast<std::uint32_t>(mesh.vertices.size());
  stats.triangle_count = static_cast<std::uint32_t>(mesh.triangles.size());
  stats.bounds = mesh_bounds(mesh);
  for (Triangle triangle : mesh.triangles) {
    if (degenerate_triangle(mesh, triangle)) {
      ++stats.degenerate_count;
      continue;
    }
    const Vec3 a = mesh.vertices[triangle.a].position;
    const Vec3 b = mesh.vertices[triangle.b].position;
    const Vec3 c = mesh.vertices[triangle.c].position;
    stats.surface_area += triangle_area(a, b, c);
  }
  return stats;
}

std::vector<MeshError> validate_mesh(const MeshAsset& mesh) {
  std::vector<MeshError> errors;
  for (std::uint32_t i = 0; i < mesh.triangles.size(); ++i) {
    const Triangle triangle = mesh.triangles[i];
    if (!valid_triangle(mesh, triangle)) {
      errors.push_back({"triangle references a missing vertex", i});
    } else if (degenerate_triangle(mesh, triangle)) {
      errors.push_back({"triangle is degenerate", i});
    }
  }
  return errors;
}

void recalculate_normals(MeshAsset& mesh) {
  for (Vertex& vertex : mesh.vertices) {
    vertex.normal = {};
  }
  for (Triangle triangle : mesh.triangles) {
    if (!valid_triangle(mesh, triangle)) {
      continue;
    }
    const Vec3 a = mesh.vertices[triangle.a].position;
    const Vec3 b = mesh.vertices[triangle.b].position;
    const Vec3 c = mesh.vertices[triangle.c].position;
    const Vec3 normal = cross(b - a, c - a);
    mesh.vertices[triangle.a].normal = mesh.vertices[triangle.a].normal + normal;
    mesh.vertices[triangle.b].normal = mesh.vertices[triangle.b].normal + normal;
    mesh.vertices[triangle.c].normal = mesh.vertices[triangle.c].normal + normal;
  }
  for (Vertex& vertex : mesh.vertices) {
    vertex.normal = normalize(vertex.normal);
  }
}

void recalculate_tangents(MeshAsset& mesh) {
  for (Vertex& vertex : mesh.vertices) {
    vertex.tangent = {};
  }
  for (Triangle triangle : mesh.triangles) {
    if (!valid_triangle(mesh, triangle)) {
      continue;
    }
    Vertex& v0 = mesh.vertices[triangle.a];
    Vertex& v1 = mesh.vertices[triangle.b];
    Vertex& v2 = mesh.vertices[triangle.c];
    const Vec3 edge1 = v1.position - v0.position;
    const Vec3 edge2 = v2.position - v0.position;
    const float du1 = v1.texcoord.x - v0.texcoord.x;
    const float dv1 = v1.texcoord.y - v0.texcoord.y;
    const float du2 = v2.texcoord.x - v0.texcoord.x;
    const float dv2 = v2.texcoord.y - v0.texcoord.y;
    const float determinant = du1 * dv2 - du2 * dv1;
    if (std::fabs(determinant) <= 0.000001f) {
      continue;
    }
    const float inv = 1.0f / determinant;
    const Vec3 tangent = (edge1 * dv2 - edge2 * dv1) * inv;
    v0.tangent = v0.tangent + tangent;
    v1.tangent = v1.tangent + tangent;
    v2.tangent = v2.tangent + tangent;
  }
  for (Vertex& vertex : mesh.vertices) {
    vertex.tangent = normalize(vertex.tangent);
    if (length(vertex.tangent) <= 0.000001f) {
      vertex.tangent = {1.0f, 0.0f, 0.0f};
    }
  }
}

void transform_mesh(MeshAsset& mesh, const Mat4& transform) {
  for (Vertex& vertex : mesh.vertices) {
    vertex.position = transform_point(transform, vertex.position);
    vertex.normal = normalize(transform_vector(transform, vertex.normal));
    vertex.tangent = normalize(transform_vector(transform, vertex.tangent));
  }
}

void append_mesh(MeshAsset& target, const MeshAsset& source) {
  const std::uint32_t base = static_cast<std::uint32_t>(target.vertices.size());
  target.vertices.insert(target.vertices.end(), source.vertices.begin(), source.vertices.end());
  for (Triangle triangle : source.triangles) {
    target.triangles.push_back({triangle.a + base, triangle.b + base, triangle.c + base});
  }
}

MeshAsset make_plane_mesh(std::string name, float width, float depth, std::uint32_t material) {
  const float hx = width * 0.5f;
  const float hz = depth * 0.5f;
  MeshAsset mesh;
  mesh.name = std::move(name);
  mesh.material = material;
  mesh.vertices = {
      vertex({-hx, 0.0f, -hz}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}),
      vertex({hx, 0.0f, -hz}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}),
      vertex({hx, 0.0f, hz}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),
      vertex({-hx, 0.0f, hz}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}),
  };
  mesh.triangles = {{0, 1, 2}, {0, 2, 3}};
  recalculate_tangents(mesh);
  return mesh;
}

MeshAsset make_box_mesh(std::string name, Vec3 size, std::uint32_t material) {
  MeshAsset mesh;
  mesh.name = std::move(name);
  mesh.material = material;
  const float hx = size.x * 0.5f;
  const float hy = size.y * 0.5f;
  const float hz = size.z * 0.5f;
  const std::array<Vec3, 8> p{{
      {-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {-hx, hy, -hz},
      {-hx, -hy, hz},  {hx, -hy, hz},  {hx, hy, hz},  {-hx, hy, hz},
  }};
  const auto add_face = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal) {
    const std::uint32_t base = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(vertex(a, normal, {0.0f, 0.0f}));
    mesh.vertices.push_back(vertex(b, normal, {1.0f, 0.0f}));
    mesh.vertices.push_back(vertex(c, normal, {1.0f, 1.0f}));
    mesh.vertices.push_back(vertex(d, normal, {0.0f, 1.0f}));
    mesh.triangles.push_back({base, base + 1, base + 2});
    mesh.triangles.push_back({base, base + 2, base + 3});
  };
  add_face(p[0], p[1], p[2], p[3], {0.0f, 0.0f, -1.0f});
  add_face(p[5], p[4], p[7], p[6], {0.0f, 0.0f, 1.0f});
  add_face(p[4], p[0], p[3], p[7], {-1.0f, 0.0f, 0.0f});
  add_face(p[1], p[5], p[6], p[2], {1.0f, 0.0f, 0.0f});
  add_face(p[3], p[2], p[6], p[7], {0.0f, 1.0f, 0.0f});
  add_face(p[4], p[5], p[1], p[0], {0.0f, -1.0f, 0.0f});
  recalculate_tangents(mesh);
  return mesh;
}

MeshAsset make_uv_sphere_mesh(std::string name,
                              float radius,
                              std::uint32_t rings,
                              std::uint32_t segments,
                              std::uint32_t material) {
  MeshAsset mesh;
  mesh.name = std::move(name);
  mesh.material = material;
  rings = std::max<std::uint32_t>(rings, 2);
  segments = std::max<std::uint32_t>(segments, 3);
  for (std::uint32_t ring = 0; ring <= rings; ++ring) {
    const float v = static_cast<float>(ring) / static_cast<float>(rings);
    const float phi = v * static_cast<float>(M_PI);
    for (std::uint32_t segment = 0; segment <= segments; ++segment) {
      const float u = static_cast<float>(segment) / static_cast<float>(segments);
      const float theta = u * static_cast<float>(M_PI) * 2.0f;
      Vec3 normal{std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
      mesh.vertices.push_back(vertex(normal * radius, normal, {u, v}));
    }
  }
  const std::uint32_t stride = segments + 1;
  for (std::uint32_t ring = 0; ring < rings; ++ring) {
    for (std::uint32_t segment = 0; segment < segments; ++segment) {
      const std::uint32_t a = ring * stride + segment;
      const std::uint32_t b = a + 1;
      const std::uint32_t c = a + stride;
      const std::uint32_t d = c + 1;
      if (ring != 0) {
        mesh.triangles.push_back({a, c, b});
      }
      if (ring + 1 != rings) {
        mesh.triangles.push_back({b, c, d});
      }
    }
  }
  recalculate_tangents(mesh);
  return mesh;
}

MeshAsset weld_vertices(const MeshAsset& mesh, float epsilon) {
  MeshAsset out;
  out.name = mesh.name;
  out.material = mesh.material;
  std::unordered_map<WeldKey, std::uint32_t, WeldHash> remap;
  std::vector<std::uint32_t> indices(mesh.vertices.size(), 0);
  for (std::uint32_t i = 0; i < mesh.vertices.size(); ++i) {
    const WeldKey key = weld_key(mesh.vertices[i].position, epsilon);
    const auto found = remap.find(key);
    if (found != remap.end()) {
      indices[i] = found->second;
    } else {
      indices[i] = static_cast<std::uint32_t>(out.vertices.size());
      remap.emplace(key, indices[i]);
      out.vertices.push_back(mesh.vertices[i]);
    }
  }
  for (Triangle triangle : mesh.triangles) {
    if (!valid_triangle(mesh, triangle)) {
      continue;
    }
    Triangle remapped{indices[triangle.a], indices[triangle.b], indices[triangle.c]};
    if (!degenerate_triangle(out, remapped)) {
      out.triangles.push_back(remapped);
    }
  }
  recalculate_normals(out);
  recalculate_tangents(out);
  return out;
}

}  // namespace knotwork
