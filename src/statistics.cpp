#include "knotwork/statistics.hpp"
#include "knotwork/graph.hpp"

#include <sstream>

namespace knotwork {

SceneStatistics collect_scene_statistics(const Scene& scene) {
  SceneStatistics stats;
  for (const std::string& value : scene.strings) {
    stats.string_bytes += static_cast<std::uint32_t>(value.size());
  }
  stats.node_count = static_cast<std::uint32_t>(scene.nodes.size());
  stats.material_count = static_cast<std::uint32_t>(scene.materials.size());
  const GraphStats graph = graph_stats(scene);
  stats.root_count = graph.roots;
  stats.max_depth = graph.max_depth;
  for (const Node& node : scene.nodes) {
    if (node.kind == NodeKind::Instance || node.kind == NodeKind::Camera || node.kind == NodeKind::Light) {
      ++stats.animated_node_estimate;
    }
  }
  return stats;
}

AssetStatistics collect_asset_statistics(const AssetLibrary& assets) {
  AssetStatistics stats;
  stats.mesh_count = assets.stats().mesh_count;
  for (MeshHandle handle : assets.handles()) {
    const MeshAsset* mesh = assets.get(handle);
    if (mesh == nullptr) {
      continue;
    }
    const MeshStats mesh_stats_value = mesh_stats(*mesh);
    stats.vertex_count += mesh_stats_value.vertex_count;
    stats.triangle_count += mesh_stats_value.triangle_count;
    stats.degenerate_triangle_count += mesh_stats_value.degenerate_count;
    stats.surface_area += mesh_stats_value.surface_area;
  }
  return stats;
}

std::string statistics_text(const SceneStatistics& scene, const AssetStatistics& assets) {
  std::ostringstream out;
  out << "scene.nodes=" << scene.node_count << "\n";
  out << "scene.materials=" << scene.material_count << "\n";
  out << "scene.roots=" << scene.root_count << "\n";
  out << "scene.max_depth=" << scene.max_depth << "\n";
  out << "scene.string_bytes=" << scene.string_bytes << "\n";
  out << "assets.meshes=" << assets.mesh_count << "\n";
  out << "assets.vertices=" << assets.vertex_count << "\n";
  out << "assets.triangles=" << assets.triangle_count << "\n";
  out << "assets.degenerates=" << assets.degenerate_triangle_count << "\n";
  out << "assets.surface_area=" << assets.surface_area << "\n";
  return out.str();
}

}  // namespace knotwork
