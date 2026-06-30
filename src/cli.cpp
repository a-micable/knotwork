#include "knotwork/flatten.hpp"

#include <filesystem>
#include <iostream>

namespace {

int usage() {
  std::cerr << "usage: knotwork-info <scene.kwscene>\n";
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    return usage();
  }

  auto loaded = knotwork::load_scene_file(std::filesystem::path(argv[1]));
  if (!loaded.ok()) {
    std::cerr << "load failed: " << knotwork::scene_error_name(loaded.error().code) << ": "
              << loaded.error().message << "\n";
    return 1;
  }

  auto flattened = knotwork::flatten_scene(loaded.value());
  if (!flattened.ok()) {
    std::cerr << "flatten failed: " << knotwork::scene_error_name(flattened.error().code) << ": "
              << flattened.error().message << "\n";
    return 1;
  }

  const knotwork::Scene& scene = loaded.value();
  std::cout << "strings: " << scene.strings.size() << "\n";
  std::cout << "materials: " << scene.materials.size() << "\n";
  std::cout << "nodes: " << scene.nodes.size() << "\n";
  for (std::size_t i = 0; i < scene.nodes.size(); ++i) {
    const knotwork::Node& node = scene.nodes[i];
    const auto name = scene.string_at(node.name).value_or("<unnamed>");
    std::cout << "node " << i << " id=" << node.id << " name=" << name
              << " kind=" << knotwork::node_kind_name(node.kind)
              << " world=" << flattened.value()[i].world << "\n";
  }
  return 0;
}
