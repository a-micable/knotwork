#pragma once

#include "knotwork/mesh.hpp"

namespace knotwork {

struct CylinderOptions {
  float radius = 1.0f;
  float height = 1.0f;
  std::uint32_t segments = 32;
  bool caps = true;
};

struct GridOptions {
  float width = 10.0f;
  float depth = 10.0f;
  std::uint32_t columns = 10;
  std::uint32_t rows = 10;
};

struct ConeOptions {
  float radius = 1.0f;
  float height = 1.0f;
  std::uint32_t segments = 32;
  bool cap = true;
};

[[nodiscard]] MeshAsset make_cylinder_mesh(std::string name,
                                           CylinderOptions options,
                                           std::uint32_t material = kNoIndex);
[[nodiscard]] MeshAsset make_cone_mesh(std::string name,
                                       ConeOptions options,
                                       std::uint32_t material = kNoIndex);
[[nodiscard]] MeshAsset make_grid_mesh(std::string name,
                                       GridOptions options,
                                       std::uint32_t material = kNoIndex);
[[nodiscard]] MeshAsset make_axes_mesh(std::string name, float length = 1.0f);
[[nodiscard]] MeshAsset make_wire_cube_mesh(std::string name, Vec3 size);

}  // namespace knotwork
