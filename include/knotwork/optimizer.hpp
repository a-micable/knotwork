#pragma once

#include "knotwork/asset_library.hpp"

#include <string>
#include <vector>

namespace knotwork {

struct OptimizationLog {
  std::vector<std::string> messages;
  std::uint32_t removed_nodes = 0;
  std::uint32_t removed_strings = 0;
  std::uint32_t removed_materials = 0;
  std::uint32_t welded_vertices = 0;
};

struct SceneOptimizationOptions {
  bool remove_empty_leaf_nodes = true;
  bool remove_unused_materials = true;
  bool compact_strings = true;
};

struct AssetOptimizationOptions {
  bool weld_vertices = true;
  bool recalculate_normals = true;
  bool recalculate_tangents = true;
  float weld_epsilon = 0.0001f;
};

[[nodiscard]] Scene optimize_scene(const Scene& scene,
                                   SceneOptimizationOptions options,
                                   OptimizationLog& log);
void optimize_assets(AssetLibrary& library, AssetOptimizationOptions options, OptimizationLog& log);
[[nodiscard]] std::vector<std::uint32_t> unused_materials(const Scene& scene);
[[nodiscard]] std::vector<std::uint32_t> unused_strings(const Scene& scene);
[[nodiscard]] bool removable_empty_leaf(const Scene& scene, std::uint32_t node);

}  // namespace knotwork
