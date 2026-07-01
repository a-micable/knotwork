#pragma once

#include "knotwork/scene_diff.hpp"
#include "knotwork/selection.hpp"

namespace knotwork {

struct MergeOptions {
  bool prefix_imported_names = true;
  std::string prefix = "imported/";
};

struct MergeResult {
  Scene scene;
  std::vector<std::uint32_t> imported_nodes;
  std::vector<std::uint32_t> imported_materials;
};

[[nodiscard]] MergeResult merge_scene(const Scene& base, const Scene& imported, MergeOptions options = {});
[[nodiscard]] Scene isolate_subtree(const Scene& scene, std::uint32_t root);
[[nodiscard]] Scene clone_selection(const Scene& scene, const Selection& selection);

}  // namespace knotwork
