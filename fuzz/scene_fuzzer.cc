#include "knotwork/flatten.hpp"
#include "knotwork/import_pipeline.hpp"
#include "knotwork/builder.hpp"
#include "knotwork/editor_state.hpp"
#include "knotwork/mesh_io.hpp"
#include "knotwork/package.hpp"
#include "knotwork/patch.hpp"
#include "knotwork/project.hpp"
#include "knotwork/spatial_index.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <vector>

namespace {

std::uint8_t byte_at(std::span<const std::byte> bytes, std::size_t index) {
  if (index >= bytes.size()) {
    return 0;
  }
  return static_cast<std::uint8_t>(bytes[index]);
}

float fuzz_unit(std::uint8_t value) {
  return (static_cast<float>(value) / 255.0f) * 4.0f - 2.0f;
}

knotwork::Scene fallback_scene() {
  knotwork::SceneBuilder builder;
  const std::uint32_t material = builder.add_material("matte");
  const std::uint32_t root = builder.add_node("root", knotwork::NodeKind::Empty);
  builder.add_mesh("mesh", root, material, knotwork::translated(1.0f, 0.0f, 0.0f));
  builder.add_instance("copy", root, 1, knotwork::translated(0.0f, 1.0f, 0.0f));
  return builder.take();
}

void fuzz_scene_bytes(std::span<const std::byte> bytes) {
  auto scene = knotwork::load_scene(bytes);
  if (!scene.ok()) {
    return;
  }
  auto flattened = knotwork::flatten_scene(scene.value());
  if (!flattened.ok()) {
    return;
  }
  auto bounds = knotwork::default_unit_mesh_bounds(scene.value());
  volatile std::size_t sink = flattened.value().size() + bounds.size();
  (void)sink;
}

void fuzz_mesh_bytes(std::span<const std::byte> bytes) {
  auto mesh = knotwork::load_mesh_binary(bytes);
  if (!mesh.ok()) {
    return;
  }
  knotwork::MeshBvh bvh = knotwork::build_mesh_bvh(mesh.value());
  knotwork::PickRequest request;
  request.ray.origin = {0.0f, 0.0f, 10.0f};
  request.ray.direction = {0.0f, 0.0f, -1.0f};
  auto hits = knotwork::raycast_bvh(mesh.value(), bvh, knotwork::Mat4::identity(), request);
  volatile std::size_t sink = hits.size() + bvh.nodes.size();
  (void)sink;
}

void fuzz_package_bytes(std::span<const std::byte> bytes) {
  auto package = knotwork::load_package(bytes);
  if (!package.ok()) {
    return;
  }
  knotwork::Project project = knotwork::package_to_project(package.value(), "fuzz");
  auto inspection = knotwork::inspect_project(project);
  volatile std::size_t sink = inspection.diagnostics.size() + inspection.notes.size();
  (void)sink;
}

void fuzz_editor_replay_bytes(std::span<const std::byte> bytes) {
  knotwork::Project project = knotwork::make_project("fuzz-editor", {});
  auto package = knotwork::load_package(bytes);
  if (package.ok()) {
    project = knotwork::package_to_project(package.value(), "fuzz-editor");
  } else {
    project.scene = fallback_scene();
  }

  knotwork::EditorState editor(std::move(project));
  if (editor.project().scene.nodes.empty()) {
    editor.set_scene(fallback_scene());
  }

  const std::size_t node_count = editor.project().scene.nodes.size();
  const std::uint32_t target = static_cast<std::uint32_t>(byte_at(bytes, 0) % node_count);
  editor.select_node(target);
  if (node_count > 1) {
    editor.select_node(static_cast<std::uint32_t>(byte_at(bytes, 1) % node_count));
  }

  knotwork::Transform transform = knotwork::translated(fuzz_unit(byte_at(bytes, 2)),
                                                       fuzz_unit(byte_at(bytes, 3)),
                                                       fuzz_unit(byte_at(bytes, 4)));
  bool replay_ok = editor.transform_node(target, transform);
  replay_ok = editor.undo() || replay_ok;
  replay_ok = editor.redo() || replay_ok;

  knotwork::ScenePatch patch;
  patch.name = "fuzz-replay";
  knotwork::PatchOp rename;
  rename.kind = knotwork::PatchOpKind::RenameString;
  rename.index = byte_at(bytes, 5);
  rename.text = "fuzz-name";
  patch.ops.push_back(rename);
  knotwork::PatchOp reparent;
  reparent.kind = knotwork::PatchOpKind::ReparentNode;
  reparent.index = target;
  reparent.other = node_count > 1 ? static_cast<std::uint32_t>(byte_at(bytes, 6) % node_count)
                                  : knotwork::kNoIndex;
  patch.ops.push_back(reparent);
  replay_ok = editor.apply(std::move(patch), "fuzz-structured-patch") || replay_ok;

  knotwork::Scene replacement = fallback_scene();
  replacement.nodes.front().local.translation.x = fuzz_unit(byte_at(bytes, 7));
  editor.set_scene(std::move(replacement));
  replay_ok = editor.undo() || replay_ok;
  replay_ok = editor.redo() || replay_ok;
  auto inspection = editor.inspect();
  volatile std::size_t sink = editor.status_text().size() + inspection.diagnostics.size() +
                              (replay_ok ? 1U : 0U);
  (void)sink;
}

void fuzz_import_bytes(std::span<const std::byte> bytes) {
  knotwork::Project project = knotwork::make_project("fuzz-import", {});
  knotwork::ImportRequest request = knotwork::scene_binary_import("scene", std::vector<std::byte>(bytes.begin(), bytes.end()));
  knotwork::ImportResult result = knotwork::import_into_project(project, request);
  if (knotwork::import_failed(result)) {
    request = knotwork::package_import("package", std::vector<std::byte>(bytes.begin(), bytes.end()));
    result = knotwork::import_into_project(project, request);
  }
  volatile std::size_t sink = result.imported_meshes + result.imported_nodes + result.errors.size();
  (void)sink;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  const auto* bytes = reinterpret_cast<const std::byte*>(data);
  std::span<const std::byte> input(bytes, size);
  fuzz_scene_bytes(input);
  fuzz_mesh_bytes(input);
  fuzz_package_bytes(input);
  fuzz_editor_replay_bytes(input);
  fuzz_import_bytes(input);
  return 0;
}

#ifdef KNOTWORK_STANDALONE_FUZZER
namespace {

std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    return {};
  }
  const std::streamsize size = input.tellg();
  input.seekg(0, std::ios::beg);
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  input.read(reinterpret_cast<char*>(bytes.data()), size);
  return bytes;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: scene_fuzzer <scene-or-corpus-path>...\n";
    return 2;
  }
  for (int i = 1; i < argc; ++i) {
    const std::filesystem::path path(argv[i]);
    if (std::filesystem::is_directory(path)) {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (!entry.is_regular_file()) {
          continue;
        }
        const auto bytes = read_file(entry.path());
        LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
      }
    } else {
      const auto bytes = read_file(path);
      LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
    }
  }
  return 0;
}
#endif
