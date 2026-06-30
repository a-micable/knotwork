#include "knotwork/graph.hpp"

#include <algorithm>
#include <queue>
#include <sstream>

namespace knotwork {

std::vector<std::uint32_t> root_nodes(const Scene& scene) {
  std::vector<std::uint32_t> roots;
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    if (scene.nodes[i].parent == kNoIndex) {
      roots.push_back(i);
    }
  }
  return roots;
}

std::vector<std::uint32_t> child_nodes(const Scene& scene, std::uint32_t parent) {
  std::vector<std::uint32_t> children;
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    if (scene.nodes[i].parent == parent) {
      children.push_back(i);
    }
  }
  return children;
}

std::vector<std::uint32_t> ancestors(const Scene& scene, std::uint32_t index) {
  std::vector<std::uint32_t> out;
  while (index < scene.nodes.size() && scene.nodes[index].parent != kNoIndex) {
    index = scene.nodes[index].parent;
    out.push_back(index);
  }
  return out;
}

std::vector<std::uint32_t> descendants(const Scene& scene, std::uint32_t index) {
  std::vector<std::uint32_t> out;
  std::queue<std::uint32_t> pending;
  pending.push(index);
  while (!pending.empty()) {
    const std::uint32_t current = pending.front();
    pending.pop();
    for (std::uint32_t child : child_nodes(scene, current)) {
      out.push_back(child);
      pending.push(child);
    }
  }
  return out;
}

std::optional<std::uint32_t> find_node_by_name(const Scene& scene, std::string_view name) {
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const auto value = scene.string_at(scene.nodes[i].name);
    if (value && *value == name) {
      return i;
    }
  }
  return std::nullopt;
}

std::optional<NodePath> path_to_node(const Scene& scene, std::uint32_t index) {
  if (index >= scene.nodes.size()) {
    return std::nullopt;
  }
  NodePath path;
  path.indices = ancestors(scene, index);
  std::reverse(path.indices.begin(), path.indices.end());
  path.indices.push_back(index);

  std::ostringstream display;
  for (std::size_t i = 0; i < path.indices.size(); ++i) {
    if (i != 0) {
      display << "/";
    }
    const Node& node = scene.nodes[path.indices[i]];
    display << scene.string_at(node.name).value_or("<unnamed>");
  }
  path.display = display.str();
  return path;
}

GraphStats graph_stats(const Scene& scene) {
  GraphStats stats;
  stats.roots = static_cast<std::uint32_t>(root_nodes(scene).size());
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    if (child_nodes(scene, i).empty()) {
      ++stats.leaves;
    }
    stats.max_depth = std::max(stats.max_depth, static_cast<std::uint32_t>(ancestors(scene, i).size()));
    switch (node.kind) {
      case NodeKind::Mesh:
        ++stats.mesh_count;
        break;
      case NodeKind::Light:
        ++stats.light_count;
        break;
      case NodeKind::Camera:
        ++stats.camera_count;
        break;
      case NodeKind::Instance:
        ++stats.instance_count;
        break;
      case NodeKind::Empty:
        break;
    }
  }
  return stats;
}

std::vector<std::uint32_t> topological_parent_order(const Scene& scene) {
  std::vector<std::uint32_t> order;
  std::queue<std::uint32_t> pending;
  for (std::uint32_t root : root_nodes(scene)) {
    pending.push(root);
  }
  while (!pending.empty()) {
    const std::uint32_t current = pending.front();
    pending.pop();
    order.push_back(current);
    for (std::uint32_t child : child_nodes(scene, current)) {
      pending.push(child);
    }
  }
  return order;
}

}  // namespace knotwork
