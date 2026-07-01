#include "knotwork/asset_library.hpp"
#include "knotwork/format.hpp"
#include "knotwork/report.hpp"
#include "knotwork/scene_audit.hpp"
#include "knotwork/scene_inspector.hpp"
#include "knotwork/statistics.hpp"

#include <filesystem>
#include <iostream>

namespace {

int usage() {
  std::cerr << "usage: knotwork-doctor <scene.kwscene> [--markdown]\n";
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    return usage();
  }
  const bool markdown = argc == 3 && std::string(argv[2]) == "--markdown";
  auto loaded = knotwork::load_scene_file(std::filesystem::path(argv[1]));
  if (!loaded.ok()) {
    std::cerr << "load failed: " << knotwork::scene_error_name(loaded.error().code) << ": "
              << loaded.error().message << "\n";
    return 1;
  }
  knotwork::AssetLibrary assets;
  if (markdown) {
    std::cout << knotwork::scene_markdown_report(loaded.value(), assets);
    return 0;
  }
  knotwork::SceneInspection inspection = knotwork::inspect_scene(loaded.value(), assets);
  knotwork::SceneStatistics scene_stats = knotwork::collect_scene_statistics(loaded.value());
  knotwork::AssetStatistics asset_stats = knotwork::collect_asset_statistics(assets);
  std::cout << knotwork::inspection_to_text(loaded.value(), inspection);
  std::cout << knotwork::statistics_text(scene_stats, asset_stats);
  auto findings = knotwork::audit_scene(loaded.value(), assets);
  if (!findings.empty()) {
    std::cout << "Audit\n" << knotwork::audit_to_text(loaded.value(), findings);
  }
  for (const knotwork::AuditFinding& finding : findings) {
    if (finding.severity == knotwork::Severity::Error) {
      return 1;
    }
  }
  return 0;
}
