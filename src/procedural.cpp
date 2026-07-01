#include "knotwork/procedural.hpp"

#include <cmath>

namespace knotwork {
namespace {

Vertex make_vertex(Vec3 position, Vec3 normal, Vec2 uv) {
  Vertex vertex;
  vertex.position = position;
  vertex.normal = normal;
  vertex.texcoord = uv;
  return vertex;
}

}  // namespace

MeshAsset make_cylinder_mesh(std::string name, CylinderOptions options, std::uint32_t material) {
  MeshAsset mesh;
  mesh.name = std::move(name);
  mesh.material = material;
  options.segments = std::max<std::uint32_t>(options.segments, 3);
  const float half = options.height * 0.5f;
  for (std::uint32_t i = 0; i <= options.segments; ++i) {
    const float u = static_cast<float>(i) / static_cast<float>(options.segments);
    const float angle = u * 6.28318530718f;
    const Vec3 normal{std::cos(angle), 0.0f, std::sin(angle)};
    const Vec3 bottom = normal * options.radius + Vec3{0.0f, -half, 0.0f};
    const Vec3 top = normal * options.radius + Vec3{0.0f, half, 0.0f};
    mesh.vertices.push_back(make_vertex(bottom, normal, {u, 0.0f}));
    mesh.vertices.push_back(make_vertex(top, normal, {u, 1.0f}));
  }
  for (std::uint32_t i = 0; i < options.segments; ++i) {
    const std::uint32_t a = i * 2;
    const std::uint32_t b = a + 1;
    const std::uint32_t c = a + 2;
    const std::uint32_t d = a + 3;
    mesh.triangles.push_back({a, c, b});
    mesh.triangles.push_back({b, c, d});
  }
  if (options.caps) {
    const std::uint32_t bottom_center = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(make_vertex({0.0f, -half, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f}));
    const std::uint32_t top_center = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(make_vertex({0.0f, half, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}));
    for (std::uint32_t i = 0; i < options.segments; ++i) {
      const std::uint32_t a = i * 2;
      const std::uint32_t b = ((i + 1) % options.segments) * 2;
      mesh.triangles.push_back({bottom_center, b, a});
      mesh.triangles.push_back({top_center, a + 1, b + 1});
    }
  }
  recalculate_tangents(mesh);
  return mesh;
}

MeshAsset make_cone_mesh(std::string name, ConeOptions options, std::uint32_t material) {
  MeshAsset mesh;
  mesh.name = std::move(name);
  mesh.material = material;
  options.segments = std::max<std::uint32_t>(options.segments, 3);
  const float half = options.height * 0.5f;
  const std::uint32_t tip = 0;
  mesh.vertices.push_back(make_vertex({0.0f, half, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}));
  for (std::uint32_t i = 0; i <= options.segments; ++i) {
    const float u = static_cast<float>(i) / static_cast<float>(options.segments);
    const float angle = u * 6.28318530718f;
    Vec3 radial{std::cos(angle), 0.0f, std::sin(angle)};
    Vec3 position = radial * options.radius + Vec3{0.0f, -half, 0.0f};
    Vec3 normal = normalize({radial.x, options.radius / options.height, radial.z});
    mesh.vertices.push_back(make_vertex(position, normal, {u, 0.0f}));
  }
  for (std::uint32_t i = 1; i <= options.segments; ++i) {
    mesh.triangles.push_back({tip, i, i + 1});
  }
  if (options.cap) {
    const std::uint32_t center = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(make_vertex({0.0f, -half, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f}));
    for (std::uint32_t i = 1; i <= options.segments; ++i) {
      mesh.triangles.push_back({center, i + 1, i});
    }
  }
  recalculate_normals(mesh);
  recalculate_tangents(mesh);
  return mesh;
}

MeshAsset make_grid_mesh(std::string name, GridOptions options, std::uint32_t material) {
  MeshAsset mesh;
  mesh.name = std::move(name);
  mesh.material = material;
  options.columns = std::max<std::uint32_t>(options.columns, 1);
  options.rows = std::max<std::uint32_t>(options.rows, 1);
  for (std::uint32_t row = 0; row <= options.rows; ++row) {
    const float v = static_cast<float>(row) / static_cast<float>(options.rows);
    for (std::uint32_t column = 0; column <= options.columns; ++column) {
      const float u = static_cast<float>(column) / static_cast<float>(options.columns);
      const float x = (u - 0.5f) * options.width;
      const float z = (v - 0.5f) * options.depth;
      mesh.vertices.push_back(make_vertex({x, 0.0f, z}, {0.0f, 1.0f, 0.0f}, {u, v}));
    }
  }
  const std::uint32_t stride = options.columns + 1;
  for (std::uint32_t row = 0; row < options.rows; ++row) {
    for (std::uint32_t column = 0; column < options.columns; ++column) {
      const std::uint32_t a = row * stride + column;
      const std::uint32_t b = a + 1;
      const std::uint32_t c = a + stride;
      const std::uint32_t d = c + 1;
      mesh.triangles.push_back({a, b, d});
      mesh.triangles.push_back({a, d, c});
    }
  }
  recalculate_tangents(mesh);
  return mesh;
}

MeshAsset make_axes_mesh(std::string name, float length) {
  MeshAsset mesh;
  mesh.name = std::move(name);
  mesh.vertices = {
      make_vertex({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}),
      make_vertex({length, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}),
      make_vertex({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}),
      make_vertex({0.0f, length, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}),
      make_vertex({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}),
      make_vertex({0.0f, 0.0f, length}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}),
  };
  return mesh;
}

MeshAsset make_wire_cube_mesh(std::string name, Vec3 size) {
  MeshAsset mesh = make_box_mesh(std::move(name), size);
  mesh.triangles.clear();
  return mesh;
}

}  // namespace knotwork
