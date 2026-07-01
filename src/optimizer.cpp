#include "knotwork/optimizer.hpp"
#include "knotwork/graph.hpp"
#include "knotwork/validate.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace knotwork {
namespace {

void log_message(OptimizationLog& log, std::string message) {
  log.messages.push_back(std::move(message));
}

std::string number_message(std::string_view prefix, std::uint32_t count) {
  std::ostringstream out;
  out << prefix << count;
  return out.str();
}

}  // namespace

std::vector<std::uint32_t> unused_materials(const Scene& scene) {
  std::set<std::uint32_t> used;
  for (const Node& node : scene.nodes) {
    if (node.material != kNoIndex) {
      used.insert(node.material);
    }
  }
  std::vector<std::uint32_t> out;
  for (std::uint32_t i = 0; i < scene.materials.size(); ++i) {
    if (!used.contains(i)) {
      out.push_back(i);
    }
  }
  return out;
}

std::vector<std::uint32_t> unused_strings(const Scene& scene) {
  std::set<std::uint32_t> used;
  for (const Material& material : scene.materials) {
    if (material.name != kNoIndex) {
      used.insert(material.name);
    }
  }
  for (const Node& node : scene.nodes) {
    if (node.name != kNoIndex) {
      used.insert(node.name);
    }
  }
  std::vector<std::uint32_t> out;
  for (std::uint32_t i = 0; i < scene.strings.size(); ++i) {
    if (!used.contains(i)) {
      out.push_back(i);
    }
  }
  return out;
}

bool removable_empty_leaf(const Scene& scene, std::uint32_t node) {
  if (node >= scene.nodes.size()) {
    return false;
  }
  return scene.nodes[node].kind == NodeKind::Empty && child_nodes(scene, node).empty();
}

Scene optimize_scene(const Scene& scene, SceneOptimizationOptions options, OptimizationLog& log) {
  Scene out = scene;
  if (options.remove_empty_leaf_nodes) {
    for (std::uint32_t i = static_cast<std::uint32_t>(out.nodes.size()); i > 0; --i) {
      const std::uint32_t index = i - 1;
      if (!removable_empty_leaf(out, index)) {
        continue;
      }
      out.nodes.erase(out.nodes.begin() + index);
      for (Node& node : out.nodes) {
        if (node.parent == index) {
          node.parent = kNoIndex;
        } else if (node.parent > index && node.parent != kNoIndex) {
          --node.parent;
        }
        if (node.instance_target == index) {
          node.instance_target = kNoIndex;
        } else if (node.instance_target > index && node.instance_target != kNoIndex) {
          --node.instance_target;
        }
      }
      ++log.removed_nodes;
    }
    if (log.removed_nodes != 0) {
      log_message(log, number_message("removed empty leaf nodes: ", log.removed_nodes));
    }
  }

  if (options.remove_unused_materials) {
    std::vector<std::uint32_t> unused = unused_materials(out);
    std::sort(unused.rbegin(), unused.rend());
    for (std::uint32_t material : unused) {
      out.materials.erase(out.materials.begin() + material);
      for (Node& node : out.nodes) {
        if (node.material == material) {
          node.material = kNoIndex;
        } else if (node.material > material && node.material != kNoIndex) {
          --node.material;
        }
      }
      ++log.removed_materials;
    }
    if (log.removed_materials != 0) {
      log_message(log, number_message("removed unused materials: ", log.removed_materials));
    }
  }

  if (options.compact_strings) {
    std::vector<std::uint32_t> unused = unused_strings(out);
    std::sort(unused.rbegin(), unused.rend());
    for (std::uint32_t string_index : unused) {
      out.strings.erase(out.strings.begin() + string_index);
      for (Material& material : out.materials) {
        if (material.name > string_index && material.name != kNoIndex) {
          --material.name;
        }
      }
      for (Node& node : out.nodes) {
        if (node.name > string_index && node.name != kNoIndex) {
          --node.name;
        }
      }
      ++log.removed_strings;
    }
    if (log.removed_strings != 0) {
      log_message(log, number_message("removed unused strings: ", log.removed_strings));
    }
  }

  const SceneError validation = validate_scene(out);
  if (validation.code != SceneErrorCode::None) {
    log_message(log, std::string("optimized scene validation failed: ") + validation.message);
    return scene;
  }
  return out;
}

void optimize_assets(AssetLibrary& library, AssetOptimizationOptions options, OptimizationLog& log) {
  for (MeshHandle handle : library.handles()) {
    MeshAsset* mesh = library.get(handle);
    if (mesh == nullptr) {
      continue;
    }
    const std::uint32_t before = static_cast<std::uint32_t>(mesh->vertices.size());
    if (options.weld_vertices) {
      *mesh = weld_vertices(*mesh, options.weld_epsilon);
    }
    if (options.recalculate_normals) {
      recalculate_normals(*mesh);
    }
    if (options.recalculate_tangents) {
      recalculate_tangents(*mesh);
    }
    const std::uint32_t after = static_cast<std::uint32_t>(mesh->vertices.size());
    if (after < before) {
      log.welded_vertices += before - after;
    }
  }
  if (log.welded_vertices != 0) {
    log_message(log, number_message("welded vertices: ", log.welded_vertices));
  }
}

}  // namespace knotwork
