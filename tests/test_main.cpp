#include "knotwork/asset_library.hpp"
#include "knotwork/builder.hpp"
#include "knotwork/camera.hpp"
#include "knotwork/checksum.hpp"
#include "knotwork/consistency.hpp"
#include "knotwork/csv.hpp"
#include "knotwork/debug_dump.hpp"
#include "knotwork/editor_state.hpp"
#include "knotwork/flatten.hpp"
#include "knotwork/keyframe_editor.hpp"
#include "knotwork/material.hpp"
#include "knotwork/mesh_io.hpp"
#include "knotwork/obj.hpp"
#include "knotwork/optimizer.hpp"
#include "knotwork/package.hpp"
#include "knotwork/patch.hpp"
#include "knotwork/pathfinding.hpp"
#include "knotwork/project.hpp"
#include "knotwork/raycast.hpp"
#include "knotwork/report.hpp"
#include "knotwork/scene_diff.hpp"
#include "knotwork/scene_inspector.hpp"
#include "knotwork/scene_audit.hpp"
#include "knotwork/scene_layout.hpp"
#include "knotwork/scene_merge.hpp"
#include "knotwork/scene_script.hpp"
#include "knotwork/schema.hpp"
#include "knotwork/selection.hpp"
#include "knotwork/spatial_index.hpp"
#include "knotwork/string_util.hpp"
#include "knotwork/table.hpp"
#include "knotwork/units.hpp"
#include "knotwork/validation_report.hpp"
#include "knotwork/version.hpp"

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

void deep_parent_chain_is_rejected_without_stack_growth() {
  const std::string test = "deep_parent_chain_is_rejected_without_stack_growth";
  knotwork::Scene scene;
  scene.strings = {"node"};
  constexpr std::uint32_t count = 5000;
  scene.nodes.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    scene.nodes.push_back(make_node(i + 1, 0, knotwork::NodeKind::Empty));
    scene.nodes.back().parent = i + 1 < count ? i + 1 : knotwork::kNoIndex;
  }
  auto loaded = knotwork::load_scene(knotwork::save_scene(scene));
  require(!loaded.ok(), test, "deep parent chain loaded successfully");
  if (!loaded.ok()) {
    require(loaded.error().code == knotwork::SceneErrorCode::CountLimitExceeded, test,
            "deep parent chain produced wrong error");
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

void mesh_primitives_have_expected_stats() {
  const std::string test = "mesh_primitives_have_expected_stats";
  knotwork::MeshAsset plane = knotwork::make_plane_mesh("floor", 4.0f, 2.0f);
  knotwork::MeshStats plane_stats = knotwork::mesh_stats(plane);
  require(plane_stats.vertex_count == 4, test, "plane vertex count mismatch");
  require(plane_stats.triangle_count == 2, test, "plane triangle count mismatch");
  require(knotwork::nearly_equal(plane_stats.surface_area, 8.0f), test, "plane surface area mismatch");

  knotwork::MeshAsset box = knotwork::make_box_mesh("box", {2.0f, 2.0f, 2.0f});
  knotwork::MeshStats box_stats = knotwork::mesh_stats(box);
  require(box_stats.vertex_count == 24, test, "box vertex count mismatch");
  require(box_stats.triangle_count == 12, test, "box triangle count mismatch");
  require(box_stats.bounds.has_value(), test, "box bounds missing");
  require(box_stats.bounds && box_stats.bounds->contains({1.0f, 1.0f, 1.0f}), test,
          "box bounds did not contain corner");
}

void obj_round_trip_loads_triangles() {
  const std::string test = "obj_round_trip_loads_triangles";
  knotwork::MeshAsset mesh = knotwork::make_plane_mesh("quad", 2.0f, 2.0f);
  std::string obj = knotwork::save_obj(mesh);
  auto loaded = knotwork::load_obj(obj);
  require(!knotwork::obj_has_errors(loaded), test, "OBJ loader reported issues");
  require(loaded.mesh.vertices.size() == mesh.vertices.size(), test, "OBJ vertex count changed");
  require(loaded.mesh.triangles.size() == mesh.triangles.size(), test, "OBJ triangle count changed");
}

void asset_library_searches_and_summarizes() {
  const std::string test = "asset_library_searches_and_summarizes";
  knotwork::AssetLibrary library;
  auto floor = library.add_mesh(knotwork::make_plane_mesh("floor", 8.0f, 8.0f), "built-in", {"level", "walkable"});
  auto box = library.add_mesh(knotwork::make_box_mesh("crate", {1.0f, 1.0f, 1.0f}), "built-in", {"prop"});
  require(knotwork::valid_handle(floor), test, "floor handle invalid");
  require(knotwork::valid_handle(box), test, "box handle invalid");
  knotwork::MeshSearch search;
  search.tag = "walkable";
  require(library.search(search).size() == 1, test, "tag search mismatch");
  require(library.stats().mesh_count == 2, test, "library mesh count mismatch");
  require(knotwork::summarize_library(library).find("crate") != std::string::npos, test,
          "library summary missing crate");
}

void raycast_hits_mesh_in_scene() {
  const std::string test = "raycast_hits_mesh_in_scene";
  knotwork::AssetLibrary assets;
  auto mesh = assets.add_mesh(knotwork::make_plane_mesh("floor", 4.0f, 4.0f));
  knotwork::SceneBuilder builder;
  std::uint32_t mat = builder.add_material("mesh_ref");
  std::uint32_t root = builder.add_node("root", knotwork::NodeKind::Empty);
  std::uint32_t floor = builder.add_mesh("floor", root, mat);
  knotwork::Scene scene = builder.take();
  scene.nodes[floor].material = mesh.index;

  knotwork::PickRequest request;
  request.ray.origin = {0.0f, 2.0f, 0.0f};
  request.ray.direction = {0.0f, -1.0f, 0.0f};
  auto hits = knotwork::raycast_scene(scene, assets, request);
  require(!hits.empty(), test, "raycast missed floor");
  require(hits.front().node == floor, test, "raycast hit wrong node");
}

void patches_and_diffs_apply_scene_changes() {
  const std::string test = "patches_and_diffs_apply_scene_changes";
  knotwork::Scene scene = sample_scene();
  knotwork::ScenePatch patch = knotwork::transform_patch(1, knotwork::translated(7.0f, 0.0f, 0.0f));
  knotwork::PatchResult patched = knotwork::apply_patch(scene, patch);
  require(!knotwork::patch_has_errors(patched), test, "patch returned errors");
  require(knotwork::nearly_equal(patched.scene.nodes[1].local.translation.x, 7.0f), test,
          "patch did not update transform");
  knotwork::SceneDiff diff = knotwork::diff_scenes(scene, patched.scene);
  require(!knotwork::empty(diff), test, "diff was empty for changed scene");
  require(knotwork::diff_to_text(diff).find("node-changed") != std::string::npos, test,
          "diff text missing node change");
}

void package_round_trip_preserves_entries() {
  const std::string test = "package_round_trip_preserves_entries";
  knotwork::Package package;
  package.entries.push_back(knotwork::make_scene_entry("scene.kwscene", sample_scene()));
  package.entries.push_back(knotwork::make_mesh_obj_entry("floor.obj", knotwork::make_plane_mesh("floor", 1.0f, 1.0f)));
  package.entries.push_back(knotwork::make_text_entry("notes.txt", "hello"));
  auto loaded = knotwork::load_package(knotwork::save_package(package));
  require(loaded.ok(), test, loaded.ok() ? "" : loaded.error().message);
  if (!loaded.ok()) {
    return;
  }
  require(loaded.value().entries.size() == 3, test, "package entry count mismatch");
  require(knotwork::find_entry(loaded.value(), "floor.obj").has_value(), test, "missing mesh entry");
}

void malformed_package_count_is_rejected_before_reserve() {
  const std::string test = "malformed_package_count_is_rejected_before_reserve";
  const std::vector<std::byte> bytes{
      std::byte{'K'},  std::byte{'N'},  std::byte{'O'},  std::byte{'T'},
      std::byte{'P'},  std::byte{'K'},  std::byte{'G'},  std::byte{0},
      std::byte{1},    std::byte{0},    std::byte{0},    std::byte{0},
      std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}};
  auto loaded = knotwork::load_package(bytes);
  require(!loaded.ok(), test, "malformed package count loaded successfully");
  if (!loaded.ok()) {
    require(loaded.error().code == knotwork::SceneErrorCode::CountLimitExceeded, test,
            "malformed package count produced wrong error");
  }
}

void inspection_reports_scene_and_assets() {
  const std::string test = "inspection_reports_scene_and_assets";
  knotwork::AssetLibrary assets;
  const knotwork::MeshHandle crate = assets.add_mesh(knotwork::make_box_mesh("crate", {1.0f, 1.0f, 1.0f}));
  (void)crate;
  knotwork::Scene scene = sample_scene();
  knotwork::SceneInspection inspection = knotwork::inspect_scene(scene, assets);
  require(inspection.graph.mesh_count == 1, test, "inspection mesh node count mismatch");
  require(inspection.assets.mesh_count == 1, test, "inspection asset count mismatch");
  require(knotwork::inspection_to_text(scene, inspection).find("Scene") != std::string::npos, test,
          "inspection text missing header");
}

void mesh_binary_round_trip_preserves_hash() {
  const std::string test = "mesh_binary_round_trip_preserves_hash";
  knotwork::MeshAsset mesh = knotwork::make_box_mesh("crate", {1.0f, 2.0f, 3.0f});
  const std::uint64_t before = knotwork::mesh_content_hash(mesh);
  auto loaded = knotwork::load_mesh_binary(knotwork::save_mesh_binary(mesh));
  require(loaded.ok(), test, loaded.ok() ? "" : loaded.error().message);
  if (!loaded.ok()) {
    return;
  }
  require(knotwork::mesh_content_hash(loaded.value()) == before, test, "mesh hash changed");
}

void malformed_mesh_counts_are_rejected_before_allocation() {
  const std::string test = "malformed_mesh_counts_are_rejected_before_allocation";
  const std::vector<std::byte> bytes{
      std::byte{'K'},  std::byte{'N'},  std::byte{'O'},  std::byte{'T'},
      std::byte{'M'},  std::byte{'S'},  std::byte{'H'},  std::byte{0},
      std::byte{1},    std::byte{0},    std::byte{0},    std::byte{0},
      std::byte{0},    std::byte{0},    std::byte{0},    std::byte{0},
      std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
      std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
      std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}};
  auto loaded = knotwork::load_mesh_binary(bytes);
  require(!loaded.ok(), test, "malformed mesh count loaded successfully");
}

void bvh_raycast_matches_direct_mesh_raycast() {
  const std::string test = "bvh_raycast_matches_direct_mesh_raycast";
  knotwork::MeshAsset mesh = knotwork::make_box_mesh("box", {2.0f, 2.0f, 2.0f});
  knotwork::MeshHandle handle{0};
  knotwork::MeshBvh bvh = knotwork::build_mesh_bvh(mesh, handle);
  require(!bvh.nodes.empty(), test, "BVH had no nodes");
  require(knotwork::bvh_leaf_count(bvh) > 0, test, "BVH had no leaves");
  knotwork::PickRequest request;
  request.ray.origin = {0.0f, 0.0f, 5.0f};
  request.ray.direction = {0.0f, 0.0f, -1.0f};
  auto direct = knotwork::raycast_mesh(mesh, handle, knotwork::Mat4::identity(), request);
  auto accelerated = knotwork::raycast_bvh(mesh, bvh, knotwork::Mat4::identity(), request);
  require(!direct.empty(), test, "direct raycast missed");
  require(!accelerated.empty(), test, "BVH raycast missed");
  require(knotwork::nearly_equal(direct.front().distance, accelerated.front().distance), test,
          "BVH hit distance differed from direct raycast");
}

void optimizer_removes_unused_scene_data() {
  const std::string test = "optimizer_removes_unused_scene_data";
  knotwork::Scene scene = sample_scene();
  scene.strings.push_back("unused");
  scene.materials.push_back({static_cast<std::uint32_t>(scene.strings.size() - 1), {1.0f, 1.0f, 1.0f}, 0.5f, 0.0f});
  knotwork::OptimizationLog log;
  knotwork::Scene optimized = knotwork::optimize_scene(scene, {}, log);
  require(optimized.materials.size() + 1 == scene.materials.size(), test, "unused material not removed");
  require(!log.messages.empty(), test, "optimizer did not log changes");
}

void command_history_undoes_transform() {
  const std::string test = "command_history_undoes_transform";
  knotwork::Scene scene = sample_scene();
  knotwork::CommandHistory history;
  knotwork::Command command = knotwork::make_transform_command(scene, 1, knotwork::translated(9.0f, 0.0f, 0.0f));
  require(history.execute(scene, command), test, "command execution failed");
  require(knotwork::nearly_equal(scene.nodes[1].local.translation.x, 9.0f), test, "redo transform mismatch");
  require(history.undo(scene), test, "undo failed");
  require(knotwork::nearly_equal(scene.nodes[1].local.translation.x, 1.0f), test, "undo transform mismatch");
  require(history.redo(scene), test, "redo failed");
}

void editor_scene_replacement_clears_replay_history() {
  const std::string test = "editor_scene_replacement_clears_replay_history";
  knotwork::Project project = knotwork::make_project("editor", {});
  project.scene = sample_scene();
  knotwork::EditorState editor(project);
  require(editor.transform_node(1, knotwork::translated(9.0f, 0.0f, 0.0f)), test,
          "initial transform failed");
  require(editor.undo(), test, "undo before replacement failed");

  knotwork::Scene replacement;
  replacement.strings = {"only-root"};
  replacement.nodes.push_back(make_node(99, 0, knotwork::NodeKind::Empty));
  editor.set_scene(replacement);

  require(!editor.undo(), test, "undo replayed across scene replacement");
  require(!editor.redo(), test, "redo replayed across scene replacement");
  require(editor.project().scene.nodes.size() == 1, test, "replacement scene was changed");
  require(editor.project().scene.nodes.front().id == 99, test, "replacement node changed");
}

void project_package_and_report_are_generated() {
  const std::string test = "project_package_and_report_are_generated";
  knotwork::Project project = knotwork::make_project("demo", "/tmp/demo");
  project.scene = sample_scene();
  const knotwork::MeshHandle handle = project.assets.add_mesh(knotwork::make_plane_mesh("floor", 2.0f, 2.0f));
  (void)handle;
  knotwork::add_project_file(project, "scene.kwscene", knotwork::PackageEntryKind::Scene, "main scene");
  knotwork::Package package = knotwork::project_to_package(project);
  require(package.entries.size() >= 2, test, "project package missing entries");
  knotwork::Project loaded = knotwork::package_to_project(package, "loaded");
  require(!loaded.scene.nodes.empty(), test, "package project missing scene");
  require(knotwork::project_markdown_report(project).find("Knotwork Project Report") != std::string::npos,
          test, "project report missing title");
}

void camera_frustum_finds_visible_bounds() {
  const std::string test = "camera_frustum_finds_visible_bounds";
  knotwork::Scene scene = sample_scene();
  std::vector<knotwork::LocalBounds> bounds{{1, {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}}}};
  knotwork::CameraView camera;
  camera.eye = {1.0f, 2.0f, 10.0f};
  camera.target = {1.0f, 2.0f, 0.0f};
  camera.aspect = 1.0f;
  auto visible = knotwork::visible_nodes(scene, bounds, camera);
  require(!visible.empty(), test, "camera did not see mesh bounds");
}

void material_library_finds_builtin_tags() {
  const std::string test = "material_library_finds_builtin_tags";
  knotwork::MaterialLibrary library;
  for (knotwork::MaterialPreset preset : knotwork::built_in_materials()) {
    const std::uint32_t index = library.add(std::move(preset));
    (void)index;
  }
  require(library.size() >= 5, test, "built-in material count too small");
  require(!library.search_by_tag("metal").empty(), test, "metal tag search failed");
  const knotwork::MaterialPreset* preset = library.get(*library.find("brushed-steel"));
  require(preset != nullptr, test, "brushed steel missing");
  if (preset != nullptr) {
    knotwork::Material material;
    material.base_color = preset->base_color;
    material.roughness = preset->roughness;
    material.metallic = preset->metallic;
    knotwork::Vec3 preview = knotwork::preview_lambert(material, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    require(preview.x > 0.0f, test, "material preview was black");
  }
}

void layout_patch_moves_selected_nodes() {
  const std::string test = "layout_patch_moves_selected_nodes";
  knotwork::Scene scene = sample_scene();
  knotwork::Selection selection;
  knotwork::select(selection, 1);
  knotwork::select(selection, 2);
  knotwork::ScenePatch patch = knotwork::distribute_patch(scene, selection, knotwork::LayoutAxis::X, -3.0f, 3.0f);
  knotwork::PatchResult result = knotwork::apply_patch(scene, patch);
  require(!knotwork::patch_has_errors(result), test, "layout patch failed");
  require(knotwork::nearly_equal(result.scene.nodes[1].local.translation.x, -3.0f), test,
          "first selected node not distributed");
  require(knotwork::nearly_equal(result.scene.nodes[2].local.translation.x, 3.0f), test,
          "second selected node not distributed");
}

void scene_script_builds_scene() {
  const std::string test = "scene_script_builds_scene";
  const std::string script =
      "material matte 0.8 0.7 0.6\n"
      "node root empty - 0 0 0\n"
      "node mesh mesh root 1 2 3\n";
  knotwork::ScriptResult result = knotwork::scene_from_script(script);
  require(!knotwork::script_has_errors(result), test, "script parser reported errors");
  require(result.scene.nodes.size() == 2, test, "script node count mismatch");
  require(knotwork::scene_to_script(result.scene).find("node mesh") != std::string::npos, test,
          "script export missing mesh");
}

void audit_and_csv_exports_work() {
  const std::string test = "audit_and_csv_exports_work";
  knotwork::AssetLibrary assets;
  knotwork::Scene scene = sample_scene();
  knotwork::AuditOptions options;
  options.require_light = true;
  auto findings = knotwork::audit_scene(scene, assets, options);
  require(!findings.empty(), test, "audit should report missing light or asset");
  require(knotwork::audit_to_text(scene, findings).find("warning") != std::string::npos, test,
          "audit text missing warning");
  require(knotwork::scene_nodes_csv(scene).find("index,id,name") != std::string::npos, test,
          "node CSV missing header");
  require(knotwork::materials_csv(scene).find("roughness") != std::string::npos, test,
          "material CSV missing header");
}

void keyframe_editor_merges_tracks() {
  const std::string test = "keyframe_editor_merges_tracks";
  knotwork::AnimationClip a;
  a.name = "a";
  a.duration = 1.0f;
  a.tracks.push_back({1, {{0.0f, knotwork::translated(0.0f, 0.0f, 0.0f), knotwork::Interpolation::Linear}}});
  knotwork::AnimationClip b;
  b.name = "b";
  b.duration = 2.0f;
  b.tracks.push_back({1, {{1.0f, knotwork::translated(1.0f, 0.0f, 0.0f), knotwork::Interpolation::Linear}}});
  knotwork::AnimationClip merged = knotwork::merge_clips(a, b, "merged");
  knotwork::ClipSummary summary = knotwork::summarize_clip(merged);
  require(summary.track_count == 1, test, "merged track count mismatch");
  require(summary.key_count == 2, test, "merged key count mismatch");
  knotwork::shift_clip(merged, 1.0f);
  require(knotwork::nearly_equal(knotwork::summarize_clip(merged).first_time, 1.0f), test,
          "shifted first time mismatch");
}

void pathfinding_connects_scene_nodes() {
  const std::string test = "pathfinding_connects_scene_nodes";
  knotwork::Scene scene = sample_scene();
  knotwork::GraphPath path = knotwork::shortest_path(scene, 1, 2);
  require(!path.empty(), test, "path between siblings missing");
  require(path.nodes.front() == 1 && path.nodes.back() == 2, test, "path endpoints mismatch");
  require(knotwork::connected(scene, 1, 2), test, "connected returned false");
}

void scene_merge_and_unit_scaling_work() {
  const std::string test = "scene_merge_and_unit_scaling_work";
  knotwork::Scene base = sample_scene();
  knotwork::Scene imported = sample_scene();
  knotwork::MergeResult merged = knotwork::merge_scene(base, imported);
  require(merged.scene.nodes.size() == base.nodes.size() + imported.nodes.size(), test,
          "merge node count mismatch");
  knotwork::scale_scene_units(merged.scene, knotwork::Unit::Meter, knotwork::Unit::Centimeter);
  require(knotwork::nearly_equal(merged.scene.nodes[1].local.translation.x, 100.0f), test,
          "unit scaling mismatch");
  knotwork::Scene subtree = knotwork::isolate_subtree(base, 0);
  require(!subtree.nodes.empty(), test, "subtree isolation failed");
}

void text_tables_render_scene_and_assets() {
  const std::string test = "text_tables_render_scene_and_assets";
  knotwork::AssetLibrary assets;
  const knotwork::MeshHandle handle = assets.add_mesh(knotwork::make_plane_mesh("floor", 1.0f, 1.0f));
  (void)handle;
  std::string nodes = knotwork::render_table(knotwork::scene_node_table(sample_scene()));
  std::string asset_text = knotwork::render_table(knotwork::asset_table(assets));
  require(nodes.find("mesh") != std::string::npos, test, "node table missing mesh");
  require(asset_text.find("floor") != std::string::npos, test, "asset table missing floor");
}

void newest_reporting_modules_work() {
  const std::string test = "newest_reporting_modules_work";
  knotwork::Project project = knotwork::make_project("reporting", "/tmp/reporting");
  project.scene = sample_scene();
  const knotwork::MeshHandle handle = project.assets.add_mesh(knotwork::make_plane_mesh("floor", 1.0f, 1.0f));
  (void)handle;
  knotwork::ValidationReport validation = knotwork::validate_scene_detailed(project.scene);
  require(validation.ok(), test, "detailed validation failed for sample scene");
  require(knotwork::validation_report_text(validation).find("validation=ok") != std::string::npos, test,
          "validation report missing status");
  knotwork::ConsistencyReport consistency = knotwork::check_project_consistency(project);
  require(consistency.ok(), test, "project consistency reported an error");
  require(knotwork::consistency_report_text(consistency).find("consistency=ok") != std::string::npos, test,
          "consistency report missing status");
  require(knotwork::dump_project(project).find("Project reporting") != std::string::npos, test,
          "project dump missing project name");
  require(knotwork::schema_markdown(knotwork::scene_schema()).find("Node") != std::string::npos, test,
          "schema markdown missing Node");
}

void string_and_checksum_utilities_work() {
  const std::string test = "string_and_checksum_utilities_work";
  require(knotwork::trim("  hello  ") == "hello", test, "trim failed");
  require(knotwork::slugify("Hello, Knotwork!") == "hello-knotwork", test, "slugify failed");
  require(knotwork::iequals("Scene", "scene"), test, "iequals failed");
  require(knotwork::starts_with_ignore_case("Knotwork", "knot"), test, "prefix failed");
  require(knotwork::replace_all("a/b/c", "/", ".") == "a.b.c", test, "replace failed");
  require(knotwork::repeat("ab", 3) == "ababab", test, "repeat failed");
  require(knotwork::indent("x\n", 2).find("  x") != std::string::npos, test, "indent failed");
  require(knotwork::prefix_lines("a\nb", "> ").find("> b") != std::string::npos, test,
          "prefix lines failed");
  std::vector<std::string> wrapped = knotwork::wrap_words("alpha beta gamma", 8);
  require(wrapped.size() >= 2, test, "word wrap failed");
  require(knotwork::to_upper("abc") == "ABC", test, "upper failed");
  require(knotwork::to_lower("ABC") == "abc", test, "lower failed");
  require(knotwork::contains_ignore_case("Knotwork Scene", "scene"), test, "contains failed");
  bool bool_value = false;
  require(knotwork::parse_bool("yes", bool_value) && bool_value, test, "bool parse failed");
  std::uint32_t int_value = 0;
  require(knotwork::parse_u32("42", int_value) && int_value == 42, test, "u32 parse failed");
  require(knotwork::ensure_suffix("scene", ".kwscene") == "scene.kwscene", test, "suffix failed");
  std::vector<std::string> values = {"prefix-a", "prefix-b", "prefix-c"};
  require(knotwork::common_prefix(values) == "prefix-", test, "common prefix failed");
  require(knotwork::edit_distance("kitten", "sitting") == 3, test, "edit distance failed");
  require(knotwork::number_lines("a\nb", 10).find("11: b") != std::string::npos, test,
          "number lines failed");
  require(knotwork::pad_left("7", 3, '0') == "007", test, "left pad failed");
  require(knotwork::pad_right("x", 3, '.') == "x..", test, "right pad failed");
  require(knotwork::truncate("abcdef", 5) == "ab...", test, "truncate failed");
  std::vector<std::string> lines = knotwork::split_lines("one\ntwo\nthree");
  require(lines.size() == 3, test, "split lines failed");
  std::vector<std::string> words = knotwork::split_words_copy("one two  three");
  require(words.size() == 3, test, "split words failed");
  require(knotwork::join(words, ",") == "one,two,three", test, "join failed");
  require(knotwork::hex(255, 4) == "00ff", test, "hex failed");
  require(knotwork::bytes_from_string("abc").size() == 3, test, "bytes conversion failed");
  require(knotwork::checksum_text({1, 2, 3}).find("crc32") != std::string::npos, test,
          "manual checksum text failed");
  require(knotwork::pad_left(knotwork::to_lower("A"), 2, '_') == "_a", test,
          "combined string helpers failed");
  require(knotwork::truncate(knotwork::number_lines("alpha", 1), 4) == "1...",
          test, "combined truncate failed");
  knotwork::Checksums sums = knotwork::checksums(knotwork::bytes_from_string("abc"));
  require(sums.crc32 != 0, test, "crc32 failed");
  require(knotwork::checksum_text(sums).find("fnv1a64") != std::string::npos, test,
          "checksum text failed");
}

void schema_and_version_helpers_work() {
  const std::string test = "schema_and_version_helpers_work";
  std::vector<knotwork::TypeSchema> schema = knotwork::scene_schema();
  require(!schema.empty(), test, "schema is empty");
  require(knotwork::find_type_schema(schema, "Scene") != nullptr, test, "Scene schema missing");
  std::vector<std::string> names = knotwork::schema_type_names(schema);
  require(!names.empty(), test, "schema names empty");
  require(knotwork::schema_text(schema).find("Transform") != std::string::npos, test,
          "schema text missing transform");
  require(knotwork::schema_markdown(schema).find("Knotwork Schema") != std::string::npos, test,
          "schema markdown missing title");
  knotwork::VersionInfo info = knotwork::version_info();
  require(info.name == "Knotwork", test, "version name mismatch");
  require(knotwork::version_string().find("0.1.0") != std::string::npos, test,
          "version string mismatch");
  require(knotwork::build_capabilities().find("validation") != std::string::npos, test,
          "capabilities missing validation");
  require(knotwork::find_type_schema(schema, "Nope") == nullptr, test,
          "missing schema lookup should fail");
  require(knotwork::schema_type_names({}).empty(), test,
          "empty schema names should be empty");
  require(knotwork::schema_text({}).empty(), test,
          "empty schema text should be empty");
  require(knotwork::schema_markdown({}).find("Knotwork Schema") != std::string::npos,
          test,
          "empty schema markdown should still have title");
  require(!knotwork::version_string().empty(),
          test,
          "version string should not be empty");
  require(knotwork::version_info().major == 0, test, "version major mismatch");
  require(knotwork::version_info().minor == 1, test, "version minor mismatch");
}

}  // namespace

int main() {
  round_trip_preserves_tables();
  flatten_composes_parent_transform();
  flatten_applies_instance_target();
  parent_cycle_is_rejected();
  deep_parent_chain_is_rejected_without_stack_growth();
  instance_cycle_is_rejected();
  malformed_bytes_do_not_load();
  mesh_primitives_have_expected_stats();
  obj_round_trip_loads_triangles();
  asset_library_searches_and_summarizes();
  raycast_hits_mesh_in_scene();
  patches_and_diffs_apply_scene_changes();
  package_round_trip_preserves_entries();
  malformed_package_count_is_rejected_before_reserve();
  inspection_reports_scene_and_assets();
  mesh_binary_round_trip_preserves_hash();
  malformed_mesh_counts_are_rejected_before_allocation();
  bvh_raycast_matches_direct_mesh_raycast();
  optimizer_removes_unused_scene_data();
  command_history_undoes_transform();
  editor_scene_replacement_clears_replay_history();
  project_package_and_report_are_generated();
  camera_frustum_finds_visible_bounds();
  material_library_finds_builtin_tags();
  layout_patch_moves_selected_nodes();
  scene_script_builds_scene();
  audit_and_csv_exports_work();
  keyframe_editor_merges_tracks();
  pathfinding_connects_scene_nodes();
  scene_merge_and_unit_scaling_work();
  text_tables_render_scene_and_assets();
  newest_reporting_modules_work();
  string_and_checksum_utilities_work();
  schema_and_version_helpers_work();

  if (!failures().empty()) {
    for (const Failure& failure : failures()) {
      std::cerr << failure.name << ": " << failure.message << "\n";
    }
    return EXIT_FAILURE;
  }
  std::cout << "all tests passed\n";
  return EXIT_SUCCESS;
}
