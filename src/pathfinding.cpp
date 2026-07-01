#include "knotwork/pathfinding.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace knotwork {

std::vector<std::uint32_t> adjacent_nodes(const Scene& scene, std::uint32_t node, WalkOptions options) {
  std::vector<std::uint32_t> out;
  if (node >= scene.nodes.size()) {
    return out;
  }
  if (options.include_parent_edges && scene.nodes[node].parent != kNoIndex) {
    out.push_back(scene.nodes[node].parent);
  }
  if (options.include_child_edges) {
    const auto children = child_nodes(scene, node);
    out.insert(out.end(), children.begin(), children.end());
  }
  if (options.include_instance_edges && scene.nodes[node].instance_target != kNoIndex) {
    out.push_back(scene.nodes[node].instance_target);
  }
  return out;
}

GraphPath shortest_path(const Scene& scene, std::uint32_t start, std::uint32_t goal, WalkOptions options) {
  GraphPath path;
  if (start >= scene.nodes.size() || goal >= scene.nodes.size()) {
    return path;
  }
  std::queue<std::uint32_t> pending;
  std::set<std::uint32_t> seen;
  std::map<std::uint32_t, std::uint32_t> previous;
  pending.push(start);
  seen.insert(start);
  while (!pending.empty()) {
    const std::uint32_t current = pending.front();
    pending.pop();
    if (current == goal) {
      break;
    }
    for (std::uint32_t next : adjacent_nodes(scene, current, options)) {
      if (seen.insert(next).second) {
        previous[next] = current;
        pending.push(next);
      }
    }
  }
  if (!seen.contains(goal)) {
    return path;
  }
  for (std::uint32_t at = goal;; at = previous[at]) {
    path.nodes.push_back(at);
    if (at == start) {
      break;
    }
  }
  std::reverse(path.nodes.begin(), path.nodes.end());
  return path;
}

std::vector<std::uint32_t> breadth_first(const Scene& scene, std::uint32_t start, WalkOptions options) {
  std::vector<std::uint32_t> order;
  if (start >= scene.nodes.size()) {
    return order;
  }
  std::queue<std::uint32_t> pending;
  std::set<std::uint32_t> seen;
  pending.push(start);
  seen.insert(start);
  while (!pending.empty()) {
    const std::uint32_t current = pending.front();
    pending.pop();
    order.push_back(current);
    for (std::uint32_t next : adjacent_nodes(scene, current, options)) {
      if (seen.insert(next).second) {
        pending.push(next);
      }
    }
  }
  return order;
}

bool connected(const Scene& scene, std::uint32_t a, std::uint32_t b) {
  return !shortest_path(scene, a, b).empty();
}

}  // namespace knotwork
