#pragma once

#include "knotwork/project.hpp"

#include <string>

namespace knotwork {

struct ReportOptions {
  bool include_diagnostics = true;
  bool include_assets = true;
  bool include_hierarchy = true;
  bool include_manifest = true;
};

[[nodiscard]] std::string scene_markdown_report(const Scene& scene,
                                                const AssetLibrary& assets,
                                                ReportOptions options = {});
[[nodiscard]] std::string project_markdown_report(const Project& project,
                                                  ReportOptions options = {});

}  // namespace knotwork
