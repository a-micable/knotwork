#include "knotwork/import_pipeline.hpp"
#include "knotwork/scene_merge.hpp"
#include "knotwork/text.hpp"

#include <sstream>

namespace knotwork {
namespace {

void message(ImportResult& result, std::string text) {
  result.messages.push_back(std::move(text));
}

void add_error(ImportResult& result, SceneErrorCode code, std::string text) {
  result.errors.push_back({code, std::move(text)});
}

void maybe_add_project_file(Project& project, const ImportRequest& request, PackageEntryKind kind) {
  if (request.add_to_project_files && !request.source_uri.empty()) {
    add_project_file(project, request.source_uri, kind, request.name);
  }
}

ImportResult import_obj(Project& project, const ImportRequest& request) {
  ImportResult result;
  ObjLoadResult obj = load_obj(request.text);
  for (const ObjIssue& issue : obj.issues) {
    std::ostringstream out;
    out << "OBJ line " << issue.line << ": " << issue.message;
    message(result, out.str());
  }
  if (obj_has_errors(obj)) {
    add_error(result, SceneErrorCode::InvalidString, "OBJ import reported issues");
    return result;
  }
  if (!request.name.empty()) {
    obj.mesh.name = request.name;
  }
  const MeshHandle handle = project.assets.add_mesh(std::move(obj.mesh), request.source_uri, {"imported", "obj"});
  (void)handle;
  ++result.imported_meshes;
  maybe_add_project_file(project, request, PackageEntryKind::Mesh);
  message(result, "imported OBJ mesh");
  return result;
}

ImportResult import_mesh_binary(Project& project, const ImportRequest& request) {
  ImportResult result;
  auto mesh = load_mesh_binary(request.bytes);
  if (!mesh.ok()) {
    add_error(result, SceneErrorCode::InvalidString, mesh.error().message);
    return result;
  }
  if (!request.name.empty()) {
    mesh.value().name = request.name;
  }
  const MeshHandle handle = project.assets.add_mesh(std::move(mesh.value()), request.source_uri, {"imported", "binary"});
  (void)handle;
  ++result.imported_meshes;
  maybe_add_project_file(project, request, PackageEntryKind::Mesh);
  message(result, "imported binary mesh");
  return result;
}

ImportResult import_scene(Project& project, const Scene& scene, const ImportRequest& request) {
  ImportResult result;
  if (request.merge_scene) {
    MergeResult merged = merge_scene(project.scene, scene, {.prefix_imported_names = false});
    result.imported_nodes = static_cast<std::uint32_t>(merged.imported_nodes.size());
    result.imported_materials = static_cast<std::uint32_t>(merged.imported_materials.size());
    project.scene = std::move(merged.scene);
    message(result, "merged scene");
  } else {
    project.scene = scene;
    result.imported_nodes = static_cast<std::uint32_t>(scene.nodes.size());
    result.imported_materials = static_cast<std::uint32_t>(scene.materials.size());
    message(result, "replaced scene");
  }
  maybe_add_project_file(project, request, PackageEntryKind::Scene);
  return result;
}

ImportResult import_package(Project& project, const ImportRequest& request) {
  ImportResult result;
  auto package = load_package(request.bytes);
  if (!package.ok()) {
    add_error(result, package.error().code, package.error().message);
    return result;
  }
  Project imported = package_to_project(package.value(), request.name.empty() ? "imported" : request.name);
  MergeResult merged = merge_scene(project.scene, imported.scene, {.prefix_imported_names = false});
  project.scene = std::move(merged.scene);
  for (MeshHandle handle : imported.assets.handles()) {
    const MeshAsset* mesh = imported.assets.get(handle);
    if (mesh != nullptr) {
      const MeshHandle added = project.assets.add_mesh(*mesh, request.source_uri, {"package"});
      (void)added;
      ++result.imported_meshes;
    }
  }
  result.imported_nodes = static_cast<std::uint32_t>(merged.imported_nodes.size());
  result.imported_materials = static_cast<std::uint32_t>(merged.imported_materials.size());
  maybe_add_project_file(project, request, PackageEntryKind::Binary);
  message(result, "imported package");
  return result;
}

}  // namespace

ImportResult import_into_project(Project& project, const ImportRequest& request) {
  switch (request.kind) {
    case ImportKind::ObjText:
      return import_obj(project, request);
    case ImportKind::MeshBinary:
      return import_mesh_binary(project, request);
    case ImportKind::SceneBinary: {
      auto scene = load_scene(request.bytes);
      if (!scene.ok()) {
        return {{}, {}, {}, {}, {scene.error()}};
      }
      return import_scene(project, scene.value(), request);
    }
    case ImportKind::SceneText: {
      auto scene = scene_from_text(request.text);
      if (!scene.ok()) {
        return {{}, {}, {}, {}, {scene.error()}};
      }
      return import_scene(project, scene.value(), request);
    }
    case ImportKind::PackageBinary:
      return import_package(project, request);
  }
  ImportResult result;
  add_error(result, SceneErrorCode::InvalidEnum, "unknown import kind");
  return result;
}

ImportResult import_into_editor(EditorState& editor, const ImportRequest& request) {
  ImportResult result = import_into_project(editor.project(), request);
  for (const std::string& item : result.messages) {
    editor.push_message(Severity::Info, item);
  }
  for (const SceneError& error : result.errors) {
    editor.push_message(Severity::Warning, error.message);
  }
  return result;
}

ImportRequest obj_import(std::string name, std::string text, std::string source_uri) {
  ImportRequest request;
  request.kind = ImportKind::ObjText;
  request.name = std::move(name);
  request.text = std::move(text);
  request.source_uri = std::move(source_uri);
  return request;
}

ImportRequest scene_text_import(std::string name, std::string text, std::string source_uri) {
  ImportRequest request;
  request.kind = ImportKind::SceneText;
  request.name = std::move(name);
  request.text = std::move(text);
  request.source_uri = std::move(source_uri);
  return request;
}

ImportRequest scene_binary_import(std::string name, std::vector<std::byte> bytes, std::string source_uri) {
  ImportRequest request;
  request.kind = ImportKind::SceneBinary;
  request.name = std::move(name);
  request.bytes = std::move(bytes);
  request.source_uri = std::move(source_uri);
  return request;
}

ImportRequest package_import(std::string name, std::vector<std::byte> bytes, std::string source_uri) {
  ImportRequest request;
  request.kind = ImportKind::PackageBinary;
  request.name = std::move(name);
  request.bytes = std::move(bytes);
  request.source_uri = std::move(source_uri);
  return request;
}

bool import_failed(const ImportResult& result) {
  return !result.errors.empty();
}

std::string import_summary(const ImportResult& result) {
  std::ostringstream out;
  out << "meshes=" << result.imported_meshes << "\n";
  out << "nodes=" << result.imported_nodes << "\n";
  out << "materials=" << result.imported_materials << "\n";
  out << "errors=" << result.errors.size() << "\n";
  for (const std::string& item : result.messages) {
    out << "- " << item << "\n";
  }
  return out.str();
}

}  // namespace knotwork
