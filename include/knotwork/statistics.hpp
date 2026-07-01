#pragma once

#include "knotwork/project.hpp"

namespace knotwork {

struct SceneStatistics {
  std::uint32_t string_bytes = 0;
  std::uint32_t node_count = 0;
  std::uint32_t material_count = 0;
  std::uint32_t root_count = 0;
  std::uint32_t max_depth = 0;
  std::uint32_t animated_node_estimate = 0;
};

struct AssetStatistics {
  std::uint32_t mesh_count = 0;
  std::uint32_t vertex_count = 0;
  std::uint32_t triangle_count = 0;
  std::uint32_t degenerate_triangle_count = 0;
  float surface_area = 0.0f;
};

[[nodiscard]] SceneStatistics collect_scene_statistics(const Scene& scene);
[[nodiscard]] AssetStatistics collect_asset_statistics(const AssetLibrary& assets);
[[nodiscard]] std::string statistics_text(const SceneStatistics& scene, const AssetStatistics& assets);

}  // namespace knotwork
