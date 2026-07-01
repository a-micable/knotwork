#pragma once

#include "knotwork/format.hpp"

#include <string>
#include <vector>

namespace knotwork {

enum class PatchOpKind {
  AddString,
  RenameString,
  AddMaterial,
  SetMaterialColor,
  AddNode,
  RemoveNode,
  ReparentNode,
  SetTransform,
  SetNodeMaterial,
  SetInstanceTarget,
};

struct PatchOp {
  PatchOpKind kind = PatchOpKind::AddString;
  std::uint32_t index = kNoIndex;
  std::uint32_t other = kNoIndex;
  std::uint32_t third = kNoIndex;
  std::string text;
  Material material{};
  Node node{};
  Transform transform{};
  Vec3 color{};
};

struct ScenePatch {
  std::string name;
  std::vector<PatchOp> ops;
};

struct PatchResult {
  Scene scene;
  std::vector<SceneError> errors;
};

[[nodiscard]] const char* patch_op_name(PatchOpKind kind);
[[nodiscard]] PatchResult apply_patch(const Scene& scene, const ScenePatch& patch);
[[nodiscard]] bool patch_has_errors(const PatchResult& result);
[[nodiscard]] ScenePatch rename_node_patch(std::uint32_t node, std::string name);
[[nodiscard]] ScenePatch reparent_patch(std::uint32_t node, std::uint32_t parent);
[[nodiscard]] ScenePatch transform_patch(std::uint32_t node, Transform transform);
[[nodiscard]] std::string patch_to_text(const ScenePatch& patch);

}  // namespace knotwork
