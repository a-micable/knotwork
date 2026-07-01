#include "knotwork/debug_dump.hpp"
#include "knotwork/flatten.hpp"

#include <iomanip>
#include <sstream>

namespace knotwork {

std::string dump_scene(const Scene& scene, DumpOptions options) {
  std::ostringstream out;
  out << "Scene\n";
  if (options.include_strings) {
    out << "Strings\n";
    for (std::uint32_t i = 0; i < scene.strings.size(); ++i) {
      out << "  [" << i << "] " << scene.strings[i] << "\n";
    }
  }
  if (options.include_materials) {
    out << "Materials\n";
    for (std::uint32_t i = 0; i < scene.materials.size(); ++i) {
      const Material& material = scene.materials[i];
      out << "  [" << i << "] name=" << material.name << " color=" << material.base_color.to_string()
          << " roughness=" << material.roughness << " metallic=" << material.metallic << "\n";
    }
  }
  if (options.include_nodes) {
    auto flattened = options.include_matrices ? flatten_scene(scene) : Result<std::vector<WorldNode>>(std::vector<WorldNode>{});
    out << "Nodes\n";
    for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
      const Node& node = scene.nodes[i];
      out << "  [" << i << "] id=" << node.id << " name=" << node.name << " kind="
          << node_kind_name(node.kind) << " parent=" << node.parent << " material=" << node.material
          << " instance=" << node.instance_target << " t=" << node.local.translation.to_string() << "\n";
      if (options.include_matrices && flattened.ok() && i < flattened.value().size()) {
        out << "    world=" << flattened.value()[i].world << "\n";
      }
    }
  }
  return out.str();
}

std::string dump_assets(const AssetLibrary& assets) {
  std::ostringstream out;
  out << "Assets\n";
  for (MeshHandle handle : assets.handles()) {
    const MeshRecord* record = assets.record(handle);
    if (record == nullptr) {
      continue;
    }
    const MeshStats stats = mesh_stats(record->mesh);
    out << "  [" << handle.index << "] " << record->mesh.name << " vertices=" << stats.vertex_count
        << " triangles=" << stats.triangle_count << " area=" << stats.surface_area << "\n";
  }
  return out.str();
}

std::string dump_project(const Project& project, DumpOptions options) {
  std::ostringstream out;
  out << "Project " << project.name << "\n";
  out << "Root " << project.root.string() << "\n";
  out << dump_scene(project.scene, options);
  if (options.include_assets) {
    out << dump_assets(project.assets);
  }
  return out.str();
}

std::string dump_hex(std::span<const std::byte> bytes, std::uint32_t columns) {
  std::ostringstream out;
  columns = std::max<std::uint32_t>(columns, 1);
  for (std::uint32_t i = 0; i < bytes.size(); ++i) {
    if (i % columns == 0) {
      out << std::setw(8) << std::setfill('0') << std::hex << i << "  ";
    }
    out << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(bytes[i]) << " ";
    if (i % columns == columns - 1) {
      out << "\n";
    }
  }
  if (bytes.size() % columns != 0) {
    out << "\n";
  }
  return out.str();
}

}  // namespace knotwork
