#include "knotwork/query.hpp"

#include <algorithm>

namespace knotwork {

bool matches_query(const Scene& scene, std::uint32_t index, const NodeQuery& query) {
  if (index >= scene.nodes.size()) {
    return false;
  }
  const Node& node = scene.nodes[index];
  if (query.kind && node.kind != *query.kind) {
    return false;
  }
  if (query.parent && node.parent != *query.parent) {
    return false;
  }
  if (query.material && node.material != *query.material) {
    return false;
  }
  if (query.roots_only && node.parent != kNoIndex) {
    return false;
  }
  if (query.leaves_only && !child_nodes(scene, index).empty()) {
    return false;
  }
  if (!query.name_contains.empty()) {
    const auto name = scene.string_at(node.name).value_or("");
    if (name.find(query.name_contains) == std::string_view::npos) {
      return false;
    }
  }
  return true;
}

std::vector<std::uint32_t> query_nodes(const Scene& scene, const NodeQuery& query) {
  std::vector<std::uint32_t> out;
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    if (matches_query(scene, i, query)) {
      out.push_back(i);
    }
  }
  return out;
}

std::vector<std::uint32_t> filter_nodes(
    const Scene& scene,
    const std::function<bool(const Scene&, std::uint32_t)>& predicate) {
  std::vector<std::uint32_t> out;
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    if (predicate(scene, i)) {
      out.push_back(i);
    }
  }
  return out;
}

}  // namespace knotwork
