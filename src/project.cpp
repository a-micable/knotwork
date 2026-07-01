#include "knotwork/project.hpp"
#include "knotwork/mesh_io.hpp"
#include "knotwork/obj.hpp"

#include <sstream>

namespace knotwork {

Project make_project(std::string name, std::filesystem::path root) {
  Project project;
  project.name = std::move(name);
  project.root = std::move(root);
  return project;
}

void add_project_file(Project& project, std::filesystem::path path, PackageEntryKind kind, std::string label) {
  ProjectFile file;
  file.path = std::move(path);
  file.kind = kind;
  file.label = std::move(label);
  project.files.push_back(std::move(file));
}

std::string project_manifest_text(const Project& project) {
  std::ostringstream out;
  out << "project " << project.name << "\n";
  out << "root " << project.root.string() << "\n";
  for (const ProjectFile& file : project.files) {
    out << "file " << static_cast<int>(file.kind) << " " << file.path.string();
    if (!file.label.empty()) {
      out << " label=" << file.label;
    }
    out << "\n";
  }
  return out.str();
}

Project package_to_project(const Package& package, std::string name) {
  Project project = make_project(std::move(name), {});
  for (const PackageEntry& entry : package.entries) {
    add_project_file(project, entry.name, entry.kind, entry.name);
    if (entry.kind == PackageEntryKind::Scene && project.scene.nodes.empty()) {
      auto scene = load_scene(entry.payload);
      if (scene.ok()) {
        project.scene = scene.value();
      }
    }
    if (entry.kind == PackageEntryKind::Mesh) {
      const std::string text(reinterpret_cast<const char*>(entry.payload.data()), entry.payload.size());
      ObjLoadResult obj = load_obj(text);
      if (!obj_has_errors(obj)) {
        const MeshHandle handle = project.assets.add_mesh(std::move(obj.mesh), entry.name, {"package"});
        (void)handle;
      }
    }
  }
  return project;
}

Package project_to_package(const Project& project) {
  Package package;
  package.entries.push_back(make_text_entry("manifest.txt", project_manifest_text(project)));
  package.entries.push_back(make_scene_entry("scene.kwscene", project.scene));
  for (MeshHandle handle : project.assets.handles()) {
    const MeshAsset* mesh = project.assets.get(handle);
    if (mesh == nullptr) {
      continue;
    }
    package.entries.push_back(make_mesh_obj_entry(mesh->name + ".obj", *mesh));
  }
  return package;
}

SceneInspection inspect_project(const Project& project) {
  return inspect_scene(project.scene, project.assets);
}

}  // namespace knotwork
