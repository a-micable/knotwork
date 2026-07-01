#include "knotwork/report.hpp"
#include "knotwork/graph.hpp"

#include <sstream>

namespace knotwork {
namespace {

void write_hierarchy(std::ostringstream& out, const Scene& scene, std::uint32_t node, std::uint32_t depth) {
  for (std::uint32_t i = 0; i < depth; ++i) {
    out << "  ";
  }
  const Node& item = scene.nodes[node];
  out << "- " << scene.string_at(item.name).value_or("<unnamed>") << " (" << node_kind_name(item.kind)
      << ")\n";
  for (std::uint32_t child : child_nodes(scene, node)) {
    write_hierarchy(out, scene, child, depth + 1);
  }
}

}  // namespace

std::string scene_markdown_report(const Scene& scene, const AssetLibrary& assets, ReportOptions options) {
  const SceneInspection inspection = inspect_scene(scene, assets);
  std::ostringstream out;
  out << "# Knotwork Scene Report\n\n";
  out << "## Summary\n\n";
  out << "- Nodes: " << scene.nodes.size() << "\n";
  out << "- Materials: " << scene.materials.size() << "\n";
  out << "- Mesh assets: " << inspection.assets.mesh_count << "\n";
  out << "- Triangles: " << inspection.assets.triangle_count << "\n";
  out << "- Max depth: " << inspection.graph.max_depth << "\n\n";

  if (options.include_hierarchy) {
    out << "## Hierarchy\n\n";
    for (std::uint32_t root : root_nodes(scene)) {
      write_hierarchy(out, scene, root, 0);
    }
    out << "\n";
  }

  if (options.include_assets) {
    out << "## Assets\n\n";
    for (MeshHandle handle : assets.handles()) {
      const MeshAsset* mesh = assets.get(handle);
      if (mesh == nullptr) {
        continue;
      }
      const MeshStats stats = mesh_stats(*mesh);
      out << "- `" << mesh->name << "`: " << stats.vertex_count << " vertices, "
          << stats.triangle_count << " triangles\n";
    }
    out << "\n";
  }

  if (options.include_diagnostics) {
    out << "## Diagnostics\n\n";
    if (inspection.diagnostics.empty() && inspection.notes.empty()) {
      out << "No diagnostics.\n\n";
    }
    for (const Diagnostic& diagnostic : inspection.diagnostics) {
      out << "- " << format_diagnostic(scene, diagnostic) << "\n";
    }
    for (const std::string& note : inspection.notes) {
      out << "- " << note << "\n";
    }
    out << "\n";
  }

  return out.str();
}

std::string project_markdown_report(const Project& project, ReportOptions options) {
  std::ostringstream out;
  out << "# Knotwork Project Report\n\n";
  out << "- Project: " << project.name << "\n";
  out << "- Root: " << project.root.string() << "\n";
  out << "- Files: " << project.files.size() << "\n\n";
  if (options.include_manifest) {
    out << "## Manifest\n\n";
    out << "```text\n" << project_manifest_text(project) << "```\n\n";
  }
  out << scene_markdown_report(project.scene, project.assets, options);
  return out.str();
}

}  // namespace knotwork
