#include "knotwork/flatten.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Failure {
  std::string name;
  std::string message;
};

std::vector<Failure>& failures() {
  static std::vector<Failure> data;
  return data;
}

void require(bool condition, std::string name, std::string message) {
  if (!condition) {
    failures().push_back({std::move(name), std::move(message)});
  }
}

knotwork::Node make_node(std::uint32_t id,
                         std::uint32_t name,
                         knotwork::NodeKind kind,
                         std::uint32_t parent = knotwork::kNoIndex) {
  knotwork::Node node;
  node.id = id;
  node.name = name;
  node.kind = kind;
  node.parent = parent;
  return node;
}

knotwork::Scene sample_scene() {
  knotwork::Scene scene;
  scene.strings = {"root", "mesh", "copy", "material"};
  scene.materials.push_back({3, {1.0f, 0.5f, 0.25f}, 0.4f, 0.1f});
  scene.nodes.push_back(make_node(1, 0, knotwork::NodeKind::Empty));
  scene.nodes.push_back(make_node(2, 1, knotwork::NodeKind::Mesh, 0));
  scene.nodes.back().material = 0;
  scene.nodes.back().local.translation = {1.0f, 2.0f, 3.0f};
  scene.nodes.push_back(make_node(3, 2, knotwork::NodeKind::Instance, 0));
  scene.nodes.back().instance_target = 1;
  scene.nodes.back().local.translation = {10.0f, 0.0f, 0.0f};
  return scene;
}

void round_trip_preserves_tables() {
  const std::string test = "round_trip_preserves_tables";
  knotwork::Scene scene = sample_scene();
  const auto bytes = knotwork::save_scene(scene);
  auto loaded = knotwork::load_scene(bytes);
  require(loaded.ok(), test, loaded.ok() ? "" : loaded.error().message);
  if (!loaded.ok()) {
    return;
  }
  require(loaded.value().strings == scene.strings, test, "string table changed");
  require(loaded.value().materials.size() == scene.materials.size(), test, "material table changed");
  require(loaded.value().nodes.size() == scene.nodes.size(), test, "node table changed");
}

void flatten_composes_parent_transform() {
  const std::string test = "flatten_composes_parent_transform";
  knotwork::Scene scene = sample_scene();
  scene.nodes[0].local.translation = {5.0f, 0.0f, 0.0f};
  auto flattened = knotwork::flatten_scene(scene);
  require(flattened.ok(), test, flattened.ok() ? "" : flattened.error().message);
  if (!flattened.ok()) {
    return;
  }
  require(knotwork::nearly_equal(flattened.value()[1].world.at(0, 3), 6.0f), test,
          "child x translation did not include parent");
  require(knotwork::nearly_equal(flattened.value()[1].world.at(1, 3), 2.0f), test,
          "child y translation changed unexpectedly");
}

void flatten_applies_instance_target() {
  const std::string test = "flatten_applies_instance_target";
  auto flattened = knotwork::flatten_scene(sample_scene());
  require(flattened.ok(), test, flattened.ok() ? "" : flattened.error().message);
  if (!flattened.ok()) {
    return;
  }
  require(knotwork::nearly_equal(flattened.value()[2].world.at(0, 3), 11.0f), test,
          "instance did not combine placement and target transform");
  require(knotwork::nearly_equal(flattened.value()[2].world.at(1, 3), 2.0f), test,
          "instance y translation did not inherit target");
}

void parent_cycle_is_rejected() {
  const std::string test = "parent_cycle_is_rejected";
  knotwork::Scene scene = sample_scene();
  scene.nodes[0].parent = 2;
  auto loaded = knotwork::load_scene(knotwork::save_scene(scene));
  require(!loaded.ok(), test, "cyclic parent graph loaded successfully");
  if (!loaded.ok()) {
    require(loaded.error().code == knotwork::SceneErrorCode::Cycle, test,
            "cycle produced wrong error code");
  }
}

void instance_cycle_is_rejected() {
  const std::string test = "instance_cycle_is_rejected";
  knotwork::Scene scene;
  scene.strings = {"a", "b"};
  scene.nodes.push_back(make_node(1, 0, knotwork::NodeKind::Instance));
  scene.nodes.push_back(make_node(2, 1, knotwork::NodeKind::Instance));
  scene.nodes[0].instance_target = 1;
  scene.nodes[1].instance_target = 0;
  auto loaded = knotwork::load_scene(knotwork::save_scene(scene));
  require(!loaded.ok(), test, "cyclic instance graph loaded successfully");
  if (!loaded.ok()) {
    require(loaded.error().code == knotwork::SceneErrorCode::Cycle, test,
            "cycle produced wrong error code");
  }
}

void malformed_bytes_do_not_load() {
  const std::string test = "malformed_bytes_do_not_load";
  const std::vector<std::byte> bytes{std::byte{'K'}, std::byte{'N'}, std::byte{'O'}};
  auto loaded = knotwork::load_scene(bytes);
  require(!loaded.ok(), test, "short input loaded successfully");
}

}  // namespace

int main() {
  round_trip_preserves_tables();
  flatten_composes_parent_transform();
  flatten_applies_instance_target();
  parent_cycle_is_rejected();
  instance_cycle_is_rejected();
  malformed_bytes_do_not_load();

  if (!failures().empty()) {
    for (const Failure& failure : failures()) {
      std::cerr << failure.name << ": " << failure.message << "\n";
    }
    return EXIT_FAILURE;
  }
  std::cout << "all tests passed\n";
  return EXIT_SUCCESS;
}
