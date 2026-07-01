#include "knotwork/asset_manifest.hpp"
#include "knotwork/debug_dump.hpp"
#include "knotwork/format.hpp"

#include <filesystem>
#include <iostream>

namespace {

int usage() {
  std::cerr << "usage: knotwork-debug <scene.kwscene> [--matrices]\n";
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    return usage();
  }
  knotwork::DumpOptions options;
  options.include_matrices = argc == 3 && std::string(argv[2]) == "--matrices";
  auto scene = knotwork::load_scene_file(std::filesystem::path(argv[1]));
  if (!scene.ok()) {
    std::cerr << scene.error().message << "\n";
    return 1;
  }
  knotwork::Project project = knotwork::make_project("debug", std::filesystem::current_path());
  project.scene = scene.value();
  std::cout << knotwork::dump_project(project, options);
  std::cout << knotwork::asset_manifest_text(knotwork::build_asset_manifest(project.assets));
  return 0;
}
