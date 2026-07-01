#pragma once

#include "knotwork/graph.hpp"

#include <queue>

namespace knotwork {

struct GraphPath {
  std::vector<std::uint32_t> nodes;
  [[nodiscard]] bool empty() const { return nodes.empty(); }
};

struct WalkOptions {
  bool include_parent_edges = true;
  bool include_child_edges = true;
  bool include_instance_edges = true;
};

[[nodiscard]] std::vector<std::uint32_t> adjacent_nodes(const Scene& scene,
                                                        std::uint32_t node,
                                                        WalkOptions options = {});
[[nodiscard]] GraphPath shortest_path(const Scene& scene,
                                      std::uint32_t start,
                                      std::uint32_t goal,
                                      WalkOptions options = {});
[[nodiscard]] std::vector<std::uint32_t> breadth_first(const Scene& scene,
                                                       std::uint32_t start,
                                                       WalkOptions options = {});
[[nodiscard]] bool connected(const Scene& scene, std::uint32_t a, std::uint32_t b);

}  // namespace knotwork
