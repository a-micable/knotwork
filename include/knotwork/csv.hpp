#pragma once

#include "knotwork/project.hpp"

#include <string>

namespace knotwork {

[[nodiscard]] std::string scene_nodes_csv(const Scene& scene);
[[nodiscard]] std::string materials_csv(const Scene& scene);
[[nodiscard]] std::string assets_csv(const AssetLibrary& assets);
[[nodiscard]] std::string diagnostics_csv(const Scene& scene, std::span<const Diagnostic> diagnostics);
[[nodiscard]] std::string project_files_csv(const Project& project);

}  // namespace knotwork
