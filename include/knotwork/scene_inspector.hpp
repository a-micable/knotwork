#pragma once

#include "knotwork/asset_library.hpp"
#include "knotwork/diagnostics.hpp"
#include "knotwork/graph.hpp"

#include <string>
#include <vector>

namespace knotwork {

struct SceneInspection {
  GraphStats graph{};
  AssetLibraryStats assets{};
  std::vector<Diagnostic> diagnostics;
  std::vector<std::string> notes;
};

[[nodiscard]] SceneInspection inspect_scene(const Scene& scene, const AssetLibrary& assets);
[[nodiscard]] std::string inspection_to_text(const Scene& scene, const SceneInspection& inspection);
[[nodiscard]] std::vector<std::string> missing_asset_notes(const Scene& scene, const AssetLibrary& assets);
[[nodiscard]] std::vector<std::string> material_usage_notes(const Scene& scene);

}  // namespace knotwork
