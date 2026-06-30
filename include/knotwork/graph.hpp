#pragma once

#include "knotwork/scene.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace knotwork {

struct GraphStats {
  std::uint32_t roots = 0;
  std::uint32_t leaves = 0;
  std::uint32_t max_depth = 0;
  std::uint32_t mesh_count = 0;
  std::uint32_t light_count = 0;
  std::uint32_t camera_count = 0;
  std::uint32_t instance_count = 0;
};

struct NodePath {
  std::vector<std::uint32_t> indices;
  std::string display;
};

[[nodiscard]] std::vector<std::uint32_t> root_nodes(const Scene& scene);
[[nodiscard]] std::vector<std::uint32_t> child_nodes(const Scene& scene, std::uint32_t parent);
[[nodiscard]] std::vector<std::uint32_t> ancestors(const Scene& scene, std::uint32_t index);
[[nodiscard]] std::vector<std::uint32_t> descendants(const Scene& scene, std::uint32_t index);
[[nodiscard]] std::optional<std::uint32_t> find_node_by_name(const Scene& scene, std::string_view name);
[[nodiscard]] std::optional<NodePath> path_to_node(const Scene& scene, std::uint32_t index);
[[nodiscard]] GraphStats graph_stats(const Scene& scene);
[[nodiscard]] std::vector<std::uint32_t> topological_parent_order(const Scene& scene);

}  // namespace knotwork
