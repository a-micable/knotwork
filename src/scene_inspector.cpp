#include "knotwork/scene_inspector.hpp"
#include "knotwork/graph.hpp"

#include <map>
#include <sstream>

namespace knotwork {

std::vector<std::string> missing_asset_notes(const Scene& scene, const AssetLibrary& assets) {
  std::vector<std::string> notes;
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    if (node.kind != NodeKind::Mesh && node.kind != NodeKind::Instance) {
      continue;
    }
    if (node.material == kNoIndex) {
      continue;
    }
    MeshHandle handle{node.material};
    if (assets.get(handle) == nullptr) {
      std::ostringstream out;
      out << "node " << scene.string_at(node.name).value_or("<unnamed>")
          << " references missing mesh asset " << node.material;
      notes.push_back(out.str());
    }
  }
  return notes;
}

std::vector<std::string> material_usage_notes(const Scene& scene) {
  std::map<std::uint32_t, std::uint32_t> usage;
  for (const Node& node : scene.nodes) {
    if (node.material != kNoIndex) {
      ++usage[node.material];
    }
  }
  std::vector<std::string> notes;
  for (std::uint32_t i = 0; i < scene.materials.size(); ++i) {
    if (usage[i] == 0) {
      std::ostringstream out;
      out << "material " << scene.string_at(scene.materials[i].name).value_or("<unnamed>") << " is unused";
      notes.push_back(out.str());
    }
  }
  for (const auto& [material, count] : usage) {
    if (material >= scene.materials.size()) {
      std::ostringstream out;
      out << "invalid material " << material << " is referenced " << count << " time(s)";
      notes.push_back(out.str());
    }
  }
  return notes;
}

SceneInspection inspect_scene(const Scene& scene, const AssetLibrary& assets) {
  SceneInspection inspection;
  inspection.graph = graph_stats(scene);
  inspection.assets = assets.stats();
  inspection.diagnostics = diagnose_scene(scene);
  std::vector<std::string> missing = missing_asset_notes(scene, assets);
  inspection.notes.insert(inspection.notes.end(), missing.begin(), missing.end());
  std::vector<std::string> materials = material_usage_notes(scene);
  inspection.notes.insert(inspection.notes.end(), materials.begin(), materials.end());
  if (inspection.graph.max_depth > 64) {
    inspection.notes.push_back("scene hierarchy is very deep");
  }
  if (inspection.assets.mesh_count == 0) {
    inspection.notes.push_back("asset library has no meshes");
  }
  return inspection;
}

std::string inspection_to_text(const Scene& scene, const SceneInspection& inspection) {
  std::ostringstream out;
  out << "Scene\n";
  out << "  roots: " << inspection.graph.roots << "\n";
  out << "  leaves: " << inspection.graph.leaves << "\n";
  out << "  max-depth: " << inspection.graph.max_depth << "\n";
  out << "  meshes: " << inspection.graph.mesh_count << "\n";
  out << "  lights: " << inspection.graph.light_count << "\n";
  out << "  cameras: " << inspection.graph.camera_count << "\n";
  out << "  instances: " << inspection.graph.instance_count << "\n";
  out << "Assets\n";
  out << "  mesh-assets: " << inspection.assets.mesh_count << "\n";
  out << "  vertices: " << inspection.assets.vertex_count << "\n";
  out << "  triangles: " << inspection.assets.triangle_count << "\n";
  out << "Diagnostics\n";
  for (const Diagnostic& diagnostic : inspection.diagnostics) {
    out << "  " << format_diagnostic(scene, diagnostic) << "\n";
  }
  out << "Notes\n";
  for (const std::string& note : inspection.notes) {
    out << "  " << note << "\n";
  }
  return out.str();
}

}  // namespace knotwork
