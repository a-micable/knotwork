#include "knotwork/csv.hpp"

#include <sstream>

namespace knotwork {
namespace {

std::string csv_escape(std::string_view value) {
  bool quote = false;
  for (char ch : value) {
    quote = quote || ch == ',' || ch == '"' || ch == '\n';
  }
  if (!quote) {
    return std::string(value);
  }
  std::string out = "\"";
  for (char ch : value) {
    if (ch == '"') {
      out += "\"\"";
    } else {
      out += ch;
    }
  }
  out += "\"";
  return out;
}

}  // namespace

std::string scene_nodes_csv(const Scene& scene) {
  std::ostringstream out;
  out << "index,id,name,kind,parent,material,instance_target,x,y,z\n";
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    out << i << "," << node.id << "," << csv_escape(scene.string_at(node.name).value_or("")) << ","
        << node_kind_name(node.kind) << "," << node.parent << "," << node.material << ","
        << node.instance_target << "," << node.local.translation.x << "," << node.local.translation.y
        << "," << node.local.translation.z << "\n";
  }
  return out.str();
}

std::string materials_csv(const Scene& scene) {
  std::ostringstream out;
  out << "index,name,r,g,b,roughness,metallic\n";
  for (std::uint32_t i = 0; i < scene.materials.size(); ++i) {
    const Material& material = scene.materials[i];
    out << i << "," << csv_escape(scene.string_at(material.name).value_or("")) << ","
        << material.base_color.x << "," << material.base_color.y << "," << material.base_color.z
        << "," << material.roughness << "," << material.metallic << "\n";
  }
  return out.str();
}

std::string assets_csv(const AssetLibrary& assets) {
  std::ostringstream out;
  out << "handle,name,vertices,triangles,source\n";
  for (MeshHandle handle : assets.handles()) {
    const MeshRecord* record = assets.record(handle);
    if (record == nullptr) {
      continue;
    }
    out << handle.index << "," << csv_escape(record->mesh.name) << "," << record->mesh.vertices.size()
        << "," << record->mesh.triangles.size() << "," << csv_escape(record->source_uri) << "\n";
  }
  return out.str();
}

std::string diagnostics_csv(const Scene& scene, std::span<const Diagnostic> diagnostics) {
  std::ostringstream out;
  out << "severity,code,node,message\n";
  for (const Diagnostic& diagnostic : diagnostics) {
    std::string node_name;
    if (diagnostic.node != kNoIndex && diagnostic.node < scene.nodes.size()) {
      node_name = std::string(scene.string_at(scene.nodes[diagnostic.node].name).value_or(""));
    }
    out << severity_name(diagnostic.severity) << "," << scene_error_name(diagnostic.code) << ","
        << csv_escape(node_name) << "," << csv_escape(diagnostic.message) << "\n";
  }
  return out.str();
}

std::string project_files_csv(const Project& project) {
  std::ostringstream out;
  out << "path,kind,label\n";
  for (const ProjectFile& file : project.files) {
    out << csv_escape(file.path.string()) << "," << static_cast<int>(file.kind) << ","
        << csv_escape(file.label) << "\n";
  }
  return out.str();
}

}  // namespace knotwork
