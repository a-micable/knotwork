#pragma once

#include "knotwork/graph.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace knotwork {

struct NodeQuery {
  std::optional<NodeKind> kind;
  std::optional<std::uint32_t> parent;
  std::optional<std::uint32_t> material;
  std::string name_contains;
  bool roots_only = false;
  bool leaves_only = false;
};

[[nodiscard]] bool matches_query(const Scene& scene, std::uint32_t index, const NodeQuery& query);
[[nodiscard]] std::vector<std::uint32_t> query_nodes(const Scene& scene, const NodeQuery& query);
[[nodiscard]] std::vector<std::uint32_t> filter_nodes(
    const Scene& scene,
    const std::function<bool(const Scene&, std::uint32_t)>& predicate);

}  // namespace knotwork
