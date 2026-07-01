#pragma once

#include "knotwork/bounds.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace knotwork {

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;
};

struct Vertex {
  Vec3 position{};
  Vec3 normal{};
  Vec2 texcoord{};
  Vec3 tangent{1.0f, 0.0f, 0.0f};
};

struct Triangle {
  std::uint32_t a = 0;
  std::uint32_t b = 0;
  std::uint32_t c = 0;
};

struct MeshAsset {
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;
  std::uint32_t material = kNoIndex;
};

struct MeshStats {
  std::uint32_t vertex_count = 0;
  std::uint32_t triangle_count = 0;
  std::uint32_t degenerate_count = 0;
  float surface_area = 0.0f;
  std::optional<Aabb> bounds;
};

struct MeshError {
  std::string message;
  std::uint32_t triangle = kNoIndex;
};

[[nodiscard]] Vec3 operator+(Vec3 lhs, Vec3 rhs);
[[nodiscard]] Vec3 operator-(Vec3 lhs, Vec3 rhs);
[[nodiscard]] Vec3 operator*(Vec3 lhs, float rhs);
[[nodiscard]] Vec3 operator/(Vec3 lhs, float rhs);
[[nodiscard]] float dot(Vec3 lhs, Vec3 rhs);
[[nodiscard]] Vec3 cross(Vec3 lhs, Vec3 rhs);
[[nodiscard]] float length(Vec3 value);
[[nodiscard]] Vec3 normalize(Vec3 value);
[[nodiscard]] float triangle_area(Vec3 a, Vec3 b, Vec3 c);
[[nodiscard]] bool valid_triangle(const MeshAsset& mesh, Triangle triangle);
[[nodiscard]] bool degenerate_triangle(const MeshAsset& mesh, Triangle triangle, float epsilon = 0.000001f);
[[nodiscard]] std::optional<Aabb> mesh_bounds(const MeshAsset& mesh);
[[nodiscard]] MeshStats mesh_stats(const MeshAsset& mesh);
[[nodiscard]] std::vector<MeshError> validate_mesh(const MeshAsset& mesh);
void recalculate_normals(MeshAsset& mesh);
void recalculate_tangents(MeshAsset& mesh);
void transform_mesh(MeshAsset& mesh, const Mat4& transform);
void append_mesh(MeshAsset& target, const MeshAsset& source);
[[nodiscard]] MeshAsset make_plane_mesh(std::string name, float width, float depth, std::uint32_t material = kNoIndex);
[[nodiscard]] MeshAsset make_box_mesh(std::string name, Vec3 size, std::uint32_t material = kNoIndex);
[[nodiscard]] MeshAsset make_uv_sphere_mesh(std::string name,
                                            float radius,
                                            std::uint32_t rings,
                                            std::uint32_t segments,
                                            std::uint32_t material = kNoIndex);
[[nodiscard]] MeshAsset weld_vertices(const MeshAsset& mesh, float epsilon);

}  // namespace knotwork
