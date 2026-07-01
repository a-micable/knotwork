#pragma once

#include "knotwork/editor_state.hpp"
#include "knotwork/mesh_io.hpp"
#include "knotwork/obj.hpp"

namespace knotwork {

enum class ImportKind {
  ObjText,
  MeshBinary,
  SceneBinary,
  SceneText,
  PackageBinary,
};

struct ImportRequest {
  ImportKind kind = ImportKind::ObjText;
  std::string name;
  std::string source_uri;
  std::vector<std::byte> bytes;
  std::string text;
  bool merge_scene = true;
  bool add_to_project_files = true;
};

struct ImportResult {
  std::uint32_t imported_meshes = 0;
  std::uint32_t imported_nodes = 0;
  std::uint32_t imported_materials = 0;
  std::vector<std::string> messages;
  std::vector<SceneError> errors;
};

[[nodiscard]] ImportResult import_into_project(Project& project, const ImportRequest& request);
[[nodiscard]] ImportResult import_into_editor(EditorState& editor, const ImportRequest& request);
[[nodiscard]] ImportRequest obj_import(std::string name, std::string text, std::string source_uri = {});
[[nodiscard]] ImportRequest scene_text_import(std::string name, std::string text, std::string source_uri = {});
[[nodiscard]] ImportRequest scene_binary_import(std::string name,
                                                std::vector<std::byte> bytes,
                                                std::string source_uri = {});
[[nodiscard]] ImportRequest package_import(std::string name,
                                           std::vector<std::byte> bytes,
                                           std::string source_uri = {});
[[nodiscard]] bool import_failed(const ImportResult& result);
[[nodiscard]] std::string import_summary(const ImportResult& result);

}  // namespace knotwork
