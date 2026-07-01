#pragma once

#include "knotwork/mesh.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace knotwork {

struct ObjIssue {
  std::uint32_t line = 0;
  std::string message;
};

struct ObjLoadResult {
  MeshAsset mesh;
  std::vector<ObjIssue> issues;
};

struct ObjOptions {
  bool triangulate = true;
  bool recalculate_missing_normals = true;
  bool recalculate_tangents = true;
  bool tolerate_unknown_lines = true;
};

[[nodiscard]] ObjLoadResult load_obj(std::string_view text, ObjOptions options = {});
[[nodiscard]] std::string save_obj(const MeshAsset& mesh);
[[nodiscard]] std::string save_obj_with_material(const MeshAsset& mesh, std::string_view material_name);
[[nodiscard]] bool obj_has_errors(const ObjLoadResult& result);

}  // namespace knotwork
