#include "knotwork/scene_merge.hpp"

#include <map>

namespace knotwork {
namespace {

std::uint32_t copy_string(Scene& out, const Scene& input, std::uint32_t index, const MergeOptions& options) {
  if (index == kNoIndex || index >= input.strings.size()) {
    return kNoIndex;
  }
  std::string value = input.strings[index];
  if (options.prefix_imported_names) {
    value = options.prefix + value;
  }
  out.strings.push_back(std::move(value));
  return static_cast<std::uint32_t>(out.strings.size() - 1);
}

}  // namespace

MergeResult merge_scene(const Scene& base, const Scene& imported, MergeOptions options) {
  MergeResult result;
  result.scene = base;
  std::map<std::uint32_t, std::uint32_t> string_map;
  std::map<std::uint32_t, std::uint32_t> material_map;
  std::map<std::uint32_t, std::uint32_t> node_map;

  for (std::uint32_t i = 0; i < imported.strings.size(); ++i) {
    string_map[i] = copy_string(result.scene, imported, i, options);
  }
  for (std::uint32_t i = 0; i < imported.materials.size(); ++i) {
    Material material = imported.materials[i];
    if (material.name != kNoIndex) {
      material.name = string_map[material.name];
    }
    result.scene.materials.push_back(material);
    const std::uint32_t new_index = static_cast<std::uint32_t>(result.scene.materials.size() - 1);
    material_map[i] = new_index;
    result.imported_materials.push_back(new_index);
  }
  for (std::uint32_t i = 0; i < imported.nodes.size(); ++i) {
    Node node = imported.nodes[i];
    if (node.name != kNoIndex) {
      node.name = string_map[node.name];
    }
    if (node.material != kNoIndex) {
      node.material = material_map[node.material];
    }
    node.parent = kNoIndex;
    node.instance_target = kNoIndex;
    result.scene.nodes.push_back(node);
    const std::uint32_t new_index = static_cast<std::uint32_t>(result.scene.nodes.size() - 1);
    node_map[i] = new_index;
    result.imported_nodes.push_back(new_index);
  }
  for (std::uint32_t i = 0; i < imported.nodes.size(); ++i) {
    Node& node = result.scene.nodes[node_map[i]];
    if (imported.nodes[i].parent != kNoIndex) {
      node.parent = node_map[imported.nodes[i].parent];
    }
    if (imported.nodes[i].instance_target != kNoIndex) {
      node.instance_target = node_map[imported.nodes[i].instance_target];
    }
  }
  return result;
}

Scene isolate_subtree(const Scene& scene, std::uint32_t root) {
  Scene out;
  if (root >= scene.nodes.size()) {
    return out;
  }
  Selection selection = select_descendants(scene, root);
  selection.nodes.insert(root);
  return clone_selection(scene, selection);
}

Scene clone_selection(const Scene& scene, const Selection& selection) {
  Scene out;
  out.strings = scene.strings;
  out.materials = scene.materials;
  std::map<std::uint32_t, std::uint32_t> node_map;
  for (std::uint32_t node_index : selection.nodes) {
    if (node_index >= scene.nodes.size()) {
      continue;
    }
    node_map[node_index] = static_cast<std::uint32_t>(out.nodes.size());
    out.nodes.push_back(scene.nodes[node_index]);
  }
  for (const auto& [old_index, new_index] : node_map) {
    Node& node = out.nodes[new_index];
    if (node.parent != kNoIndex && node_map.contains(node.parent)) {
      node.parent = node_map[node.parent];
    } else {
      node.parent = kNoIndex;
    }
    if (node.instance_target != kNoIndex && node_map.contains(node.instance_target)) {
      node.instance_target = node_map[node.instance_target];
    } else if (node.kind == NodeKind::Instance) {
      node.instance_target = kNoIndex;
    }
  }
  return out;
}

}  // namespace knotwork
