#include "knotwork/format.hpp"

#include <filesystem>
#include <iostream>

namespace {

knotwork::Node node(std::uint32_t id,
                    std::uint32_t name,
                    knotwork::NodeKind kind,
                    std::uint32_t parent = knotwork::kNoIndex) {
  knotwork::Node out;
  out.id = id;
  out.name = name;
  out.kind = kind;
  out.parent = parent;
  return out;
}

knotwork::Scene basic_scene() {
  knotwork::Scene scene;
  scene.strings = {"root", "body", "camera", "matte"};
  scene.materials.push_back({3, {0.8f, 0.7f, 0.6f}, 0.9f, 0.0f});
  scene.nodes.push_back(node(10, 0, knotwork::NodeKind::Empty));
  scene.nodes.push_back(node(20, 1, knotwork::NodeKind::Mesh, 0));
  scene.nodes.back().material = 0;
  scene.nodes.back().local.translation = {2.0f, 0.0f, 0.0f};
  scene.nodes.push_back(node(30, 2, knotwork::NodeKind::Camera, 0));
  scene.nodes.back().local.translation = {0.0f, 1.5f, 4.0f};
  scene.nodes.back().camera_fov_y = 45.0f;
  return scene;
}

knotwork::Scene instanced_scene() {
  knotwork::Scene scene = basic_scene();
  scene.strings.push_back("body-copy");
  scene.nodes.push_back(node(40, 4, knotwork::NodeKind::Instance, 0));
  scene.nodes.back().instance_target = 1;
  scene.nodes.back().local.translation = {-2.0f, 0.0f, 0.0f};
  return scene;
}

knotwork::Scene lit_scene() {
  knotwork::Scene scene = instanced_scene();
  scene.strings.push_back("key-light");
  scene.nodes.push_back(node(50, 5, knotwork::NodeKind::Light, 0));
  scene.nodes.back().local.translation = {0.0f, 3.0f, 1.0f};
  scene.nodes.back().light_kind = knotwork::LightKind::Spot;
  scene.nodes.back().light_intensity = 12.0f;
  return scene;
}

int usage() {
  std::cerr << "usage: knotwork-seed <output.kwscene> [basic|instanced|lit]\n";
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    return usage();
  }
  const std::string kind = argc == 3 ? argv[2] : "basic";
  knotwork::Scene scene;
  if (kind == "basic") {
    scene = basic_scene();
  } else if (kind == "instanced") {
    scene = instanced_scene();
  } else if (kind == "lit") {
    scene = lit_scene();
  } else {
    return usage();
  }

  knotwork::SceneError error;
  if (!knotwork::save_scene_file(std::filesystem::path(argv[1]), scene, error)) {
    std::cerr << "write failed: " << knotwork::scene_error_name(error.code) << ": "
              << error.message << "\n";
    return 1;
  }
  return 0;
}
